# Animation idle window

Animate at the existing two frames per second for one minute after launch,
or a tap. Returning to focus never renews the minute. Losing focus cancels the frame timer immediately, preserving the remaining
animation budget. Returning to focus resumes only an already active window.
After the minute, keep animating until both the visible landmark and any
current temporary event have finished. Stop scheduling frame timers on the
first quiet frame. Minute ticks still update the clock and refresh the story
text, without moving the wagon or scenery or restarting animation.

Keep animation-window state in the platform-independent Trail model so host
tests can exercise the cutoff, feature completion, and renewal. Reuse the
existing random story generator for idle minute updates. Subscribe to Pebble's
accelerometer tap service, and unsubscribe on shutdown. Retain a single frame
timer through repeated focus and tap events.

Validate the minute boundary, deferred stopping, renewed windows, frozen scene
during text refresh, existing route/events/clock behavior, and all four builds.
