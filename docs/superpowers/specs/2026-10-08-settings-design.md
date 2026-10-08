# Oregon Ho! 1.3.0 settings

Approved design: add an offline Clay settings page with five optional party
names and a No guns toggle. Events use only nonblank custom names; all blank
fields restore the existing twelve-name pool. No guns defaults to off and
excludes all six shooting templates. Saving applies immediately, replaces any
visible story with one using the new settings, and preserves animation state.

Use `@rebble/clay`, the package maintained by pebble-dev/clay. Keep the UUID,
watchface type and Basalt, Chalk, Emery and Gabbro targets. Set version 1.3.0
and declare configurable capability and six message keys.

Names are trimmed, ASCII letters uppercased for the existing story style, and
bounded to 96 UTF-8 bytes on the watch. Clay inputs allow 24 UTF-16 code units.
Control characters are removed. Empty and whitespace-only inputs are ignored.
Store each name separately and the toggle in watch persistence, avoiding the
256-byte per-record limit. Validate incoming types, lengths and termination.
Malformed fields leave the previous value intact. Failed persistence and dropped
messages are logged. Clay saves phone settings and resends saved values on ready
so changes made while the watchface is unavailable can be delivered later.

Show a live advisory warning for names longer than eight characters or containing
particularly wide letters. Explain that long names may be shortened on the watch.
The warning is conservative, not a claim to reproduce Pebble font metrics in a
browser. Preserve stored names. User refinement: keep each event in one box and
prefer smaller fonts to show the whole name and story. At draw time use Pebble's
actual font measurement to try the full message at Gothic 28, 18, then 14 (start
at 18 on small watches). Multi-word names may wrap inside that one box. Only if
the full message still cannot fit at 14, shorten the name with an ellipsis while
preserving the complete event suffix. Bound individual word widths and total
panel height, including the circular display edge.
Keep fitting buffers in static storage to respect the watch's small drawing stack.

Validate custom-only selection, sparse slots, blank fallback, bounded UTF-8,
all shooting exclusions, immediate refresh without restarting animation, malformed
messages and reload from persistence in host tests. Exercise real Clay callbacks,
live warning changes, save/cancel and phone resending in JavaScript checks.
Build all four targets and use isolated emulator flash to test real AppMessage
delivery, persistence after restart and long/wide-name rendering. Deliver the
versioned PBW and screenshots. Publishing is outside this request.
