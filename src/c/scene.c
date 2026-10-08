/*
 * Copyright (c) 2026 Ben Combee
 * SPDX-License-Identifier: MIT
 * See LICENSE in the repository root for the full license text.
 */

#include "scene.h"
#include "event_layout.h"
#include <string.h>
#include <stdio.h>

static GBitmap *s_convoy[4], *s_clock, *s_panel, *s_landmark, *s_raster;
static int s_landmark_scenery = -1;
static GColor s_palette[16];
static char s_clock_time[6], s_clock_date[20], s_status[TRAIL_MESSAGE_BYTES];
static char s_fitted_message[TRAIL_MESSAGE_BYTES];
static char s_measured_word[TRAIL_NAME_BYTES + 4];
static GRect s_panel_rect;
static bool s_panel_valid;

// Render square-pixel artwork directly into a packed 4-bit offscreen bitmap.
// Shared palettes keep the four animation frames small on Basalt and Chalk.
static void raster_block(int x, int y, int w, int h, GColor color) {
  GRect bounds = gbitmap_get_bounds(s_raster);
  int left = x < 0 ? 0 : x, top = y < 0 ? 0 : y;
  int right = x + w < bounds.size.w ? x + w : bounds.size.w;
  int bottom = y + h < bounds.size.h ? y + h : bounds.size.h;
  unsigned index = 0;
  while (index < 15 && s_palette[index].argb != color.argb) ++index;
  uint8_t *data = gbitmap_get_data(s_raster);
  int stride = gbitmap_get_bytes_per_row(s_raster);
  for (int row = top; row < bottom; ++row) {
    uint8_t *pixels = data + row * stride;
    for (int col = left; col < right; ++col) {
      int shift = (col & 1) ? 0 : 4;
      pixels[col / 2] = (pixels[col / 2] & ~(15 << shift)) | (index << shift);
    }
  }
}

// Original 5x7 bitmap alphabet. All artwork uses square pixels.
static const uint8_t s_glyphs[][7] = {
  {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
  {14,17,1,2,4,8,31}, {30,1,1,14,1,1,30},
  {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
  {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8},
  {14,17,17,14,17,17,14}, {14,17,17,15,1,1,14},
  {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
  {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
  {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
  {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17},
  {14,4,4,4,4,4,14}, {7,2,2,2,18,18,12},
  {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
  {17,27,21,21,17,17,17}, {17,25,25,21,19,19,17},
  {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
  {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
  {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
  {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
  {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
  {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
  {0,4,4,0,4,4,0}, {0,0,0,0,0,6,6},
  {0,0,0,31,0,0,0}
};

static void block(GContext *ctx, int x, int y, int w, int h, GColor color) {
  if (s_raster) {
    raster_block(x, y, w, h, color);
    return;
  }
  graphics_context_set_fill_color(ctx, color);
  graphics_fill_rect(ctx, GRect(x, y, w, h), 0, GCornerNone);
}

static int glyph_index(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
  if (c == ':') return 36;
  if (c == '.') return 37;
  if (c == '-') return 38;
  return -1;
}

static int text_width(const char *text, int scale) {
  return ((int)strlen(text) * 6 - 1) * scale;
}

static void pixel_text(GContext *ctx, const char *text, int x, int y,
                       int scale, GColor color) {
  for (; *text; ++text, x += 6 * scale) {
    int index = glyph_index(*text);
    if (index < 0) continue;
    for (int row = 0; row < 7; ++row) {
      for (int col = 0; col < 5; ++col) {
        if (s_glyphs[index][row] & (16 >> col)) {
          block(ctx, x + col * scale, y + row * scale, scale, scale, color);
        }
      }
    }
  }
}

static void centered_text(GContext *ctx, const char *text, int center, int y,
                           int scale, GColor color) {
  pixel_text(ctx, text, center - text_width(text, scale) / 2, y, scale, color);
}

typedef struct {
  char lines[4][40];
  int count, width, height;
} TextBlock;

static TextBlock wrap_text(const char *text, int max_width, int scale) {
  TextBlock result = {0};
  int max_chars = max_width / (6 * scale);
  while (*text && result.count < 4) {
    while (*text == ' ') ++text;
    int length = 0, last_space = 0;
    while (text[length] && text[length] != '\n' && length < max_chars) {
      if (text[length] == ' ') last_space = length;
      ++length;
    }
    if (text[length] && text[length] != '\n' && last_space) length = last_space;
    char *line = result.lines[result.count++];
    if (length > 39) length = 39;
    memcpy(line, text, length);
    line[length] = '\0';
    int width = text_width(line, scale);
    if (width > result.width) result.width = width;
    text += length;
    if (*text == '\n') ++text;
  }
  result.height = result.count ? (result.count * 9 - 2) * scale : 0;
  return result;
}

static void draw_text_block(GContext *ctx, const TextBlock *text, GRect box, int scale) {
  int y = box.origin.y + (box.size.h - text->height) / 2;
  for (int i = 0; i < text->count; ++i) {
    centered_text(ctx, text->lines[i], box.origin.x + box.size.w / 2,
                    y + i * 9 * scale, scale, GColorWhite);
  }
}

static void sprite(GContext *ctx, const char *const *rows, int height,
                    int x, int y, int scale) {
  for (int row = 0; row < height; ++row) {
    for (int col = 0; rows[row][col]; ++col) {
      GColor color;
      switch (rows[row][col]) {
        case 'W': color = GColorWhite; break;
        case 'T': color = GColorPastelYellow; break;
        case 'B': color = GColorWindsorTan; break;
        case 'K': color = GColorBlack; break;
        default: continue;
      }
      block(ctx, x + col * scale, y + row * scale, scale, scale, color);
    }
  }
}

static void ox(GContext *ctx, int x, int ground, int scale, unsigned frame) {
  // Compact forward-curving horns and a broad, level muzzle face left.
  static const char *const body[] = {
    "..............................",
    "..............................",
    ".....B...B....................",
    ".....BT..BT...................",
    "......TT..TT..................",
    "......TWWWWTT.................",
    ".....TWWWWWWTTTTTTTTTTTTT.....",
    ".....WWKWWTTWWWWWWWWWWWWWT....",
    "....WWWWWWWWWWWWWWWWWWWWWWT...",
    "...TTTWWWWWWWWWWWWWWWWWWWWWT..",
    "...TKTWWWWWWWWWWWWWWWWWWWWWT..",
    "...TTTTTTWWWWWWWWWWWWWWWWWWT..",
    ".......TTTWWWWWWWWWWWWWWWWWT..",
    ".........TTWWWWWWWWWWWWWWWWT..",
    "..........TWWWWWWWWWWWWWWWT...",
    "...........WWWWTTTTTTWWWWWT...",
    "...........WWWT......WWWWT...."
  };
  int y = ground - 25 * scale;
  sprite(ctx, body, ARRAY_LENGTH(body), x, y, scale);
  // A narrow tail curves from the rump; its darker tuft swishes side to side.
  static const int8_t tail_sway[] = {-2, 0, 2, 0};
  int tail_x = 29;
  for (int row = 9; row <= 20; ++row) {
    tail_x = 29 + (row - 8) * (3 + tail_sway[frame]) / 12;
    block(ctx, x + tail_x * scale, y + row * scale, scale, scale,
        GColorPastelYellow);
  }
  block(ctx, x + (tail_x - 1) * scale, y + 20 * scale, 3 * scale, 3 * scale,
      GColorWindsorTan);
  // Draw the shaded far legs first, then the near legs in the opposite stride.
  static const int8_t stride[] = {-1, 0, 1, 0};
  static const int8_t roots[] = {14, 25, 12, 23};
  static const uint8_t phases[] = {0, 2, 2, 0};
  for (int leg = 0; leg < 4; ++leg) {
    int root = roots[leg];
    int step = stride[(frame + phases[leg]) % 4];
    GColor hide = leg < 2 ? GColorPastelYellow : GColorWhite;
    block(ctx, x + root * scale, y + 16 * scale, 3 * scale, 3 * scale, hide);
    block(ctx, x + (root + step) * scale, y + 19 * scale, 2 * scale, 4 * scale,
          hide);
    block(ctx, x + (root + step - 1) * scale, y + 23 * scale, 4 * scale, scale,
          GColorWindsorTan);
  }
  // A broad diagonal yoke runs from the shoulder down beneath the throat.
  for (int row = 0; row < 8; ++row) {
    int yoke_x = x + (14 - row) * scale;
    int yoke_y = y + (6 + row) * scale;
    block(ctx, yoke_x - scale, yoke_y, 5 * scale, scale, GColorBlack);
    block(ctx, yoke_x, yoke_y, 3 * scale, scale, GColorWindsorTan);
    block(ctx, yoke_x + 2 * scale, yoke_y, scale, scale, GColorPastelYellow);
  }
}

static void wheel(GContext *ctx, int x, int y, int scale, unsigned frame) {
  static const char *const rim[] = {
    "...TTTTT...", "..TW...WT..", ".TW.....WT.", "TW.......WT",
    "TW.......WT", "TW...T...WT", "TW.......WT", "TW.......WT",
    ".TW.....WT.", "..TW...WT..", "...TTTTT..."
  };
  block(ctx, x, y, 11 * scale, 11 * scale, GColorBlack);
  sprite(ctx, rim, ARRAY_LENGTH(rim), x, y, scale);
  for (int i = 1; i < 10; ++i) {
    if (frame % 2) {
      block(ctx, x + i * scale, y + i * scale, scale, scale, GColorPastelYellow);
      block(ctx, x + (10 - i) * scale, y + i * scale, scale, scale, GColorPastelYellow);
    } else {
      block(ctx, x + 5 * scale, y + i * scale, scale, scale, GColorPastelYellow);
      block(ctx, x + i * scale, y + 5 * scale, scale, scale, GColorPastelYellow);
    }
  }
}

static void wagon(GContext *ctx, int x, int ground, int scale, unsigned frame) {
  int y = ground - 37 * scale;
  block(ctx, x + 8 * scale, y, 29 * scale, 2 * scale, GColorWhite);
  block(ctx, x + 4 * scale, y + 2 * scale, 36 * scale, 3 * scale, GColorWhite);
  block(ctx, x + 2 * scale, y + 5 * scale, 40 * scale, 15 * scale, GColorWhite);
  block(ctx, x + 4 * scale, y + 20 * scale, 36 * scale, 3 * scale, GColorWhite);
  block(ctx, x + 6 * scale, y + 23 * scale, 32 * scale, scale, GColorWhite);
  block(ctx, x + 4 * scale, y + 6 * scale, 6 * scale, 15 * scale, GColorBlack);
  block(ctx, x + 6 * scale, y + 4 * scale, 2 * scale, 3 * scale, GColorBlack);
  block(ctx, x + 6 * scale, y + 8 * scale, 2 * scale, 11 * scale, GColorLightGray);
  for (int rib = 0; rib < 3; ++rib) {
    int rx = x + (16 + rib * 9) * scale;
    block(ctx, rx - 2 * scale, y + scale, scale, 2 * scale, GColorDarkGray);
    block(ctx, rx - scale, y + 3 * scale, scale, 3 * scale, GColorDarkGray);
    block(ctx, rx, y + 6 * scale, scale, 12 * scale, GColorDarkGray);
    block(ctx, rx - scale, y + 18 * scale, scale, 4 * scale, GColorDarkGray);
  }
  block(ctx, x + 3 * scale, y + 24 * scale, 36 * scale, 7 * scale, GColorWindsorTan);
  block(ctx, x + 3 * scale, y + 25 * scale, 36 * scale, scale, GColorPastelYellow);
  block(ctx, x + 5 * scale, y + 28 * scale, 32 * scale, scale, GColorBulgarianRose);
  block(ctx, x - 11 * scale, y + 28 * scale, 16 * scale, 2 * scale, GColorWindsorTan);
  wheel(ctx, x + 3 * scale, y + 26 * scale, scale, frame);
  wheel(ctx, x + 27 * scale, y + 26 * scale, scale, frame);
}

void scene_deinit(void) {
  for (unsigned i = 0; i < ARRAY_LENGTH(s_convoy); ++i) {
    if (s_convoy[i]) gbitmap_destroy(s_convoy[i]);
    s_convoy[i] = NULL;
  }
  if (s_clock) gbitmap_destroy(s_clock);
  if (s_panel) gbitmap_destroy(s_panel);
  if (s_landmark) gbitmap_destroy(s_landmark);
  s_clock = s_panel = s_landmark = s_raster = NULL;
  s_landmark_scenery = -1;
  s_clock_time[0] = s_clock_date[0] = s_status[0] = '\0';
  s_panel_valid = false;
}

void scene_init(GRect bounds) {
  scene_deinit();
  s_palette[0] = GColorClear;
  s_palette[1] = GColorWhite;
  s_palette[2] = GColorPastelYellow;
  s_palette[3] = GColorWindsorTan;
  s_palette[4] = GColorBlack;
  s_palette[5] = GColorLightGray;
  s_palette[6] = GColorDarkGray;
  s_palette[7] = GColorBulgarianRose;
  s_palette[8] = GColorIslamicGreen;
  s_palette[9] = GColorKellyGreen;
  s_palette[10] = GColorDarkGreen;
  s_palette[11] = GColorOrange;
  s_palette[12] = GColorRed;
  s_palette[13] = GColorBlue;
  s_palette[14] = GColorVividCerulean;
  s_palette[15] = GColorCeleste;
  int scale = bounds.size.w >= 200 ? 2 : 1;
  for (unsigned i = 0; i < ARRAY_LENGTH(s_convoy); ++i) {
    s_convoy[i] = gbitmap_create_blank_with_palette(GSize(76 * scale, 37 * scale),
        GBitmapFormat4BitPalette, s_palette, false);
    if (!s_convoy[i]) continue; // Use the original drawing path if RAM is tight.
    s_raster = s_convoy[i];
    ox(NULL, 0, 37 * scale, scale, i);
    wagon(NULL, 33 * scale, 37 * scale, scale, i);
    s_raster = NULL;
  }
  int clock_scale = bounds.size.w >= 200 ? 5 : 4;
  int time_y = PBL_IF_ROUND_ELSE(bounds.size.h / 10 + 6, 13);
  s_clock = gbitmap_create_blank_with_palette(
      GSize(bounds.size.w, time_y + 7 * clock_scale + 17),
      GBitmapFormat4BitPalette, s_palette, false);
  // One reusable scenery buffer, rather than retaining every route stop in RAM.
  s_landmark = gbitmap_create_blank_with_palette(GSize(45 * scale, 32 * scale),
      GBitmapFormat4BitPalette, s_palette, false);
}

static void cache_panel(GContext *ctx, GRect panel, GPoint window_origin) {
  // Captures use screen coordinates. Do not cache a window mid-transition.
  if (window_origin.x || window_origin.y) return;
  if (s_panel) {
    GSize old_size = gbitmap_get_bounds(s_panel).size;
    if (!gsize_equal(&old_size, &panel.size)) {
      gbitmap_destroy(s_panel);
      s_panel = NULL;
    }
  }
  if (!s_panel) s_panel = gbitmap_create_blank(panel.size, GBitmapFormat8Bit);
  if (!s_panel) return;
  GBitmap *frame = graphics_capture_frame_buffer_format(ctx, GBitmapFormat8Bit);
  if (!frame) return;
  GRect frame_bounds = gbitmap_get_bounds(frame);
  if (panel.origin.y < frame_bounds.origin.y ||
      panel.origin.y + panel.size.h > frame_bounds.origin.y + frame_bounds.size.h) {
    graphics_release_frame_buffer(ctx, frame);
    return;
  }
  uint8_t *data = gbitmap_get_data(s_panel);
  int stride = gbitmap_get_bytes_per_row(s_panel);
  for (int y = 0; y < panel.size.h; ++y) {
    GBitmapDataRowInfo row = gbitmap_get_data_row_info(frame, panel.origin.y + y);
    int left = panel.origin.x > row.min_x ? panel.origin.x : row.min_x;
    int right = panel.origin.x + panel.size.w - 1;
    if (right > row.max_x) right = row.max_x;
    uint8_t *destination = data + y * stride;
    memset(destination, GColorBlack.argb, panel.size.w);
    if (right >= left) {
      memcpy(destination + left - panel.origin.x, row.data + left, right - left + 1);
    }
  }
  graphics_release_frame_buffer(ctx, frame);
  s_panel_valid = true;
}

static void landmark(GContext *ctx, int x, int ground, int scale, TrailScenery kind) {
  int y = ground - 30 * scale;
  if (kind == SCENERY_FORT) {
    block(ctx, x + 3 * scale, y + 10 * scale, 24 * scale, 19 * scale, GColorWindsorTan);
    for (int i = 0; i < 24; i += 4) {
      block(ctx, x + (3 + i) * scale, y + 8 * scale, 2 * scale, 21 * scale, GColorOrange);
    }
    block(ctx, x + 10 * scale, y + 19 * scale, 8 * scale, 10 * scale, GColorBlack);
    block(ctx, x, y + 8 * scale, 29 * scale, 2 * scale, GColorLightGray);
    block(ctx, x + 30 * scale, y, scale, 29 * scale, GColorWhite);
    block(ctx, x + 31 * scale, y, 11 * scale, 7 * scale, GColorWhite);
    for (int i = 0; i < 7; i += 2) {
      block(ctx, x + 31 * scale, y + i * scale, 11 * scale, scale, GColorRed);
    }
    block(ctx, x + 31 * scale, y, 4 * scale, 4 * scale, GColorBlue);
  } else if (kind == SCENERY_RIVER) {
    // Keep the channel narrow enough to show two pronounced opposing bends.
    static const uint8_t centers[] = {14, 21, 29, 33, 33, 29, 21, 12,
        6, 6, 10, 18, 28, 30, 30};
    for (int row = 0; row < 30; ++row) {
      int index = row / 2;
      int center = centers[index];
      if ((row & 1) && index + 1 < (int)ARRAY_LENGTH(centers)) {
        center = (center + centers[index + 1]) / 2;
      }
      // Flare the last seven rows into a broad mouth without filling the bends.
      int channel_width = row < 23 ? 5 + row / 5 : 9 + (row - 22) * 2;
      int left = center - channel_width / 2;
      block(ctx, x + left * scale, y + row * scale,
          channel_width * scale, scale, GColorVividCerulean);
      block(ctx, x + left * scale, y + row * scale, scale, scale, GColorCeleste);
    }
  } else if (kind == SCENERY_CHIMNEY_ROCK) {
    block(ctx, x + 17 * scale, y, 4 * scale, 20 * scale, GColorOrange);
    block(ctx, x + 18 * scale, y + 3 * scale, 2 * scale, 14 * scale, GColorWindsorTan);
    for (int i = 0; i < 10; ++i) {
      block(ctx, x + (16 - i) * scale, y + (19 + i) * scale,
            (6 + i * 3) * scale, scale, GColorOrange);
    }
    block(ctx, x + 5 * scale, ground - scale, 37 * scale, scale, GColorWindsorTan);
  } else if (kind == SCENERY_ROUND_ROCK) {
    for (int row = 0; row < 14; ++row) {
      int inset = 12 - row * 9 / 13;
      block(ctx, x + inset * scale, y + (16 + row) * scale,
          (45 - inset * 2) * scale, scale, GColorWindsorTan);
    }
    block(ctx, x + 13 * scale, y + 18 * scale, 19 * scale, scale, GColorPastelYellow);
    block(ctx, x + 7 * scale, y + 24 * scale, 27 * scale, scale, GColorDarkGray);
    block(ctx, x + 32 * scale, y + 22 * scale, 5 * scale, 7 * scale, GColorDarkGray);
  } else if (kind == SCENERY_MOUNTAINS) {
    for (int row = 0; row < 29; ++row) {
      int half = row * 13 / 28;
      block(ctx, x + (14 - half) * scale, y + (1 + row) * scale,
          (half * 2 + 1) * scale, scale, GColorDarkGray);
      if (row >= 8) {
        half = (row - 8) * 12 / 20;
        block(ctx, x + (31 - half) * scale, y + (1 + row) * scale,
            (half * 2 + 1) * scale, scale, GColorLightGray);
      }
    }
    block(ctx, x + 13 * scale, y + 3 * scale, 3 * scale, 2 * scale, GColorWhite);
    block(ctx, x + 30 * scale, y + 10 * scale, 3 * scale, scale, GColorWhite);
  } else if (kind == SCENERY_SPRINGS) {
    for (int row = 0; row < 7; ++row) {
      int inset = row < 3 ? 3 - row : row - 3;
      block(ctx, x + (7 + inset) * scale, y + (23 + row) * scale,
          (31 - inset * 2) * scale, scale, GColorVividCerulean);
    }
    block(ctx, x + 20 * scale, y + 3 * scale, 4 * scale, 22 * scale, GColorCeleste);
    block(ctx, x + 21 * scale, y + 2 * scale, 2 * scale, 22 * scale, GColorWhite);
    block(ctx, x + 15 * scale, y + 2 * scale, 5 * scale, 2 * scale, GColorCeleste);
    block(ctx, x + 24 * scale, y + 2 * scale, 5 * scale, 2 * scale, GColorCeleste);
    block(ctx, x + 13 * scale, y + 6 * scale, 2 * scale, 3 * scale, GColorWhite);
    block(ctx, x + 29 * scale, y + 6 * scale, 2 * scale, 3 * scale, GColorWhite);
  } else if (kind == SCENERY_DALLES) {
    block(ctx, x + 15 * scale, y + 5 * scale, 17 * scale, 25 * scale,
        GColorVividCerulean);
    block(ctx, x + 20 * scale, y + 7 * scale, 2 * scale, 22 * scale, GColorCeleste);
    for (int row = 0; row < 25; ++row) {
      int edge = row < 9 ? 15 - row / 3 : 12;
      block(ctx, x, y + (5 + row) * scale, edge * scale, scale, GColorWindsorTan);
      block(ctx, x + (45 - edge) * scale, y + (5 + row) * scale,
          edge * scale, scale, GColorDarkGray);
    }
  }
}

typedef struct {
  int text_width, text_height;
} MessageFit;

static GFont event_font(unsigned size) {
  return fonts_get_system_font(size == 28 ? FONT_KEY_GOTHIC_28 :
      size == 18 ? FONT_KEY_GOTHIC_18 : FONT_KEY_GOTHIC_14);
}

static bool message_fits(const char *message, const char *name, unsigned size, void *context) {
  const MessageFit *fit = context;
  GFont font = event_font(size);
  // Multi-word names may wrap within the single panel. Check each word to
  // prevent a very wide, unbroken name from running past the text area.
  while (*name) {
    while (*name == ' ') ++name;
    size_t used = 0;
    while (*name && *name != ' ' && used < sizeof(s_measured_word) - 1) {
      s_measured_word[used++] = *name++;
    }
    s_measured_word[used] = '\0';
    GSize word_size = graphics_text_layout_get_content_size(s_measured_word, font,
        GRect(0, 0, 4096, 4096), GTextOverflowModeWordWrap, GTextAlignmentCenter);
    if (word_size.w > fit->text_width) return false;
  }
  GSize message_size = graphics_text_layout_get_content_size(message, font,
      GRect(0, 0, fit->text_width, 4096),
      GTextOverflowModeWordWrap, GTextAlignmentCenter);
  return message_size.h <= fit->text_height;
}

static void draw_fallback_text(GContext *ctx, const char *status, GRect box, int scale) {
  const TextBlock text = wrap_text(status, box.size.w, scale);
  draw_text_block(ctx, &text, box, scale);
}

void scene_draw(GContext *ctx, GRect bounds, const Trail *trail,
                const char *time_text, const char *date_text, GFont status_font,
                GPoint window_origin) {
  const int width = bounds.size.w, height = bounds.size.h;
  const int scale = width >= 200 ? 2 : 1;
  const int clock_scale = width >= 200 ? 5 : 4;
  const int time_y = PBL_IF_ROUND_ELSE(height / 10 + 6, 13);
  const int ground = height * 66 / 100;
  block(ctx, 0, 0, width, height, GColorBlack);
  block(ctx, 0, ground, width, height - ground, GColorIslamicGreen);
  if (s_clock) {
    if (strcmp(s_clock_time, time_text) || strcmp(s_clock_date, date_text)) {
      s_raster = s_clock;
      GRect clock_bounds = gbitmap_get_bounds(s_clock);
      block(NULL, 0, 0, clock_bounds.size.w, clock_bounds.size.h, GColorBlack);
      centered_text(NULL, time_text, width / 2, time_y, clock_scale, GColorWhite);
      centered_text(NULL, date_text, width / 2, time_y + 7 * clock_scale + 10,
          1, GColorPastelYellow);
      s_raster = NULL;
      snprintf(s_clock_time, sizeof(s_clock_time), "%s", time_text);
      snprintf(s_clock_date, sizeof(s_clock_date), "%s", date_text);
    }
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(ctx, s_clock, gbitmap_get_bounds(s_clock));
  } else {
    centered_text(ctx, time_text, width / 2, time_y, clock_scale, GColorWhite);
    centered_text(ctx, date_text, width / 2, time_y + 7 * clock_scale + 10,
        1, GColorPastelYellow);
  }

  for (int i = -32; i < width + 32; i += 4) {
    int x = i + trail->grass;
    int rise = (i * i + 7) % 5;
    block(ctx, x, ground - rise - 1, 2, rise + 2, GColorIslamicGreen);
    if ((i / 4) % 3 == 0) block(ctx, x, ground + 3, 2, 1, GColorKellyGreen);
  }
  for (int i = 0; i < 9; ++i) {
    int x = (i * 37 + trail->grass) % width;
    block(ctx, x, ground + 4 + (i % 3) * 3, 3, 1, GColorDarkGreen);
  }
  if (trail->landmark_visible) {
    int x = -45 * scale + (width + 45 * scale) * trail->landmark_progress / 256;
    TrailScenery scenery = TRAIL_LANDMARKS[trail->landmark].scenery;
    if (s_landmark) {
      if (s_landmark_scenery != (int)scenery) {
        s_raster = s_landmark;
        memset(gbitmap_get_data(s_landmark), 0,
            gbitmap_get_bytes_per_row(s_landmark) * 32 * scale);
        landmark(NULL, 0, 32 * scale, scale, scenery);
        s_raster = NULL;
        s_landmark_scenery = scenery;
      }
      graphics_context_set_compositing_mode(ctx, GCompOpSet);
      graphics_draw_bitmap_in_rect(ctx, s_landmark,
          GRect(x, ground - 34 * scale, 45 * scale, 32 * scale));
    } else {
      landmark(ctx, x, ground - 2 * scale, scale, scenery);
    }
  }

  int convoy_x = (width - 76 * scale) / 2;
  if (s_convoy[trail->frame]) {
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(ctx, s_convoy[trail->frame],
        GRect(convoy_x, ground - 37 * scale, 76 * scale, 37 * scale));
  } else {
    ox(ctx, convoy_x, ground, scale, trail->frame);
    wagon(ctx, convoy_x + 33 * scale, ground, scale, trail->frame);
  }

  char progress[64];
  snprintf(progress, sizeof(progress), "%s\n%d MILES", TRAIL_LANDMARKS[trail->landmark].label,
           86 - trail->passage_tick * 85 / 320);
  const char *message = trail_event(trail);
  const char *status = message ? message : progress;
  if (!strcmp(s_status, status) && s_panel_valid) {
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(ctx, s_panel, s_panel_rect);
    return;
  }
  s_panel_valid = false;
  GFont draw_font = status_font;
  // Match each rectangular display to its circular counterpart, including
  // the inner text width so identical fonts wrap the same way.
  const int panel_width = width >= 200 ? 150 : 104;
  const int padding = 4;
  if (message && draw_font) {
    int bottom = height - 2;
#if defined(PBL_ROUND)
    // Keep both lower panel corners inside the circle, not just its center.
    const int radius = width / 2;
    const int half_panel = (panel_width + 1) / 2;
    while (bottom > height / 2 &&
        half_panel * half_panel + (bottom - height / 2) * (bottom - height / 2)
            >= (radius - 2) * (radius - 2)) --bottom;
#endif
    const int top = PBL_IF_ROUND_ELSE(ground + 1, ground);
    MessageFit fit = {panel_width - padding * 2,
        bottom - top - padding * 2};
    unsigned size = event_layout_fit(trail, s_fitted_message, sizeof(s_fitted_message),
        width >= 200 ? 28 : 18, message_fits, &fit);
    draw_font = event_font(size ? size : 14);
    status = s_fitted_message;
  }
  const GSize text_size = graphics_text_layout_get_content_size(status, draw_font,
      GRect(0, 0, panel_width - padding * 2, height),
      GTextOverflowModeWordWrap, GTextAlignmentCenter);
  const int panel_height = text_size.h + padding * 2;
  const int panel_y = PBL_IF_ROUND_ELSE(ground + 1,
      ground + (height - ground - panel_height) / 2);
  GRect panel = GRect((width - panel_width) / 2, panel_y, panel_width, panel_height);
  block(ctx, panel.origin.x, panel.origin.y, panel.size.w, panel.size.h, GColorWhite);
  block(ctx, panel.origin.x + 2, panel.origin.y + 2, panel.size.w - 4, panel.size.h - 4,
        GColorBlack);
  GRect text_box = GRect(panel.origin.x + padding, panel.origin.y + padding,
                         panel.size.w - padding * 2, panel.size.h - padding * 2);
  if (draw_font) {
    // Gothic fonts include leading above uppercase glyphs. Balance that space
    // across the top and bottom of the panel without changing the wrapping.
    text_box.origin.y -= width >= 200 ? 5 : 4;
    text_box.size.h += width >= 200 ? 5 : 4;
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, status, draw_font, text_box,
                       GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  } else {
    draw_fallback_text(ctx, status, text_box, scale);
  }
  snprintf(s_status, sizeof(s_status), "%s", message ? message : progress);
  s_panel_rect = panel;
  cache_panel(ctx, panel, window_origin);
}
