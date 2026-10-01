#include "trail.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
  Trail trail;
  static const char *const route[] = {
    "Kansas River", "Big Blue River",
    "Fort Kearney", "Chimney Rock", "Fort Laramie", "Independence Rock",
    "South Pass", "Green River", "Fort Bridger", "Soda Springs",
    "Fort Hall", "Snake River", "Fort Boise", "Blue Mountains",
    "Fort Walla Walla", "The Dalles"
  };
  assert(TRAIL_LANDMARK_COUNT == sizeof(route) / sizeof(route[0]));
  trail_init(&trail, 123);
  for (unsigned stop = 0; stop <= TRAIL_LANDMARK_COUNT; ++stop) {
    unsigned index = stop % TRAIL_LANDMARK_COUNT;
    assert(trail.landmark == index);
    assert(strcmp(TRAIL_LANDMARKS[index].name, route[index]) == 0);
    assert(TRAIL_LANDMARKS[index].label[0]);
    for (unsigned tick = 0; tick < 320; ++tick) trail_step(&trail);
  }
  assert(TRAIL_FRAME_INTERVAL_MS == 500);
  trail_init(&trail, 123);
  for (unsigned i = 0; i < 24000 / TRAIL_FRAME_INTERVAL_MS; ++i) {
    trail_step(&trail);
    assert(trail_event(&trail) == NULL);
  }
  trail_step(&trail);
  assert(trail_event(&trail) != NULL);
  assert(trail.event_remaining * TRAIL_FRAME_INTERVAL_MS == 12000);
  assert(trail.event_wait * TRAIL_FRAME_INTERVAL_MS >= 40000);
  assert(trail.event_wait * TRAIL_FRAME_INTERVAL_MS < 80000);
  for (unsigned i = 1; i < 12000 / TRAIL_FRAME_INTERVAL_MS; ++i) {
    trail_step(&trail);
    assert(trail_event(&trail) != NULL);
  }
  trail_step(&trail);
  assert(trail_event(&trail) == NULL);
  trail_init(&trail, 123);
  assert(trail_event(&trail) == NULL);
  bool saw_landmark = false, saw_event = false, saw_quiet_after_event = false;
  unsigned kinds = 0;
  unsigned new_events = 0;
  unsigned more_events = 0;
  static const char *const more_event_suffixes[] = {
    " SHOT A DEER.", " SHOT A SQUIRREL.", " SHOT A MOOSE.",
    " SHOT A RABBIT.", " SHOT A BUFFALO.", " FOUND MUSHROOMS.",
    " GOT CONSUMPTION."
  };
  for (unsigned i = 0; i < 100000; ++i) {
    trail_step(&trail);
    assert(trail.frame < 4);
    assert(trail.grass < 32);
    if (trail.landmark_visible) {
      saw_landmark = true;
      kinds |= 1u << trail.landmark;
      assert(trail.landmark_progress <= 256);
    }
    if (trail_event(&trail)) {
      saw_event = true;
      assert(strlen(trail_event(&trail)) < 64);
      if (strstr(trail_event(&trail), " SHOT A BEAR.")) new_events |= 1;
      if (strstr(trail_event(&trail), " HAS CHOLERA.")) new_events |= 2;
      if (strstr(trail_event(&trail), " HAD A BAD DREAM.")) new_events |= 4;
      for (unsigned event = 0; event < sizeof(more_event_suffixes) / sizeof(more_event_suffixes[0]); ++event) {
        if (strstr(trail_event(&trail), more_event_suffixes[event])) more_events |= 1u << event;
      }
    } else if (saw_event) {
      saw_quiet_after_event = true;
    }
  }
  assert(saw_landmark && saw_event && saw_quiet_after_event);
  assert(kinds == (1u << TRAIL_LANDMARK_COUNT) - 1);
  assert(new_events == 7);
  assert(more_events == 127);
  char time[6];
  trail_format_time(time, sizeof(time), 0, 5, false);
  assert(strcmp(time, "12:05") == 0);
  trail_format_time(time, sizeof(time), 12, 0, false);
  assert(strcmp(time, "12:00") == 0);
  trail_format_time(time, sizeof(time), 23, 59, false);
  assert(strcmp(time, "11:59") == 0);
  trail_format_time(time, sizeof(time), 0, 5, true);
  assert(strcmp(time, "00:05") == 0);
  trail_format_time(time, sizeof(time), 23, 59, true);
  assert(strcmp(time, "23:59") == 0);
  puts("Travel cycles, transient events, all landmarks and time formats passed.");
}
