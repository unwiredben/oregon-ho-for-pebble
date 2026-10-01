/*
 * Copyright (c) 2026 Ben Combee
 * SPDX-License-Identifier: MIT
 * See LICENSE in the repository root for the full license text.
 */

#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TRAIL_FRAME_INTERVAL_MS 500
#define TRAIL_LANDMARK_COUNT 16

typedef enum {
  SCENERY_FORT, SCENERY_RIVER, SCENERY_CHIMNEY_ROCK, SCENERY_ROUND_ROCK,
  SCENERY_MOUNTAINS, SCENERY_SPRINGS, SCENERY_DALLES
} TrailScenery;

typedef struct {
  const char *name;
  const char *label;
  TrailScenery scenery;
} TrailLandmark;

extern const TrailLandmark TRAIL_LANDMARKS[TRAIL_LANDMARK_COUNT];

typedef struct {
  uint8_t frame, grass, landmark;
  bool landmark_visible;
  uint16_t landmark_progress;
  uint16_t passage_tick, event_wait, event_remaining;
  uint32_t random;
  char message[64];
} Trail;

void trail_init(Trail *trail, uint32_t seed);
void trail_step(Trail *trail);
const char *trail_event(const Trail *trail);
void trail_format_time(char *buffer, size_t size, int hour, int minute, bool use_24h);
