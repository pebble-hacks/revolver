#include "autoconfig.h"
#include <pebble.h>

#define NUM_DOTS_TOTAL (12)

#define OUTER_DOT_DISTANCE 72
#define NEUTRAL_DOT_RADIUS 3
#define HOUR_DOT_RADIUS 5
#define MINUTE_DOT_RADIUS 3
#define CENTER_RADIUS 45

#define DOT_ANIMATION_STARTING_DISTANCE 25
#define DOT_ANIMATION_LENGTH 150
#define DOT_ANIMATION_DELAY 70

#define CENTER_ANIMATION_LENGTH 400
#define CENTER_ANIMATION_DELAY 50

typedef enum {
  DotType_NEUTRAL_DOT = 0,
  DotType_HOUR_DOT = 1,
  DotType_MINUTE_DOT = 2,
  DotType_HOUR_MINUTE_DOT = 3
} DotType;

typedef struct {
  uint32_t ordinal;
  uint32_t distance;
  int32_t angle;
  DotType dot_type;
} DotState;

typedef struct {
  uint32_t center_radius;
} CenterState;

typedef struct {
  GColor center_color;
  GColor hour_dot_color;
  GColor minute_dot_color;
  GColor neutral_dot_color;
} ColorConfig;

typedef struct {
  struct tm current_time;
  ColorConfig color_config;
  CenterState center_state;
  DotState dot_states[NUM_DOTS_TOTAL];
} ClockState;

void watch_model_start_intro(void);
void watch_model_init(void);

void watch_model_handle_center_change(const CenterState *state);
void watch_model_handle_dot_change(const DotState *state);
void watch_model_handle_time_change(const struct tm *tick_time);
void watch_model_handle_color_config_change(const ColorConfig *config);

DotType get_dot_type(uint32_t ordinal, const struct tm *tick_time);
