/* Copyright (c) 2026 Ben Combee. SPDX-License-Identifier: MIT */
'use strict';
var assert = require('assert');
/* Use actual Clay for save/cancel and transport, faking only the phone API. */
var listeners = {}, sent = [], urls = [], storage = {};
global.localStorage = {
  getItem: function(key) { return storage[key] || null; },
  setItem: function(key, value) { storage[key] = String(value); }
};
global.Pebble = {
  addEventListener: function(event, callback) { listeners[event] = callback; },
  getActiveWatchInfo: function() { return {platform: 'basalt'}; },
  getAccountToken: function() { return 'test-account'; },
  getWatchToken: function() { return 'test-watch'; },
  openURL: function(url) { urls.push(url); },
  sendAppMessage: function(payload, success) { sent.push(payload); if (success) success(); }
};
/* Clay's packaged entry requires the generated key module supplied by Pebble. */
var Module = require('module'), original = Module._load;
var keys = {PartyName1: 1, PartyName2: 2, PartyName3: 3,
            PartyName4: 4, PartyName5: 5, NoGuns: 6};
Module._load = function(name, parent, isMain) {
  if (name === '@rebble/clay') {
    return original.call(this, '@rebble/clay/src/js/index.js', parent, isMain);
  }
  return name === 'message_keys' ? keys : original.call(this, name, parent, isMain);
};
require('../src/pkjs/index');
listeners.showConfiguration({});
assert(urls.length === 1 && urls[0].indexOf('data:text/html') === 0);
listeners.webviewclosed({response: ''});
assert.equal(sent.length, 0);
var response = {};
Object.keys(keys).forEach(function(key) {
  response[key] = {value: key === 'NoGuns' ? true : key === 'PartyName1' ? 'Robin' : ''};
});
listeners.webviewclosed({response: encodeURIComponent(JSON.stringify(response))});
assert.equal(sent.length, 1);
assert.equal(sent[0][1], 'Robin');
assert.equal(sent[0][6], 1);
listeners.ready({});
assert.equal(sent.length, 2);
assert.deepEqual(sent[1], sent[0]);
Module._load = original;
console.log('Clay configuration generation, cancel, save and ready resend passed.');
