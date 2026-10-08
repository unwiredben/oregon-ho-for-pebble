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
bad dreams, foraging, or mishaps. The random pool contains 24 event templates.

The first event appears after about 24 seconds and lasts 12 seconds. Later
events have 40–80 seconds of quiet travel between them. Landmark cycles last
160 seconds, with 72 seconds of scrolling scenery per cycle. These are decorative
stories, not health readings or a playable simulation.

Animation runs for one minute after launch or a tap gesture, then stops once
any visible landmark and temporary story have finished. Losing focus pauses
an active window; returning resumes it without starting a new minute. Once
stopped, returning to the watchface leaves it still until a tap or app restart.
The clock and story text update on minute ticks while the scene is still.
Battery life has not been measured on physical hardware. Everyday watchface
operation needs no phone connection, internet access, or buttons. A phone is
needed to change settings; the configuration page needs no hosted server.

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

## Settings (1.3.0)

Open Oregon Ho!'s settings in the Pebble phone app to configure its offline
[Clay](https://github.com/pebble-dev/clay) page:

- Enter up to five party names. Events use only the nonblank names you enter.
  Leave all five fields blank to restore the twelve default travelers.
- Turn on **No guns** to exclude all six shooting stories. The remaining
  eighteen stories include illnesses, recovery, foraging and wildlife sightings.
- Save to apply changes immediately. Any visible story is replaced using the
  new settings; animation keeps its current state.

Fields accept up to 24 characters (UTF-16 code units). Names are trimmed,
control characters removed, and ASCII letters displayed in uppercase. Watch
storage allows 96 UTF-8 bytes per name. Settings survive watchface restarts.
Clay also saves them on the phone and resends them when the companion starts,
including settings saved while the watchface was unavailable.

The page warns live when a name is likely to be too wide. This is a conservative
estimate; the watch uses its actual font metrics to fit each story in a single
box. It tries the complete name and message at Gothic 28, 18, then 14 as needed
(starting at 18 on small watches). Multi-word names can wrap within that box.
Only if the full story cannot fit at Gothic 14 is the name shortened with `...`;
the full saved name and event sentence are preserved. Round-screen panel corners
stay within the display.

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
and regular Gothic 28 on Emery and Gabbro, with Gothic 18/14 as a fallback for
stories whose suffixes need more room.
The larger displays use larger sprites and status text. Round layouts keep the
clock and status panel inside the circular display. Status boxes keep a fixed width on each device and adjust their height to
wrapped text, with balanced spacing above and below the visible lettering.
Basalt and Chalk share a 104-pixel panel width; Emery and Gabbro share 150 pixels.
Rectangular boxes are centered horizontally and vertically in the green ground area.
The date has a clear gap above the wagon canopy, and the ground below the
status box stays clear of small footer text.

## Build and run

```sh
npm install
pebble build
pebble install --emulator emery
pebble screenshot --no-open --emulator emery previews/emery.png
```

The build creates `build/oregon-ho.pbw`, containing all four targets, and copies
it to the project root as `oregon-ho-<version>.pbw` using the version in
`package.json` (for example, `oregon-ho-1.3.0.pbw`). The root copy is refreshed
on every successful build, including builds with no source changes. Replace
`emery` with another target to run its emulator.

The SDK 4.33.1 Gabbro emulator sometimes returned to its previous watchface
after a successful install. Sending a separate `AppRunStateStart` packet for
this project's UUID launched the installed face successfully; this was an
emulator launch issue rather than a render or build failure.

## Checks and previews

Host checks exercise repeated travel cycles, all landmarks, temporary event
expiry, 12/24-hour midnight/noon formatting, custom-only names, blank fallback,
Unicode boundaries/whitespace, display shortening and shooting-event exclusion:

```sh
cc -std=c99 -Wall -Wextra -Werror -Isrc/c \
  tests/trail_test.c src/c/trail.c -o /tmp/oregon-ho-test
/tmp/oregon-ho-test

cc -std=c99 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
  -Itests/sdk -Isrc/c tests/watchface_test.c src/c/trail.c \
  -Wl,--gc-sections -o /tmp/oregon-ho-watchface-test
/tmp/oregon-ho-watchface-test

cc -std=c99 -Wall -Wextra -Werror -Itests/sdk -Isrc/c \
  tests/settings_test.c src/c/settings.c src/c/trail.c \
  -o /tmp/oregon-ho-settings-test
/tmp/oregon-ho-settings-test

cc -std=c99 -Wall -Wextra -Werror -Isrc/c \
  tests/event_layout_test.c src/c/event_layout.c src/c/trail.c \
  -o /tmp/oregon-ho-layout-test
/tmp/oregon-ho-layout-test

node tests/clay_test.js
```

The lifecycle checks exercise the animation cutoff, focus persistence, idle
clock/story ticks, and repeated taps using host substitutes for SDK timers and
redraw requests. The four-target build checks the actual Pebble SDK interfaces.

The settings checks exercise partial and malformed AppMessages, immediate
updates without advancing animation, and reload from watch persistence.
The JavaScript checks use the package's actual Pebble JavaScript bundle for
configuration generation, save/cancel and resending stored settings. An optional
DOM integration check builds the actual Clay HTML, restores saved values, types
into fields and verifies live warnings and serialized values. Install `jsdom`
outside the project and set `NODE_PATH` to its `node_modules` directory to run
`node tests/clay_page_test.js`.

Settings emulator captures and the panel-bounds report are saved under
`previews/settings/`. The check uses isolated flash and phone storage:

```sh
# Run with the Python interpreter that has pebble-tool installed.
python3 tools/verify_settings.py
```

The emulator check installs the current build into temporary flash images,
checks launch animation and landmark completion beyond the first minute,
compares frozen scenes across minute clock/story updates, and sends a tap
to confirm animation restarts. It saves captures in `previews/animation-idle/`:

```sh
# Run with the Python interpreter that has pebble-tool installed.
python3 tools/verify_animation.py
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
- `src/c/settings.c`: AppMessage validation and persistent party/settings data.
- `src/c/scene.c`: pixel lettering, sprites and display layout.
- `src/c/event_layout.c`: single-box font selection before name shortening.
- `src/pkjs/`: Clay page, live overflow advisory and settings resending.
- `tests/trail_test.c`: platform-independent travel and clock checks.
- `tests/watchface_test.c`: animation timer, focus, tap, and minute tick checks.
- `tests/settings_test.c`: settings delivery, malformed input and persistence.
- `tests/event_layout_test.c`: smaller fonts preserve the full event before shortening.
- `tests/clay_test.js`: Clay page logic and phone settings lifecycle.
- `tests/clay_page_test.js`: optional DOM test of the generated Clay HTML.

## License

Copyright (c) 2026 Ben Combee. Licensed under the [MIT License](LICENSE).
