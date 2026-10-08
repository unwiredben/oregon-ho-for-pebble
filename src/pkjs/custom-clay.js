/* Copyright (c) 2026 Ben Combee. SPDX-License-Identifier: MIT */
/* Clay serializes this function into the page: keep its dependencies inside. */
module.exports = function() {
  var clay = this;
  clay.on(clay.EVENTS.AFTER_BUILD, function() {
    for (var i = 1; i <= 5; ++i) {
      (function(index) {
        var input = clay.getItemByMessageKey('PartyName' + index);
        var field = input.$element[0];
        var description = field.querySelector('.description');
        if (!description) {
          description = document.createElement('div');
          description.className = 'description';
          field.appendChild(description);
        }
        function update() {
          var name = String(input.get() || '').replace(/^\s+|\s+$/g, '').toUpperCase();
          // Conservative advisory; the watch measures its actual Gothic font.
          var width = 0;
          for (var j = 0; j < name.length; ++j) {
            width += /[MW@]/.test(name.charAt(j)) ? 1.5 : /[I1 .'\-]/.test(name.charAt(j)) ? 0.5 : 1;
          }
          description.textContent = name.length > 8 || width > 8
            ? 'This name may need a smaller font or be shortened to fit the full story on the watch. Short names are easier to read.' : '';
          description.style.display = description.textContent ? '' : 'none';
        }
        update();
        input.on('input change', update);
      })(i);
    }
  });
};
