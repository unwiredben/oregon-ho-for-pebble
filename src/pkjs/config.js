/* Copyright (c) 2026 Ben Combee. SPDX-License-Identifier: MIT */
var names = [{type: 'heading', defaultValue: 'Your party'},
  {type: 'text', defaultValue: 'Events use only the names you enter, leave them all blank to use the default travelers. Long names use a smaller font and events may be abbreviated to fit.'}];
for (var i = 1; i <= 5; ++i) {
  names.push({type: 'input', messageKey: 'PartyName' + i,
    // label: 'Party member ' + i, defaultValue: '',
    attributes: {maxlength: 24, placeholder: 'Optional name'}});
}
module.exports = [
  {type: 'heading', defaultValue: 'Oregon Ho!'},
  {type: 'section', items: names},
  {type: 'section', items: [
    {type: 'heading', defaultValue: 'Trail stories'},
    {type: 'toggle', messageKey: 'NoGuns', label: 'No guns', defaultValue: false,
      description: 'Disable all events involving shooting animals.'}
  ]},
  {type: 'submit', defaultValue: 'Save settings'}
];
