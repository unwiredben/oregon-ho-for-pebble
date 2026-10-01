/*
 * Copyright (c) 2026 Ben Combee
 * SPDX-License-Identifier: MIT
 * See LICENSE in the repository root for the full license text.
 */

#include "trail.h"
#include <stdio.h>
#include <string.h>

const TrailLandmark TRAIL_LANDMARKS[TRAIL_LANDMARK_COUNT] = {
  {"Kansas River", "KANSAS RIVER", SCENERY_RIVER},
  {"Big Blue River", "BIG BLUE R.", SCENERY_RIVER},
  {"Fort Kearney", "FT. KEARNEY", SCENERY_FORT},
  {"Chimney Rock", "CHIMNEY ROCK", SCENERY_CHIMNEY_ROCK},
  {"Fort Laramie", "FORT LARAMIE", SCENERY_FORT},
  {"Independence Rock", "INDEP. ROCK", SCENERY_ROUND_ROCK},
  {"South Pass", "SOUTH PASS", SCENERY_MOUNTAINS},
  {"Green River", "GREEN RIVER", SCENERY_RIVER},
  {"Fort Bridger", "FT. BRIDGER", SCENERY_FORT},
  {"Soda Springs", "SODA SPRINGS", SCENERY_SPRINGS},
  {"Fort Hall", "FORT HALL", SCENERY_FORT},
  {"Snake River", "SNAKE RIVER", SCENERY_RIVER},
  {"Fort Boise", "FORT BOISE", SCENERY_FORT},
  {"Blue Mountains", "BLUE MTNS.", SCENERY_MOUNTAINS},
  {"Fort Walla Walla", "WALLA WALLA", SCENERY_FORT},
  {"The Dalles", "THE DALLES", SCENERY_DALLES}
};

static uint32_t next_random(Trail *trail) {
  trail->random = trail->random * 1664525u + 1013904223u;
  return trail->random;
}

void trail_init(Trail *trail, uint32_t seed) {
  memset(trail, 0, sizeof(*trail));
  trail->random = seed;
  trail->event_wait = 24000 / TRAIL_FRAME_INTERVAL_MS;
}

void trail_step(Trail *trail) {
  trail->frame = (trail->frame + 1) % 4;
  trail->grass = (trail->grass + 1) % 32;
  trail->passage_tick = (trail->passage_tick + 1) % 320;
  if (trail->passage_tick == 0) {
    trail->landmark = (trail->landmark + 1) % TRAIL_LANDMARK_COUNT;
  }
  trail->landmark_visible = trail->passage_tick >= 24 && trail->passage_tick < 168;
  trail->landmark_progress = trail->landmark_visible
      ? (trail->passage_tick - 24) * 256 / 143 : 0;

  if (trail->event_remaining) {
    --trail->event_remaining;
  } else if (trail->event_wait) {
    --trail->event_wait;
  } else {
    static const char *const names[] = {"ALICE", "BEN", "MARY", "JAMES", "SARAH", "SAM"};
    static const char *const events[] = {
      "%s HAS A FEVER.", "%s BROKE AN ARM.", "%s HAS DYSENTERY.",
      "%s BROKE A LEG.", "%s HAS EXHAUSTION.", "%s IS WELL AGAIN.",
      "%s LOST AN OX.", "%s FOUND WILD FRUIT.", "%s SHOT A BEAR.",
      "%s HAS CHOLERA.", "%s HAD A BAD DREAM.", "%s SHOT A DEER.",
      "%s SHOT A SQUIRREL.", "%s SHOT A MOOSE.", "%s SHOT A RABBIT.",
      "%s SHOT A BUFFALO.", "%s FOUND MUSHROOMS.", "%s GOT CONSUMPTION."
    };
    unsigned name = (next_random(trail) >> 16) % (sizeof(names) / sizeof(names[0]));
    unsigned event = (next_random(trail) >> 16) % (sizeof(events) / sizeof(events[0]));
    snprintf(trail->message, sizeof(trail->message), events[event], names[name]);
    trail->event_remaining = 12000 / TRAIL_FRAME_INTERVAL_MS;
    const unsigned quiet_ticks = 40000 / TRAIL_FRAME_INTERVAL_MS;
    trail->event_wait = quiet_ticks + (next_random(trail) >> 16) % quiet_ticks;
  }
}

const char *trail_event(const Trail *trail) {
  return trail->event_remaining ? trail->message : NULL;
}

void trail_format_time(char *buffer, size_t size, int hour, int minute, bool use_24h) {
  if (!use_24h) {
    hour %= 12;
    if (hour == 0) hour = 12;
  }
  snprintf(buffer, size, use_24h ? "%02d:%02d" : "%d:%02d", hour, minute);
}
