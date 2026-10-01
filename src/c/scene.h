#pragma once
#include <pebble.h>
#include "trail.h"

void scene_init(GRect bounds);
void scene_deinit(void);
void scene_draw(GContext *ctx, GRect bounds, const Trail *trail,
                const char *time_text, const char *date_text, GFont status_font,
                GPoint window_origin);
