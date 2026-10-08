/* Copyright (c) 2026 Ben Combee. SPDX-License-Identifier: MIT */
/* Optional DOM integration check: install jsdom separately and set NODE_PATH. */
'use strict';
var assert = require('assert');
var JSDOM = require('jsdom').JSDOM;
var Module = require('module'), original = Module._load;
Module._load = function(name, parent, isMain) {
  return name === 'message_keys' ? {PartyName1: 1, PartyName2: 2,
    PartyName3: 3, PartyName4: 4, PartyName5: 5, NoGuns: 6}
    : original.call(this, name, parent, isMain);
};
global.Pebble = {platform: 'test', addEventListener: function() {}};
global.localStorage = {getItem: function() { return JSON.stringify({PartyName1: 'Robin', NoGuns: true}); }};
var Clay = require('@rebble/clay/src/js/index.js');
var custom = require('../src/pkjs/custom-clay');
// Expose the page's real ClayConfig only in this test's generated HTML.
var wrapped = new Function('window.testClay = this; (' + custom.toString() + ').call(this);');
var clay = new Clay(require('../src/pkjs/config'), wrapped, {autoHandleEvents: false});
var html = decodeURIComponent(clay.generateUrl().split(',').slice(1).join(','));
var dom = new JSDOM(html, {runScripts: 'dangerously', url: 'https://oregon-ho.test/'});
var document = dom.window.document;
var inputs = document.querySelectorAll('.component-input input');
assert.equal(inputs.length, 5);
assert.equal(inputs[0].value, 'Robin');
assert.equal(inputs[0].maxLength, 24);
assert(document.querySelector('.component-toggle input').checked);
inputs[0].value = 'WWWWWWWW';
inputs[0].dispatchEvent(new dom.window.Event('input', {bubbles: true}));
var field = inputs[0].closest('.component-input');
var description = field.querySelector('.description');
assert(description && description.textContent.indexOf('smaller font') >= 0);
assert.notEqual(description.style.display, 'none');
assert.equal(document.querySelectorAll('.component-text').length, 1);
inputs[0].value = 'Ann';
inputs[0].dispatchEvent(new dom.window.Event('input', {bubbles: true}));
assert.equal(description.textContent, '');
assert.equal(description.style.display, 'none');
inputs[0].value = 'Alexanderson';
inputs[0].dispatchEvent(new dom.window.Event('change', {bubbles: true}));
assert(description.textContent.indexOf('shortened') >= 0);
inputs[0].value = 'Ann';
inputs[0].dispatchEvent(new dom.window.Event('input', {bubbles: true}));
inputs[4].value = '<img src=x onerror=alert(1)>';
inputs[4].dispatchEvent(new dom.window.Event('input', {bubbles: true}));
assert.equal(document.querySelectorAll('img').length, 0);
var response = dom.window.testClay.serialize();
assert.equal(response.PartyName1.value, 'Ann');
assert.equal(response.PartyName5.value, '<img src=x onerror=alert(1)>');
assert.equal(response.NoGuns.value, true);
dom.window.close();
Module._load = original;
console.log('Generated Clay HTML builds, restores settings, warns live and serializes all fields.');
