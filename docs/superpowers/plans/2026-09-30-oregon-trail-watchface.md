# Oregon Ho Implementation Plan

> Execute inline using the executing-plans skill. Work in the supplied new project, which has no existing commits or application to preserve.

**Goal:** Deliver the approved animated Oregon Trail watchface and a PBW for all four declared color targets.

**Architecture:** A platform-independent travel model owns bounded animation counters, landmark passages, and temporary events. A native renderer draws integer pixel artwork and text. The entry point owns the window, minute service, and focus-aware timer.

**Tech Stack:** C, Pebble SDK 4.33.1, native SDK graphics, host C compiler for model tests.

## Constraints

- Preserve UUID, watchface flag, and basalt/chalk/emery/gabbro target list.
- Use black sky, white canvas, brown wagon, green bottom area.
- Honor system 12/24-hour preference; minute ticks update the clock.
- Keep round-screen time and messages within the visible circle.
- Pause animation when unfocused and cancel services/timers on teardown.

## Task 1: Travel model

Files: `src/c/trail.h`, `src/c/trail.c`, `tests/trail_test.c`.

- [x] Write host assertions for initial quiet travel, passing landmarks, transient events, repeat cycles, and midnight/noon clock formatting.
- [x] Run `cc -std=c99 -Wall -Wextra -Werror -Isrc/c tests/trail_test.c src/c/trail.c -o /tmp/oregon-ho-test`; observe missing implementation.
- [x] Implement `trail_init`, `trail_step`, `trail_event`, and `trail_format_time` using fixed-size state and bounded counters.
- [x] Compile and run the host tests successfully, including long-running travel.

## Task 2: Pixel scene and native watchface

Files: `src/c/scene.h`, `src/c/scene.c`, `src/c/oregon-ho.c`.

- [x] Add a 5x7 bitmap alphabet for time/date/status; use integer scales and word wrapping.
- [x] Draw a single walking ox, a covered wagon and alternating wheel spokes, river/fort/rock landmarks, and scrolling grass.
- [x] Derive layout from display bounds and PBL_ROUND; reserve central space for clock and bordered status.
- [x] Replace the button demo with one canvas, minute ticks, focus-aware 250 ms timer, checked allocations and paired teardown.
- [x] Run project validation and `pebble build`; check all four platform memory summaries and generated PBW.

## Task 3: Runtime inspection and delivery

Files: `README.md`, `previews/*`.

- [x] Install on all four emulators and inspect actual screenshots for text clipping, animation, and color.
- [x] Capture a passing landmark and temporary event; verify 12/24-hour settings on an emulator.
- [x] Capture an animation GIF and deliver PBW plus screenshot links.
- [x] Update README with actual targets, behavior, build commands, and host test command.
- [x] Review source lifecycle and rerun checks after any fixes; mark completed tasks here.
