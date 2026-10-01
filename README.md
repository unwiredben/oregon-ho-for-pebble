# Oregon Ho!

An Oregon Trail inspired Pebble watchface, based on the progress screens in
`inspiration/`. It uses original pixel artwork drawn directly in C, with a
black sky, green ground, white covered wagon, and one stout ox with horns and
a wooden yoke and a wiggling tail.

The large clock honors the watch's 12/24-hour preference, with the current date
beneath it. The ox walks and the wheels turn at two frames per second. The
16 route stops scroll past in trail order, from Kansas River through The Dalles,
with quiet stretches between landmarks. A bordered panel shows fictional trail progress
and occasional named travelers' illnesses, broken limbs, recoveries, hunting,
bad dreams, foraging, or mishaps. The random pool contains 18 event templates.

The first event appears after about 24 seconds and lasts 12 seconds. Later
events have 40–80 seconds of quiet travel between them. Landmark cycles last
160 seconds, with 72 seconds of scrolling scenery per cycle. These are decorative
stories, not health readings or a playable simulation.

Animation runs while the watchface is visible and pauses when it loses focus.
The clock uses minute ticks independently of animation. Continuous animation
uses more battery than a static watchface; battery life has not been measured
on physical hardware. No phone companion, internet access, or buttons are needed.

The package includes a 25×25 black-and-white wagon launcher icon with a
transparent background. Regenerate it with `python3 tools/generate_icon.py`
(requires Pillow).

Drawing caches the clock/date until they change, pre-renders four transparent
ox-and-wagon frames into packed 4-bit bitmaps, and caches the status panel until
its text changes. A single reusable bitmap caches the current landmark scenery.
Cache allocation failure falls back to direct drawing.
The 500 ms animation timer halves wakeups compared with the original 4 fps loop.
Cached and uncached fixed-scene screenshots matched pixel for pixel on all
four emulators, including a landmark and a wrapped status message.

## Route

1. Kansas River
2. Big Blue River
3. Fort Kearney
4. Chimney Rock
5. Fort Laramie
6. Independence Rock
7. South Pass
8. Green River
9. Fort Bridger
10. Soda Springs
11. Fort Hall
12. Snake River
13. Fort Boise
14. Blue Mountains
15. Fort Walla Walla
16. The Dalles

The route returns to Kansas River after The Dalles. Panel labels use compact
names where needed to fit above the distance line. Scenery includes forts,
rivers, Chimney Rock, Independence Rock, mountain passes, Soda Springs, and
river cliffs at The Dalles. Mileage remains decorative.

## Targets

| Platform | Display | Shape |
| --- | --- | --- |
| basalt | 144 × 168 color | Rectangular |
| chalk | 180 × 180 color | Round |
| emery | 200 × 228 color | Rectangular |
| gabbro | 260 × 260 color | Round |

Status messages use Pebble's regular Gothic 18 font on Basalt and Chalk,
and regular Gothic 28 on Emery and Gabbro.
The larger displays use larger sprites and status text. Round layouts keep the
clock and status panel inside the circular display. Status boxes keep a fixed width on each device and adjust their height to
wrapped text, with balanced spacing above and below the visible lettering.
Basalt and Chalk share a 104-pixel panel width; Emery and Gabbro share 150 pixels.
Rectangular boxes are centered horizontally and vertically in the green ground area.
The date has a clear gap above the wagon canopy, and the ground below the
status box stays clear of small footer text.

## Build and run

```sh
pebble build
pebble install --emulator emery
pebble screenshot --no-open --emulator emery previews/emery.png
```

The build creates `build/oregon-ho.pbw`, containing all four targets. Replace
`emery` with another target to run its emulator.

The SDK 4.33.1 Gabbro emulator sometimes returned to its previous watchface
after a successful install. Sending a separate `AppRunStateStart` packet for
this project's UUID launched the installed face successfully; this was an
emulator launch issue rather than a render or build failure.

## Checks and previews

Host checks exercise repeated travel cycles, all landmarks, temporary event
expiry, and 12/24-hour midnight/noon formatting:

```sh
cc -std=c99 -Wall -Wextra -Werror -Isrc/c \
  tests/trail_test.c src/c/trail.c -o /tmp/oregon-ho-test
/tmp/oregon-ho-test
```

Built with Pebble Tool 5.0.40 and SDK 4.33.1. All four targets were installed
and inspected in emulators. Actual captures are in `previews/`, including a
30-second Emery animation, a passing landmark, and a temporary event. The
12-hour and 24-hour modes were checked in the Emery emulator; boundary hour
formatting is covered by the host test. The host model also passed AddressSanitizer
and UndefinedBehaviorSanitizer checks. No physical-watch test was performed.

The SDK linker reports an RWX load-segment warning on each target; compilation
succeeds with no C compiler warnings.

## Source

- `src/c/oregon-ho.c`: window, timer, focus and clock lifecycle.
- `src/c/trail.c`: travel animation state, landmarks and event timing.
- `src/c/scene.c`: pixel lettering, sprites and display layout.
- `tests/trail_test.c`: platform-independent travel and clock checks.
