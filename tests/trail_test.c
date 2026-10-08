/*
 * Copyright (c) 2026 Ben Combee
 * SPDX-License-Identifier: MIT
 * See LICENSE in the repository root for the full license text.
 */

#include "trail.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bool fits_seven_columns(const char *message, const char *name, void *context) {
  unsigned *calls = context;
  ++*calls;
  assert(strstr(message, name) == message);
  return strlen(name) <= 7;
}

static void test_display_fit(void) {
  Trail trail;
  trail_init(&trail, 123);
  trail_set_name(&trail, 0, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  trail_refresh_message(&trail);
  char fitted[TRAIL_MESSAGE_BYTES];
  unsigned calls = 0;
  assert(trail_fit_event(&trail, fitted, sizeof(fitted), fits_seven_columns, &calls));
  assert(strncmp(fitted, "ABCD... ", 8) == 0);
  assert(strcmp(fitted + 7, strchr(trail.message, ' ')) == 0);
  assert(strcmp(trail.names[0], "ABCDEFGHIJKLMNOPQRSTUVWXYZ") == 0);
  assert(calls > 1);
}

static void test_party_settings(void) {
  Trail trail;
  trail_init(&trail, 123);
  trail_set_name(&trail, 2, "  Robin  ");
  assert(strcmp(trail.names[2], "ROBIN") == 0);
  for (unsigned i = 0; i < 1000; ++i) {
    trail_refresh_message(&trail);
    assert(strncmp(trail.message, "ROBIN ", 6) == 0);
  }
  trail_set_name(&trail, 4, "alex\n\t");
  bool robin = false, alex = false;
  for (unsigned i = 0; i < 1000; ++i) {
    trail_refresh_message(&trail);
    robin |= strncmp(trail.message, "ROBIN ", 6) == 0;
    alex |= strncmp(trail.message, "ALEX ", 5) == 0;
    assert(strncmp(trail.message, "ROBIN ", 6) == 0 ||
           strncmp(trail.message, "ALEX ", 5) == 0);
  }
  assert(robin && alex);
  trail_set_name(&trail, 2, " \t\r\n ");
  trail_set_name(&trail, 4, "");
  trail_refresh_message(&trail);
  assert(strncmp(trail.message, "ROBIN ", 6) != 0);
  assert(strncmp(trail.message, "ALEX ", 5) != 0);
  trail_set_name(&trail, 0, "\xc2\xa0\xe3\x80\x80");
  assert(trail.names[0][0] == '\0');
  trail_set_name(&trail, 0, "\xe3\x80\x80Robin\xc2\xa0");
  assert(strcmp(trail.names[0], "ROBIN") == 0);

  char long_name[200];
  memset(long_name, 'W', sizeof(long_name) - 1);
  long_name[sizeof(long_name) - 1] = '\0';
  trail_set_name(&trail, 0, long_name);
  assert(strlen(trail.names[0]) == TRAIL_NAME_BYTES);
  /* A multi-byte character crossing the storage bound must be omitted whole. */
  memset(long_name, 'A', TRAIL_NAME_BYTES - 1);
  strcpy(long_name + TRAIL_NAME_BYTES - 1, "é");
  trail_set_name(&trail, 0, long_name);
  assert(strlen(trail.names[0]) == TRAIL_NAME_BYTES - 1);
  trail_set_name(&trail, 0, "Élodie");
  trail_refresh_message(&trail);
  assert(strncmp(trail.message, "ÉLODIE ", strlen("ÉLODIE ")) == 0);
  char fitted[TRAIL_MESSAGE_BYTES];
  trail_format_event(&trail, fitted, sizeof(fitted), 1);
  assert(strncmp(fitted, "... ", 4) == 0);
  const char *suffix = strchr(trail.message, ' ');
  assert(strcmp(fitted + 3, suffix) == 0);
  trail_format_event(&trail, fitted, sizeof(fitted), strlen("ÉLO"));
  assert(strncmp(fitted, "ÉLO... ", strlen("ÉLO... ")) == 0);
  assert(strcmp(fitted + strlen("ÉLO..."), suffix) == 0);

  trail.no_guns = true;
  bool seen[24] = {false};
  unsigned count = 0;
  for (unsigned i = 0; i < 10000; ++i) {
    trail_refresh_message(&trail);
    assert(strstr(trail.message, " SHOT ") == NULL);
    if (!seen[trail.event_id]) { seen[trail.event_id] = true; ++count; }
  }
  assert(count == 18);
  trail.no_guns = false;
  bool saw_shooting = false;
  for (unsigned i = 0; i < 1000; ++i) {
    trail_refresh_message(&trail);
    saw_shooting |= strstr(trail.message, " SHOT ") != NULL;
  }
  assert(saw_shooting);
}

static void test_animation_idle(void) {
  Trail trail;
  trail_init(&trail, 123);
  assert(!trail_animation_finished(&trail));
  for (unsigned i = 0; i < 119; ++i) {
    trail_step(&trail);
    assert(!trail_animation_finished(&trail));
  }
  trail_step(&trail);
  /* The minute is over, but the first landmark still needs to pass. */
  assert(trail.landmark_visible);
  assert(!trail_animation_finished(&trail));
  for (unsigned i = 0; i < 500 && !trail_animation_finished(&trail); ++i) {
    trail_step(&trail);
  }
  assert(trail_animation_finished(&trail));
  assert(!trail.landmark_visible);
  assert(trail_event(&trail) == NULL);

  /* An event also prevents stopping even when there is no landmark. */
  trail.event_remaining = 2;
  assert(!trail_animation_finished(&trail));
  trail_step(&trail);
  assert(!trail_animation_finished(&trail));
  trail_step(&trail);
  assert(trail_animation_finished(&trail));

  Trail frozen = trail;
  trail_refresh_message(&trail);
  assert(trail_event(&trail) != NULL);
  assert(trail.random != frozen.random);
  assert(trail.frame == frozen.frame && trail.grass == frozen.grass);
  assert(trail.passage_tick == frozen.passage_tick);
  assert(trail.landmark == frozen.landmark);
  assert(trail.landmark_progress == frozen.landmark_progress);
  char message[64];
  strcpy(message, trail_event(&trail));
  trail_refresh_message(&trail);
  assert(strcmp(message, trail_event(&trail)) != 0);

  trail_resume_animation(&trail);
  assert(!trail_animation_finished(&trail));
  for (unsigned i = 0; i < 119; ++i) {
    trail_step(&trail);
    assert(!trail_animation_finished(&trail));
  }
  /* Repeated taps renew the full window, rather than adding extra timers. */
  trail_resume_animation(&trail);
  for (unsigned i = 0; i < 119; ++i) {
    trail_step(&trail);
    assert(!trail_animation_finished(&trail));
  }
  trail_step(&trail);
  trail.landmark_visible = false;
  trail.event_remaining = 0;
  assert(trail_animation_finished(&trail));
}

int main(void) {
  test_display_fit();
  test_party_settings();
  test_animation_idle();
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
