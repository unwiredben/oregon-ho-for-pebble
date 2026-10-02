# Copyright (c) 2026 Ben Combee
# SPDX-License-Identifier: MIT
# See LICENSE in the repository root for the full license text.

"""Crop the GIF preview's first convoy frame into square store icons."""
from pathlib import Path

from PIL import Image, ImageDraw


def main():
    root = Path(__file__).resolve().parents[1]
    source = root / "store-assets/emery/preview.gif"
    with Image.open(source) as screenshot:
        screenshot = screenshot.convert("RGB")
        # Keep the complete convoy, excluding the date and progress panel.
        convoy = screenshot.crop((20, 70, 180, 154))
        icon = Image.new("RGB", (160, 160), screenshot.getpixel((0, 70)))
        # Extend the screenshot's sky and ground to make a square composition.
        ImageDraw.Draw(icon).rectangle(
            (0, 106, 159, 159), fill=screenshot.getpixel((0, 154))
        )
        icon.paste(convoy, (0, 26))

    destination = root / "store-assets/icons"
    destination.mkdir(parents=True, exist_ok=True)
    for size in (80, 144):
        icon.resize((size, size), Image.Resampling.NEAREST).save(
            destination / f"icon-{size}x{size}.png", optimize=True
        )


if __name__ == "__main__":
    main()
