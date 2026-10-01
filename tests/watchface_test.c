/* Copyright (c) 2026 Ben Combee. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#define main watchface_main
#include "../src/c/oregon-ho.c"
#undef main

/* Only the SDK timer and drawing boundaries are faked; lifecycle and Trail
 * behavior run unchanged. Unused SDK/window code is discarded by the linker. */
static unsigned timer_registrations, redraws;
static bool timer_pending;
AppTimer *app_timer_register(uint32_t timeout, AppTimerCallback callback, void *data) {
  assert(timeout == 500 && callback == animate && data == NULL);
  assert(!timer_pending);
  timer_pending = true;
  ++timer_registrations;
  return (AppTimer *)1;
}
void app_timer_cancel(AppTimer *timer) {
  assert(timer == (AppTimer *)1 && timer_pending);
  timer_pending = false;
}
void layer_mark_dirty(Layer *layer) {
  assert(layer == s_canvas);
  ++redraws;
}
bool clock_is_24h_style(void) { return true; }

static void fire_frame(void) {
  assert(timer_pending);
  timer_pending = false;
  animate(NULL);
}

int main(void) {
  trail_init(&s_trail, 123);
  s_canvas = (Layer *)1;
  focus_changed(true);
  assert(timer_pending);
  for (unsigned i = 0; i < 119; ++i) fire_frame();
  assert(!s_animation_stopped);

  /* Focus preserves the remaining window rather than renewing it. */
  focus_changed(false);
  assert(!timer_pending && s_timer == NULL);
  assert(s_trail.animation_remaining == 1);
  focus_changed(true);
  assert(s_trail.animation_remaining == 1);
  fire_frame();
  assert(s_trail.landmark_visible && timer_pending);
  for (unsigned i = 0; i < 500 && timer_pending; ++i) fire_frame();
  assert(s_animation_stopped && s_timer == NULL);
  assert(!s_trail.landmark_visible && trail_event(&s_trail) == NULL);

  unsigned registrations = timer_registrations;
  unsigned old_redraws = redraws;
  Trail frozen = s_trail;
  tick(NULL, MINUTE_UNIT);
  assert(redraws > old_redraws && s_time[0]);
  assert(trail_event(&s_trail) != NULL);
  assert(s_trail.random != frozen.random);
  assert(s_trail.frame == frozen.frame && s_trail.grass == frozen.grass);
  assert(s_trail.passage_tick == frozen.passage_tick);
  assert(!timer_pending && timer_registrations == registrations);
  focus_changed(false);
  tapped(ACCEL_AXIS_X, 1);
  assert(!timer_pending);
  focus_changed(true);
  assert(s_animation_stopped && !timer_pending);
  assert(timer_registrations == registrations);

  tapped(ACCEL_AXIS_Y, -1);
  assert(!s_animation_stopped && timer_pending);
  assert(timer_registrations == registrations + 1);
  fire_frame();
  assert(s_trail.animation_remaining == 119);
  registrations = timer_registrations;
  tapped(ACCEL_AXIS_Z, 1);
  assert(s_trail.animation_remaining == 120);
  assert(timer_registrations == registrations);
  uint32_t random = s_trail.random;
  tick(NULL, MINUTE_UNIT);
  assert(s_trail.random == random && timer_registrations == registrations);
  focus_changed(false);
  assert(!timer_pending);
  puts("Animation cutoff, focus persistence, idle ticks and tap renewal passed.");
}
