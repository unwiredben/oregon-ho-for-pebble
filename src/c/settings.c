/* Copyright (c) 2026 Ben Combee. SPDX-License-Identifier: MIT */
#include <pebble.h>
#include <string.h>
#include "settings.h"

enum { PERSIST_NAME_FIRST = 100, PERSIST_NO_GUNS = 105 };
static Trail *s_trail;
static void (*s_changed)(void);

static void inbox_received(DictionaryIterator *iterator, void *context) {
  // SDK-generated message keys are extern variables, not C constants.
  const uint32_t name_keys[TRAIL_PARTY_SIZE] = {
    MESSAGE_KEY_PartyName1, MESSAGE_KEY_PartyName2, MESSAGE_KEY_PartyName3,
    MESSAGE_KEY_PartyName4, MESSAGE_KEY_PartyName5
  };
  (void)context;
  bool changed = false;
  for (unsigned i = 0; i < TRAIL_PARTY_SIZE; ++i) {
    Tuple *tuple = dict_find(iterator, name_keys[i]);
    if (!tuple) continue;
    // Tuple's string member is a zero-length array in the SDK. Access the
    // variable-size union payload as bytes after checking the tuple length.
    const char *name = (const char *)tuple->value;
    if (tuple->type != TUPLE_CSTRING || !tuple->length ||
        tuple->length > TRAIL_NAME_BYTES + 1 ||
        name[tuple->length - 1] != '\0' ||
        memchr(name, '\0', tuple->length - 1)) {
      APP_LOG(APP_LOG_LEVEL_WARNING, "Ignored malformed party name %u", i + 1);
      continue;
    }
    char previous[TRAIL_NAME_BYTES + 1];
    strcpy(previous, s_trail->names[i]);
    trail_set_name(s_trail, i, name);
    if (strcmp(previous, s_trail->names[i])) {
      changed = true;
      if (persist_write_string(PERSIST_NAME_FIRST + i, s_trail->names[i]) < 0) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Unable to persist party name %u", i + 1);
      }
    }
  }
  Tuple *tuple = dict_find(iterator, MESSAGE_KEY_NoGuns);
  if (tuple && (tuple->type == TUPLE_INT || tuple->type == TUPLE_UINT) &&
      (tuple->length == 1 || tuple->length == 2 || tuple->length == 4)) {
    uint32_t value = 0;
    memcpy(&value, tuple->value, tuple->length);
    if (value <= 1 && s_trail->no_guns != (value != 0)) {
      changed = true;
      s_trail->no_guns = value != 0;
      if (persist_write_bool(PERSIST_NO_GUNS, s_trail->no_guns) < 0) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Unable to persist No guns");
      }
    }
  }
  if (changed) {
    if (trail_event(s_trail)) trail_refresh_message(s_trail);
    if (s_changed) s_changed();
  }
}

static void inbox_dropped(AppMessageResult reason, void *context) {
  (void)context;
  (void)reason;
  APP_LOG(APP_LOG_LEVEL_WARNING, "Settings message dropped: %d", (int)reason);
}

void settings_init(Trail *trail, void (*changed)(void)) {
  s_trail = trail;
  s_changed = changed;
  for (unsigned i = 0; i < TRAIL_PARTY_SIZE; ++i) {
    if (persist_exists(PERSIST_NAME_FIRST + i)) {
      char name[TRAIL_NAME_BYTES + 1] = {0};
      if (persist_read_string(PERSIST_NAME_FIRST + i, name, sizeof(name)) > 0) {
        name[TRAIL_NAME_BYTES] = '\0';
        trail_set_name(trail, i, name);
      }
    }
  }
  if (persist_exists(PERSIST_NO_GUNS)) trail->no_guns = persist_read_bool(PERSIST_NO_GUNS);
  app_message_register_inbox_received(inbox_received);
  app_message_register_inbox_dropped(inbox_dropped);
  AppMessageResult result = app_message_open(768, 64);
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Unable to open settings inbox: %d", (int)result);
  }
}

void settings_deinit(void) {
  app_message_deregister_callbacks();
  s_trail = NULL;
  s_changed = NULL;
}
