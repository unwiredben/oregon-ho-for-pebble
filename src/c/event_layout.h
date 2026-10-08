/* Copyright (c) 2026 Ben Combee. SPDX-License-Identifier: MIT */
#pragma once
#include "trail.h"

/* Try the whole event at progressively smaller fonts (28, 18, 14), then
 * shorten only the name at 14. Returns the chosen font size, or zero if none fit. */
unsigned event_layout_fit(const Trail *trail, char *message, size_t size,
    unsigned preferred_font,
    bool (*fits)(const char *message, const char *name, unsigned font, void *context),
    void *context);
