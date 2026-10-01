# Oregon Ho watchface design

Build a native C Pebble watchface for the existing basalt, chalk, emery, and
gabbro targets. Adapt the supplied Oregon Trail progress screenshots to the
watch display with crisp pixel artwork and a black, green, white, and brown
palette.

## Screen

Show large pixel-style time at the top, respecting the watch's 12/24-hour
setting, with the current date underneath and an 11–15 pixel clear gap above
the wagon canopy on the declared targets. Reserve the middle for a left-facing
single stout ox with prominent horns and a wooden yoke pulling a white covered
wagon with a brown chassis. Animate walking
legs and wheel spokes. Keep the clock and messages inside the safe central
area on round displays.

Fill the bottom portion with bright green terrain. Add a black status panel
with a white pixel border. Keep the panel width constant on each device and size its height to the wrapped
text. Balance the visible padding above and below the lettering. Center
rectangular panels horizontally and vertically in the green ground area. Basalt and Chalk use the built-in
regular Gothic 18 status font; Emery and Gabbro use regular Gothic 28.
Panels fit the text height with four pixels of padding on each side. Basalt
and Chalk share a 104-pixel panel width; Emery and Gabbro share 150 pixels.
During ordinary travel it shows a landmark name
and distance; occasional temporary messages describe fictional travelers
getting sick, breaking limbs, recovering, or encountering trail mishaps.
Use a built-in pool of names and events without phone connectivity. Leave the
green space below the panel clear, without a pace footer.

## Motion and landmarks

Keep the wagon in place while terrain details and occasional pixel landmarks
move from left to right, matching the left-facing wagon in the references.
Cycle through the 16 ordered route stops in README.md, from Kansas River
through The Dalles, and then return to Kansas River. Omit Independence, Missouri
and Willamette Valley, Oregon; retain Independence Rock. Omit "Crossing" from
river names and use compact panel labels where needed. Use a modest animation rate and pause animation while the application
is not in focus. Advance time through the minute tick service independently
of the animation timer. Cancel timers and unsubscribe services at shutdown.

## Implementation

Use native pixel drawing and small sprite patterns rather than raster image
generation, which gives precise edges, a small bundle, and predictable color
mapping. Separate scene drawing from travel/event timing. Scale artwork in
integer pixels where practical; position the clock, scene, and status panel
from display bounds and round-screen geometry.

Animate at two frames per second. Pre-render the four convoy frames into
transparent 4-bit bitmaps. Cache the clock/date until they change and the
status panel until its text changes. Preserve direct rendering as a fallback
when a bitmap cannot be allocated. Reuse one 4-bit bitmap for the current
landmark scenery, rebuilding only when its scenery type changes. Keep event durations independent of the
chosen frame interval.

## Validation

Build all four declared targets with the installed Pebble SDK. Inspect
emulator captures on rectangular and round displays, including walking
frames, passing landmarks, and temporary event text. Check 12/24-hour time
formatting and timer lifecycle. Document build/install commands and animation
behavior in the README. Any unavailable emulator checks must be reported.
