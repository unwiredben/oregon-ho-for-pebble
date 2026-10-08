/* Copyright (c) 2026 Ben Combee. SPDX-License-Identifier: MIT */
#include "event_layout.h"

typedef struct {
  bool (*fits)(const char *, const char *, unsigned, void *);
  void *context;
} SmallFontFit;

static bool fits_small_font(const char *message, const char *name, void *context) {
  const SmallFontFit *fit = context;
  return fit->fits(message, name, 14, fit->context);
}

unsigned event_layout_fit(const Trail *trail, char *message, size_t size,
    unsigned preferred_font,
    bool (*fits)(const char *, const char *, unsigned, void *), void *context) {
  static const unsigned fonts[] = {28, 18, 14};
  trail_format_event(trail, message, size, TRAIL_NAME_BYTES);
  for (unsigned i = 0; i < sizeof(fonts) / sizeof(fonts[0]); ++i) {
    if (fonts[i] <= preferred_font && fits(message, trail->event_name, fonts[i], context)) {
      return fonts[i];
    }
  }
  SmallFontFit small = {fits, context};
  return trail_fit_event(trail, message, size, fits_small_font, &small) ? 14 : 0;
}
