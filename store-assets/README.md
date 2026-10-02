# Oregon Ho! 1.2.0 store assets

Each platform folder contains two assets captured from version 1.2.0 running
in the Pebble emulator:

- `03-event.png`: a traveler event, with the updated ox and diagonal yoke.
- `preview.gif`: a 40-second loop at two frames per second, showing walking,
  turning wheels, a swishing tail, river movement, and an event appearing and expiring.

| Platform | Native dimensions |
| --- | --- |
| Basalt | 144 × 168 |
| Chalk | 180 × 180 |
| Emery | 200 × 228 |
| Gabbro | 260 × 260 |

Round-watch assets retain transparent corners. GIFs use a shared palette with
the original screenshot colors. `manifest.json` lists the files and dimensions;
`validation.json` records the checked GIF duration, frames, and sizes.

`icons/icon-80x80.png` and `icons/icon-144x144.png` show the updated ox and wagon.
They use the first frame of the Emery GIF, cropped and scaled with crisp pixel
edges. Regenerate them with `python3 tools/generate_store_icons.py` after capture.

`oregon-ho-store-assets.zip` contains the eight platform assets, both icons,
and their metadata.

To regenerate, build and install the latest PBW on each target emulator, then run
`tools/capture_store_assets.py --event-only` with the Python environment containing
Pebble Tool, libpebble2, and Pillow. The script reads target platforms and app UUID
from `package.json`; SDK 4.33.1 is the default.
