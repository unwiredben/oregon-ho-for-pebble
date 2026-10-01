# Oregon Ho! store assets

`icons/icon-80x80.png` and `icons/icon-144x144.png` are square store icons
cropped from the Emery screenshot, with extended sky and ground. Nearest-neighbor
scaling preserves the pixel edges. Regenerate them with
`python3 tools/generate_store_icons.py` after capturing screenshots.

Each platform folder contains four unframed assets captured from the current
watchface running in the Pebble emulator:

- `01-watchface.png`: time, date, wagon, and trail progress.
- `02-river.png`: the approaching river with its S bend and broad mouth.
- `03-event.png`: a random traveler event.
- `preview.gif`: a 40-second loop at two frames per second, including walking,
  turning wheels, a wiggling tail, river movement, and an event appearing and expiring.

| Platform | Native dimensions |
| --- | --- |
| Basalt | 144 × 168 |
| Chalk | 180 × 180 |
| Emery | 200 × 228 |
| Gabbro | 260 × 260 |

PNG and GIF assets for round watches retain transparent corners. GIFs use a
shared palette with the original screenshot colors.

To regenerate, install the latest PBW on each target emulator and run
`tools/capture_store_assets.py` with the Python environment containing Pebble Tool,
libpebble2, and Pillow. The script reads target platforms and app UUID from
`package.json`; SDK 4.33.1 is the default.
