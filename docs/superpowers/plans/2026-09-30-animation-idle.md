# Animation idle implementation plan

**Goal:** Stop continuous animation after one minute, finishing visible features,
with tap renewal and idle minute story updates.

**Architecture:** Trail owns a remaining-frame budget and the quiet-scene stop
predicate. The Pebble lifecycle schedules frames only while that predicate allows
it and updates idle messages on minute ticks.

**Tech stack:** C99 host tests and Pebble C SDK on basalt, chalk, emery, gabbro.

- [x] Add host tests for the one-minute window, deferred stopping for landmarks
  and events, restarting the window, and story refresh without geometry changes.
  Compile with `cc -std=c99 -Wall -Wextra -Werror -Isrc/c tests/trail_test.c src/c/trail.c -o /tmp/oregon-ho-test`;
  run `/tmp/oregon-ho-test` and confirm the new behavior is absent.
- [x] Add `trail_resume_animation`, `trail_animation_finished`, and
  `trail_refresh_message` in trail.h/trail.c. Consume the animation frame budget
  in trail_step; share the current random story generator with minute refresh.
- [x] Update oregon-ho.c to stop scheduling on a quiet expired window, resume
  on tap, preserve stopped state across focus changes, refresh messages only on idle minute ticks, and unsubscribe taps.
- [x] Document behavior in README.md. Run host tests, sanitizer checks,
  `pebble build`, and inspect `git diff --check` and the resulting diff.

Validation: both host suites passed with AddressSanitizer and UndefinedBehaviorSanitizer
(leak detection disabled because the sandbox uses ptrace). The Pebble build passed
on all four targets with the existing RWX linker warnings. Independent code review
found no concrete issues. No physical-watch battery measurement was performed.

The Pebble skill in `../pebble-skills` was used for structural validation, SDK
verification, build, and emulator testing. `tools/verify_animation.py` passed on
basalt, chalk, emery, and gabbro using temporary flash images. Captures and the
report are in `previews/animation-idle/`. The existing Basalt flash showed a
firmware error screen, so it was preserved and excluded from the test.
The updated PBW was installed successfully via CloudPebble Dev Connect.
