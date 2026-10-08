/* Copyright (c) 2026 Ben Combee. SPDX-License-Identifier: MIT */
#pragma once
#include "trail.h"

/* Restore settings, register callbacks, then open AppMessage buffers. */
void settings_init(Trail *trail, void (*changed)(void));
void settings_deinit(void);
