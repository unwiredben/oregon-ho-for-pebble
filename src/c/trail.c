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
  trail_resume_animation(trail);
}

void trail_resume_animation(Trail *trail) {
  trail->animation_remaining = 60000 / TRAIL_FRAME_INTERVAL_MS;
}

bool trail_animation_finished(const Trail *trail) {
  return !trail->animation_remaining && !trail->landmark_visible &&
      !trail->event_remaining;
}

void trail_step(Trail *trail) {
  if (trail->animation_remaining) --trail->animation_remaining;
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
    trail_refresh_message(trail);
  }
}

static const struct { const char *format; bool shooting; } EVENTS[] = {
    {"%s HAS A FEVER.", false}, {"%s BROKE AN ARM.", false}, {"%s HAS DYSENTERY.", false},
    {"%s BROKE A LEG.", false}, {"%s HAS EXHAUSTION.", false}, {"%s IS WELL AGAIN.", false},
    {"%s LOST AN OX.", false}, {"%s FOUND WILD FRUIT.", false}, {"%s SHOT A BEAR.", true},
    {"%s HAS CHOLERA.", false}, {"%s HAD A BAD DREAM.", false}, {"%s SHOT A DEER.", true},
    {"%s SHOT A SQUIRREL.", true}, {"%s SHOT A MOOSE.", true}, {"%s SHOT A RABBIT.", true},
    {"%s SHOT A BUFFALO.", true}, {"%s FOUND MUSHROOMS.", false}, {"%s GOT CONSUMPTION.", false},
    {"%s PETTED A DOG.", false}, {"%s GOT THE SNIFFLES.", false}, {"%s SPIED A HAWK.", false},
    {"%s SPIED AN EAGLE.", false}, {"%s SAW A RAINBOW.", false}, {"%s MADE A FRIEND.", false}
  };

/* Return the largest whole UTF-8 prefix within the requested byte budget. */
static size_t name_prefix(const char *name, size_t bytes) {
  size_t length = strlen(name);
  if (bytes >= length) return length;
  while (bytes && ((unsigned char)name[bytes] & 0xc0) == 0x80) --bytes;
  return bytes;
}

/* ECMAScript whitespace, matching the Clay page's trim behavior. Non-ASCII
 * spaces become ordinary spaces so blank-only names restore the default pool. */
static size_t unicode_space_bytes(const char *text) {
  const unsigned char *c = (const unsigned char *)text;
  if (c[0] == 0xc2 && c[1] == 0xa0) return 2;
  if (c[0] == 0xe1 && c[1] == 0x9a && c[2] == 0x80) return 3;
  if (c[0] == 0xe2 && c[1] == 0x80 &&
      ((c[2] >= 0x80 && c[2] <= 0x8a) || c[2] == 0xa8 || c[2] == 0xa9 || c[2] == 0xaf)) return 3;
  if (c[0] == 0xe2 && c[1] == 0x81 && c[2] == 0x9f) return 3;
  if (c[0] == 0xe3 && c[1] == 0x80 && c[2] == 0x80) return 3;
  if (c[0] == 0xef && c[1] == 0xbb && c[2] == 0xbf) return 3;
  return 0;
}

void trail_set_name(Trail *trail, unsigned slot, const char *name) {
  if (slot >= TRAIL_PARTY_SIZE || !name) return;
  while (*name) {
    size_t space = unicode_space_bytes(name);
    if ((unsigned char)*name <= ' ') ++name;
    else if (space) name += space;
    else break;
  }
  size_t limit = name_prefix(name, TRAIL_NAME_BYTES);
  size_t used = 0;
  for (size_t i = 0; i < limit; ++i) {
    size_t space = unicode_space_bytes(name + i);
    if (space) {
      trail->names[slot][used++] = ' ';
      i += space - 1;
      continue;
    }
    unsigned char ch = (unsigned char)name[i];
    if (ch < ' ' || ch == 127) continue;
    trail->names[slot][used++] = ch >= 'a' && ch <= 'z' ? ch - ('a' - 'A') : ch;
  }
  while (used && trail->names[slot][used - 1] == ' ') --used;
  trail->names[slot][used] = '\0';
}

static void format_name(const Trail *trail, char *name, size_t name_bytes) {
  size_t used = name_prefix(trail->event_name, name_bytes);
  memcpy(name, trail->event_name, used);
  name[used] = '\0';
  if (used < strlen(trail->event_name)) strcpy(name + used, "...");
}

void trail_format_event(const Trail *trail, char *buffer, size_t size,
                        size_t name_bytes) {
  char name[TRAIL_NAME_BYTES + 4];
  format_name(trail, name, name_bytes);
  snprintf(buffer, size, EVENTS[trail->event_id].format, name);
}

bool trail_fit_event(const Trail *trail, char *buffer, size_t size,
    bool (*fits)(const char *, const char *, void *), void *context) {
  size_t bytes = strlen(trail->event_name);
  do {
    char name[TRAIL_NAME_BYTES + 4];
    format_name(trail, name, bytes);
    trail_format_event(trail, buffer, size, bytes);
    if (fits(buffer, name, context)) return true;
    if (!bytes) break;
    bytes = name_prefix(trail->event_name, bytes - 1);
  } while (true);
  return false;
}

void trail_refresh_message(Trail *trail) {
  static const char *const names[] = {
    "ALICE", "BEN", "MARY", "JAMES", "SARAH", "SAM",
    "ANNE", "SKYE", "ERIC", "ELI", "NAT", "LEO"
  };
  const char *party[TRAIL_PARTY_SIZE];
  unsigned party_size = 0;
  for (unsigned i = 0; i < TRAIL_PARTY_SIZE; ++i) {
    if (trail->names[i][0]) party[party_size++] = trail->names[i];
  }
  unsigned name = next_random(trail) >> 16;
  const char *selected = party_size ? party[name % party_size]
      : names[name % (sizeof(names) / sizeof(names[0]))];
  strcpy(trail->event_name, selected);

  unsigned eligible[sizeof(EVENTS) / sizeof(EVENTS[0])];
  unsigned count = 0;
  for (unsigned i = 0; i < sizeof(EVENTS) / sizeof(EVENTS[0]); ++i) {
    if (!trail->no_guns || !EVENTS[i].shooting) eligible[count++] = i;
  }
  trail->event_id = eligible[(next_random(trail) >> 16) % count];
  trail_format_event(trail, trail->message, sizeof(trail->message), TRAIL_NAME_BYTES);
  trail->event_remaining = 12000 / TRAIL_FRAME_INTERVAL_MS;
  const unsigned quiet_ticks = 40000 / TRAIL_FRAME_INTERVAL_MS;
  trail->event_wait = quiet_ticks + (next_random(trail) >> 16) % quiet_ticks;
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
