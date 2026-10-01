/* Minimal host declarations for the Pebble boundaries used by oregon-ho.c.
 * Copyright (c) 2026 Ben Combee. SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef struct Window Window;
typedef struct Layer Layer;
typedef struct AppTimer AppTimer;
typedef struct GContext GContext;
typedef void *GFont;
typedef struct { int16_t x, y; } GPoint;
typedef struct { int16_t w, h; } GSize;
typedef struct { GPoint origin; GSize size; } GRect;
typedef enum { ACCEL_AXIS_X, ACCEL_AXIS_Y, ACCEL_AXIS_Z } AccelAxisType;
typedef enum { MINUTE_UNIT = 2 } TimeUnits;
typedef void (*AppTimerCallback)(void *);
typedef struct {
  void (*load)(Window *);
  void (*unload)(Window *);
} WindowHandlers;
#define GColorBlack 0
#define FONT_KEY_GOTHIC_28 "28"
#define FONT_KEY_GOTHIC_18 "18"
#define APP_LOG(level, ...) ((void)0)
#define APP_LOG_LEVEL_ERROR 1

AppTimer *app_timer_register(uint32_t, AppTimerCallback, void *);
void app_timer_cancel(AppTimer *);
void layer_mark_dirty(Layer *);
bool clock_is_24h_style(void);
GRect layer_get_bounds(const Layer *);
GRect layer_get_frame(const Layer *);
Layer *window_get_root_layer(const Window *);
Layer *layer_create(GRect);
GFont fonts_get_system_font(const char *);
void layer_set_update_proc(Layer *, void (*)(Layer *, GContext *));
void layer_add_child(Layer *, Layer *);
void layer_destroy(Layer *);
Window *window_create(void);
void window_set_background_color(Window *, int);
void window_set_window_handlers(Window *, WindowHandlers);
void tick_timer_service_subscribe(TimeUnits, void (*)(struct tm *, TimeUnits));
void app_focus_service_subscribe(void (*)(bool));
void accel_tap_service_subscribe(void (*)(AccelAxisType, int32_t));
void window_stack_push(Window *, bool);
void app_event_loop(void);
void app_focus_service_unsubscribe(void);
void accel_tap_service_unsubscribe(void);
void tick_timer_service_unsubscribe(void);
void window_destroy(Window *);
