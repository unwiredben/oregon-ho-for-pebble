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
#define TRAIL_PARTY_SIZE 5
#define TRAIL_NAME_BYTES 96
#define TRAIL_MESSAGE_BYTES 160

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
  uint16_t animation_remaining;
  uint32_t random;
  char names[TRAIL_PARTY_SIZE][TRAIL_NAME_BYTES + 1];
  bool no_guns;
  uint8_t event_id;
  char event_name[TRAIL_NAME_BYTES + 1];
  char message[TRAIL_MESSAGE_BYTES];
} Trail;

void trail_init(Trail *trail, uint32_t seed);
void trail_step(Trail *trail);
/* A fresh minute of animation; focus changes preserve the remaining budget. */
void trail_resume_animation(Trail *trail);
/* The budget is exhausted and neither scenery nor a transient event is visible. */
bool trail_animation_finished(const Trail *trail);
/* Replace the story without advancing any animation or scenery state. */
void trail_refresh_message(Trail *trail);
/* Trim whitespace/controls, uppercase ASCII, and bound at a UTF-8 boundary. */
void trail_set_name(Trail *trail, unsigned slot, const char *name);
/* Shorten only the selected name, preserving the full event suffix. */
void trail_format_event(const Trail *trail, char *buffer, size_t size,
                        size_t name_bytes);
/* Try full name, then progressively shorter UTF-8 prefixes plus ellipsis.
 * Return false if even an ellipsis cannot fit; buffer still contains the story. */
bool trail_fit_event(const Trail *trail, char *buffer, size_t size,
    bool (*fits)(const char *message, const char *name, void *context), void *context);
const char *trail_event(const Trail *trail);
void trail_format_time(char *buffer, size_t size, int hour, int minute, bool use_24h);
