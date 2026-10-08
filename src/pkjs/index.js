/* Copyright (c) 2026 Ben Combee. SPDX-License-Identifier: MIT */
var Clay = require('@rebble/clay');
var clay = new Clay(require('./config'), require('./custom-clay'));

/* Clay persists on the phone before sending. Retry saved settings on reconnect
 * or launch, including changes saved while this watchface was not running. */
Pebble.addEventListener('ready', function() {
  var saved;
  try {
    saved = JSON.parse(localStorage.getItem('clay-settings')) || {};
  } catch (error) {
    console.log('Unable to read Oregon Ho! settings: ' + error);
    return;
  }
  var settings = Clay.prepareSettingsForAppMessage(saved);
  if (Object.keys(settings).length) {
    Pebble.sendAppMessage(settings, function() {}, function(error) {
      console.log('Unable to resend Oregon Ho! settings: ' + JSON.stringify(error));
    });
  }
});
