#include <pebble.h>
#include "scene.h"
#include "trail.h"

static Window *s_window;
static Layer *s_canvas;
static AppTimer *s_timer;
static GFont s_status_font;
static bool s_focused = true;
static Trail s_trail;
static char s_time[6], s_date[20];

static void update_time(void) {
  time_t now = time(NULL);
  struct tm *local = localtime(&now);
  trail_format_time(s_time, sizeof(s_time), local->tm_hour, local->tm_min,
                    clock_is_24h_style());
  strftime(s_date, sizeof(s_date), "%a %b %d", local);
  for (char *c = s_date; *c; ++c) {
    if (*c >= 'a' && *c <= 'z') *c -= 'a' - 'A';
  }
  if (s_canvas) layer_mark_dirty(s_canvas);
}

static void draw(Layer *layer, GContext *ctx) {
  scene_draw(ctx, layer_get_bounds(layer), &s_trail, s_time, s_date, s_status_font,
      layer_get_frame(window_get_root_layer(s_window)).origin);
}

static void animate(void *context) {
  (void)context;
  s_timer = NULL;
  if (!s_focused || !s_canvas) return;
  trail_step(&s_trail);
  layer_mark_dirty(s_canvas);
  s_timer = app_timer_register(TRAIL_FRAME_INTERVAL_MS, animate, NULL);
}

static void focus_changed(bool focused) {
  s_focused = focused;
  if (!focused && s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  } else if (focused && s_canvas && !s_timer) {
    update_time();
    s_timer = app_timer_register(TRAIL_FRAME_INTERVAL_MS, animate, NULL);
  }
}

static void tick(struct tm *tick_time, TimeUnits units_changed) {
  (void)tick_time;
  (void)units_changed;
  update_time();
}

static void load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_canvas = layer_create(layer_get_bounds(root));
  if (!s_canvas) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Unable to allocate watchface canvas");
    return;
  }
  s_status_font = fonts_get_system_font(layer_get_bounds(root).size.w >= 200
      ? FONT_KEY_GOTHIC_28 : FONT_KEY_GOTHIC_18);
  scene_init(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, draw);
  layer_add_child(root, s_canvas);
  update_time();
  if (s_focused) s_timer = app_timer_register(TRAIL_FRAME_INTERVAL_MS, animate, NULL);
}

static void unload(Window *window) {
  (void)window;
  scene_deinit();
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
  if (s_canvas) {
    layer_destroy(s_canvas);
    s_canvas = NULL;
  }
  if (s_status_font) {
    s_status_font = NULL;
  }
}

int main(void) {
  trail_init(&s_trail, (uint32_t)time(NULL));
  s_window = window_create();
  if (!s_window) return 1;
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers){.load = load, .unload = unload});
  tick_timer_service_subscribe(MINUTE_UNIT, tick);
  app_focus_service_subscribe(focus_changed);
  window_stack_push(s_window, false);
  app_event_loop();
  app_focus_service_unsubscribe();
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
  return 0;
}
