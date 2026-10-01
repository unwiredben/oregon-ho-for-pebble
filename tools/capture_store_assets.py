# Copyright (c) 2026 Ben Combee
# SPDX-License-Identifier: MIT
# See LICENSE in the repository root for the full license text.

"""Capture native-resolution store PNGs and GIFs using the Pebble Tool environment."""
import argparse
import json
import time
from pathlib import Path
from types import SimpleNamespace
from uuid import UUID

from PIL import Image
from libpebble2.communication import PebbleConnection
from libpebble2.protocol.apps import AppRunState, AppRunStateStart, AppRunStateStop
from pebble_tool.commands.screenshot import ScreenshotCommand
from pebble_tool.sdk.emulator import ManagedEmulatorTransport


def capture(command):
    rows = command._grab_processed_image(SimpleNamespace(no_correction=False), show_progress=False)
    image = Image.frombytes("RGBA", (len(rows[0]) // 4, len(rows)),
                            bytes(value for row in rows for value in row))
    # Canonicalize invisible RGB values in the circular mask for PNG/GIF parity.
    return Image.alpha_composite(Image.new("RGBA", image.size), image)


def save_gif(frames, destination):
    # Use one exact palette throughout the animation. Index zero stays transparent,
    # separate from opaque black, so round corners survive GIF conversion.
    def pixels(frame):
        return frame.get_flattened_data() if hasattr(frame, "get_flattened_data") else frame.getdata()

    colors = sorted({pixel[:3] for frame in frames for pixel in pixels(frame) if pixel[3]})
    if len(colors) > 255:
        raise ValueError("Screenshot colors exceed an exact GIF palette")
    indices = {color: index + 1 for index, color in enumerate(colors)}
    palette = [0, 0, 0] + [value for color in colors for value in color]
    palette += [0] * (768 - len(palette))
    indexed = []
    for frame in frames:
        data = bytes(indices[pixel[:3]] if pixel[3] else 0 for pixel in pixels(frame))
        image = Image.frombytes("P", frame.size, data)
        image.putpalette(palette)
        image.info["transparency"] = 0
        indexed.append(image)
    indexed[0].save(destination, save_all=True, append_images=indexed[1:],
                    duration=500, loop=0, disposal=2, transparency=0,
                    background=0, optimize=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk", default="4.33.1")
    parser.add_argument("--output", type=Path, default=Path("store-assets"))
    args = parser.parse_args()
    project = Path(__file__).resolve().parents[1]
    package = json.loads((project / "package.json").read_text())
    app_uuid = UUID(package["pebble"]["uuid"])
    platforms = package["pebble"]["targetPlatforms"]
    output = args.output if args.output.is_absolute() else project / args.output
    commands = {}
    frames = {platform: [] for platform in platforms}

    # Open all connections first, then restart the face together to align events.
    for platform in platforms:
        watch = PebbleConnection(ManagedEmulatorTransport(platform, args.sdk))
        watch.connect()
        watch.run_async()
        command = ScreenshotCommand()
        command.pebble = watch
        commands[platform] = command
        (output / platform).mkdir(parents=True, exist_ok=True)
        watch.send_packet(AppRunState(data=AppRunStateStop(uuid=app_uuid)))
    time.sleep(.25)
    for command in commands.values():
        command.pebble.send_packet(AppRunState(data=AppRunStateStart(uuid=app_uuid)))
    start = time.monotonic() + 1
    stills = {0: "01-watchface.png", 42: "02-river.png", 54: "03-event.png"}
    for index in range(80):
        time.sleep(max(0, start + index * .5 - time.monotonic()))
        for platform, command in commands.items():
            frame = capture(command)
            frames[platform].append(frame)
            if index in stills:
                frame.save(output / platform / stills[index])
        if index in (0, 42, 54, 79):
            print(f"Captured all platforms at {index / 2:.1f}s", flush=True)
    manifest = {}
    for platform, images in frames.items():
        folder = output / platform
        save_gif(images, folder / "preview.gif")
        manifest[platform] = {"width": images[0].width, "height": images[0].height,
                              "duration_ms": 40000, "interval_ms": 500,
                              "files": [*stills.values(), "preview.gif"]}
        print(f"Saved {platform}: three PNGs and animated GIF", flush=True)
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")


if __name__ == "__main__":
    main()
