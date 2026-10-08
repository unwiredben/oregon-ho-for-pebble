/* Copyright (c) 2026 Ben Combee. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pebble.h>
#include "settings.h"
uint32_t MESSAGE_KEY_PartyName1 = 1, MESSAGE_KEY_PartyName2 = 2,
    MESSAGE_KEY_PartyName3 = 3, MESSAGE_KEY_PartyName4 = 4, MESSAGE_KEY_PartyName5 = 5,
    MESSAGE_KEY_NoGuns = 6;

static void (*received)(DictionaryIterator *, void *);
static char stored[256][100];
static bool exists[256], stored_bool[256];
static unsigned changes;
static void changed(void) { ++changes; }
Tuple *dict_find(const DictionaryIterator *iterator, uint32_t key) {
  for (unsigned i = 0; i < iterator->count; ++i) {
    if (iterator->tuples[i].key == key) return &iterator->tuples[i];
  }
  return NULL;
}
bool persist_exists(uint32_t key) { return exists[key]; }
int persist_read_string(uint32_t key, char *buffer, size_t size) {
  snprintf(buffer, size, "%s", stored[key]);
  return strlen(buffer) + 1;
}
bool persist_read_bool(uint32_t key) { return stored_bool[key]; }
int persist_write_string(uint32_t key, const char *text) {
  strcpy(stored[key], text); exists[key] = true; return strlen(text) + 1;
}
int persist_write_bool(uint32_t key, bool value) {
  stored_bool[key] = value; exists[key] = true; return 1;
}
void app_message_register_inbox_received(void (*callback)(DictionaryIterator *, void *)) {
  received = callback;
}
void app_message_register_inbox_dropped(void (*callback)(AppMessageResult, void *)) {
  (void)callback;
}
AppMessageResult app_message_open(uint32_t inbox, uint32_t outbox) {
  assert(received && inbox >= 600 && outbox >= 1); return APP_MSG_OK;
}
void app_message_deregister_callbacks(void) { received = NULL; }

int main(void) {
  Trail trail;
  trail_init(&trail, 123);
  settings_init(&trail, changed);
  assert(!trail.no_guns && !trail.names[0][0]);
  do { trail_refresh_message(&trail); } while (!strstr(trail.message, " SHOT "));
  Trail frozen = trail;
  int32_t yes = 1;
  char name[] = "  Robin ";
  Tuple tuples[] = {
    {MESSAGE_KEY_PartyName3, TUPLE_CSTRING, sizeof(name), (TupleValue *)name},
    {MESSAGE_KEY_NoGuns, TUPLE_INT, sizeof(yes), (TupleValue *)&yes}
  };
  DictionaryIterator iterator = {tuples, 2};
  received(&iterator, NULL);
  assert(changes == 1 && trail.no_guns);
  assert(strncmp(trail.message, "ROBIN ", 6) == 0);
  assert(!strstr(trail.message, " SHOT "));
  assert(trail.frame == frozen.frame && trail.grass == frozen.grass);
  assert(trail.animation_remaining == frozen.animation_remaining);
  assert(trail.passage_tick == frozen.passage_tick);
  Trail reloaded;
  trail_init(&reloaded, 123);
  settings_deinit();
  settings_init(&reloaded, changed);
  assert(reloaded.no_guns && strcmp(reloaded.names[2], "ROBIN") == 0);
  /* Bad types, missing terminator, oversized strings and invalid boolean. */
  char unterminated[] = {'A', 'B'};
  char oversized[101]; memset(oversized, 'A', 100); oversized[100] = 0;
  Tuple bad[] = {
    {MESSAGE_KEY_PartyName3, TUPLE_INT, 4, (TupleValue *)&yes},
    {MESSAGE_KEY_PartyName2, TUPLE_CSTRING, 2, (TupleValue *)unterminated},
    {MESSAGE_KEY_PartyName1, TUPLE_CSTRING, sizeof(oversized), (TupleValue *)oversized}
  };
  iterator = (DictionaryIterator){bad, 3};
  received(&iterator, NULL);
  assert(strcmp(reloaded.names[2], "ROBIN") == 0);
  assert(!reloaded.names[0][0] && !reloaded.names[1][0] && changes == 1);
  yes = 2; iterator = (DictionaryIterator){&tuples[1], 1};
  received(&iterator, NULL);
  assert(reloaded.no_guns && changes == 1);
  /* A partial update leaves absent names alone; blank explicitly clears. */
  char blank[] = "   ";
  tuples[0].length = sizeof(blank); tuples[0].value = (TupleValue *)blank;
  yes = 0; iterator = (DictionaryIterator){tuples, 2};
  received(&iterator, NULL);
  assert(!reloaded.names[2][0] && !reloaded.no_guns && changes == 2);
  assert(trail_event(&reloaded) == NULL); /* No new story before the first event. */
  settings_deinit();
  trail_init(&trail, 456); settings_init(&trail, changed);
  assert(!trail.names[2][0] && !trail.no_guns);
  settings_deinit();
  puts("Settings delivery, filtering, partial/malformed updates and persistence passed.");
}
