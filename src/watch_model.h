/*
    Copyright (C) 2016 Pebble Technology Corp.

    This file is part of Revolver.

    Revolver is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Revolver.  If not, see <http://www.gnu.org/licenses/>.

*/

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
