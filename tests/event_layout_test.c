/* Copyright (c) 2026 Ben Combee. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "event_layout.h"

/* Font measurement is supplied by the SDK at runtime. These fixed capacities
 * catch shortening at a larger font before trying a smaller font with full text. */
static bool measured_fit(const char *message, const char *name, unsigned font, void *context) {
  (void)context;
  assert(strncmp(message, name, strlen(name)) == 0);
  size_t capacity = font == 28 ? 7 : font == 18 ? 14 : 20;
  return strlen(name) <= capacity;
}

int main(void) {
  Trail trail;
  char message[TRAIL_MESSAGE_BYTES];
  trail_init(&trail, 123);
  trail_set_name(&trail, 0, "Alexanderson");
  trail_refresh_message(&trail);
  assert(event_layout_fit(&trail, message, sizeof(message), 28, measured_fit, NULL) == 18);
  assert(strcmp(message, trail.message) == 0);
  trail_set_name(&trail, 0, "Alexanderson Smith");
  trail_refresh_message(&trail);
  assert(event_layout_fit(&trail, message, sizeof(message), 28, measured_fit, NULL) == 14);
  assert(strcmp(message, trail.message) == 0);
  trail_set_name(&trail, 0, "WWWWWWWWWWWWWWWWWWWWWWWW");
  trail_refresh_message(&trail);
  assert(event_layout_fit(&trail, message, sizeof(message), 28, measured_fit, NULL) == 14);
  assert(strncmp(message, "WWWWWWWWWWWWWWWWW... ", 21) == 0);
  assert(strcmp(message + 20, strchr(trail.message, ' ')) == 0);
  trail_set_name(&trail, 0, "Ben");
  trail_refresh_message(&trail);
  assert(event_layout_fit(&trail, message, sizeof(message), 18, measured_fit, NULL) == 18);
  assert(strcmp(message, trail.message) == 0);
  puts("Event layout preserves whole messages by reducing font before shortening names.");
}
