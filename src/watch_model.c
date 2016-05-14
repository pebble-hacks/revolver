#include "watch_model.h"

typedef struct {
  DotState start_state;
  DotState end_state;
} DotAnimationContext;

typedef struct {
  CenterState start_state;
  CenterState end_state;
} CenterAnimationContext;

static int64_t prv_interpolate_int64_linear(int64_t from, int64_t to, const AnimationProgress progress) {
    return from + ((progress * (to - from)) / ANIMATION_NORMALIZED_MAX);
}

static DotState prv_interpolate_dot_states(DotState *start, DotState *end, uint32_t progress) {
  return (DotState) {
    .ordinal = start->ordinal,
    .distance = (uint32_t)prv_interpolate_int64_linear(start->distance, end->distance, progress),
    .angle = (int32_t)prv_interpolate_int64_linear(start->angle, end->angle, progress),
    .dot_type = start->dot_type
  };
}

static CenterState prv_interpolate_center_states(CenterState *start, CenterState *end, uint32_t progress) {
  return (CenterState) {
    .center_radius = prv_interpolate_int64_linear(start->center_radius, end->center_radius, progress)
  };
}

DotType get_dot_type(uint32_t ordinal, const struct tm *tick_time) {
  DotType dot_type = DotType_NEUTRAL_DOT;
  if ((int32_t)ordinal == tick_time->tm_hour % 12) {
    dot_type |= DotType_HOUR_DOT;
  }
  if ((int32_t)ordinal == tick_time->tm_min / 5) {
    dot_type |= DotType_MINUTE_DOT;
  }
  return dot_type;
}

static void prv_update_center_animation(Animation *center_animation,
                                     const AnimationProgress animation_progress) {
  CenterAnimationContext *center_context = animation_get_context(center_animation);
  CenterState interpolated_state = prv_interpolate_center_states(&center_context->start_state,
                                                                 &center_context->end_state,
                                                                 animation_progress);
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "Updating center animation progress to %lu", animation_progress);
  watch_model_handle_center_change(&interpolated_state);
}

static Animation *prv_make_center_animation(void) {
  Animation *center_animation = animation_create();
  static const AnimationImplementation animation_implementation = {
    .update = prv_update_center_animation,
  };
  animation_set_implementation(center_animation, &animation_implementation);
  animation_set_duration(center_animation, CENTER_ANIMATION_LENGTH);
  animation_set_delay(center_animation, CENTER_ANIMATION_DELAY);
  animation_set_curve(center_animation, AnimationCurveEaseOut);
  CenterAnimationContext *center_context = malloc(sizeof(*center_context));
  center_context->start_state = (CenterState) { 0 };
  center_context->end_state = (CenterState) { CENTER_RADIUS };
  animation_set_handlers(center_animation, (AnimationHandlers) { 0 }, center_context);
  return center_animation;
}

static void prv_teardown_dot_animation(Animation *dot_animation) {
  DotAnimationContext *dot_context = animation_get_context(dot_animation);
  free(dot_context);
}

static void prv_update_dot_animation(Animation *dot_animation,
                                  const AnimationProgress animation_progress) {
  DotAnimationContext *dot_context = animation_get_context(dot_animation);
  DotState interpolated_state = prv_interpolate_dot_states(&dot_context->start_state,
                                                         &dot_context->end_state,
                                                         animation_progress);
  watch_model_handle_dot_change(&interpolated_state);
}

static DotState prv_get_dot_start_state(int32_t ordinal) {
  int32_t angle;
  if (ordinal == 0) {
    angle = ((12 - ordinal) * 2 - 1) * TRIG_MAX_ANGLE / 24;
  } else {
    angle = (ordinal * 2 - 1) * TRIG_MAX_ANGLE / 24;
  }
  return (DotState) {
    .ordinal = ordinal,
    .distance = DOT_ANIMATION_STARTING_DISTANCE + OUTER_DOT_DISTANCE,
    .angle = angle
  };
}

static DotState prv_get_dot_end_state(uint32_t ordinal) {
  int32_t angle = (ordinal == 0) ? TRIG_MAX_ANGLE : (ordinal * TRIG_MAX_ANGLE / 12);
  return (DotState) {
    .ordinal = ordinal,
    .distance = OUTER_DOT_DISTANCE,
    .angle = angle
  };
}

static Animation *prv_make_dot_animation(uint32_t ordinal) {
  Animation *const dot_animation = animation_create();
  static AnimationImplementation animation_implementation;
  animation_implementation = (AnimationImplementation) {
    .teardown = prv_teardown_dot_animation,
    .update = prv_update_dot_animation
  };
  animation_set_implementation(dot_animation, &animation_implementation);
  animation_set_delay(dot_animation, ordinal * DOT_ANIMATION_DELAY + CENTER_ANIMATION_LENGTH);
  animation_set_duration(dot_animation, DOT_ANIMATION_LENGTH);
  animation_set_curve(dot_animation, AnimationCurveEaseOut);
  DotAnimationContext *dot_context = malloc(sizeof(*dot_context));
  dot_context->start_state = prv_get_dot_start_state(ordinal);
  dot_context->end_state = prv_get_dot_end_state(ordinal);
  animation_set_handlers(dot_animation, (AnimationHandlers) { 0 }, dot_context);

  return dot_animation;
}

static void prv_handle_time_update(struct tm *tick_time, TimeUnits units_changed) {
  watch_model_handle_time_change(tick_time);
}

static void prv_finish_animation(Animation *animation, bool finished, void *context) {
  const time_t t = time(NULL);
  prv_handle_time_update(localtime(&t), (TimeUnits)0xff);
  tick_timer_service_subscribe(MINUTE_UNIT, prv_handle_time_update);
}

void watch_model_start_intro(void) {
  Animation *const center_animation = prv_make_center_animation();
  Animation *all_animations[NUM_DOTS_TOTAL + 1];
  all_animations[0] = center_animation;
  for (int i = 0; i < NUM_DOTS_TOTAL; i++) {
    all_animations[i + 1] = prv_make_dot_animation(i);
  }
  Animation *spawned = animation_spawn_create_from_array(all_animations, ARRAY_LENGTH(all_animations));
  animation_set_handlers(spawned, (AnimationHandlers) {
    .stopped = prv_finish_animation,
  }, NULL);
  animation_schedule(spawned);
}

ColorConfig prv_make_current_color_config() {
  return (ColorConfig) {
    .center_color = getTheme_color(),
    .hour_dot_color = getTheme_color(),
    .minute_dot_color = GColorWhite,
    .neutral_dot_color = GColorDarkGray,
  };
}

static void prv_msg_received_handler(DictionaryIterator *iter, void *context) {
  autoconfig_in_received_handler(iter, context);
  ColorConfig current_config = prv_make_current_color_config();
  watch_model_handle_color_config_change(&current_config);
}

void watch_model_init(void) {
  time_t t = time(NULL);
  watch_model_handle_time_change(localtime(&t));
  ColorConfig current_config = prv_make_current_color_config();
  watch_model_handle_color_config_change(&current_config);
  for (int i = 0; i < NUM_DOTS_TOTAL; i++) {
    DotState dot_state = prv_get_dot_start_state(i);
    watch_model_handle_dot_change(&dot_state);
  }
  CenterState start_center_state = (CenterState) { 0 };
  watch_model_handle_center_change(&start_center_state);
  app_message_register_inbox_received(prv_msg_received_handler);
}

