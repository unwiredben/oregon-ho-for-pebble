"""Generate the 25-pixel launcher icon from the watchface's wagon design."""
from pathlib import Path

from PIL import Image, ImageDraw


def main():
    image = Image.new("RGBA", (25, 25), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    black = (0, 0, 0, 255)
    white = (255, 255, 255, 255)

    # Rounded covered wagon, with an outline visible on light backgrounds.
    edges = [(8, 17), (6, 19), (5, 20)] + [(4, 21)] * 8 + [(5, 20), (6, 19)]
    for y, (left, right) in enumerate(edges, 3):
        draw.line((left, y, right, y), fill=black)
        if 3 < y < 15:
            draw.line((left + 1, y, right - 1, y), fill=white)
    draw.rectangle((5, 7, 7, 12), fill=black)
    for rib in (11, 16):
        draw.line([(rib - 1, 5), (rib, 7), (rib, 12), (rib - 1, 14)], fill=black)

    # Bed, drawbar, and axle.
    draw.rectangle((5, 16, 20, 18), fill=black)
    draw.line((6, 17, 19, 17), fill=white)
    draw.line((1, 18, 6, 18), fill=black)
    draw.line((8, 20, 18, 20), fill=black)

    wheel = ("..KKK..", ".KWWWK.", "KWWKWWK", "KWKKKWK",
             "KWWKWWK", ".KWWWK.", "..KKK..")
    for left in (5, 15):
        for row, pixels in enumerate(wheel):
            for col, pixel in enumerate(pixels):
                if pixel != ".":
                    draw.point((left + col, 17 + row), fill=black if pixel == "K" else white)

    destination = Path(__file__).resolve().parents[1] / "resources/images/menu_icon.png"
    destination.parent.mkdir(parents=True, exist_ok=True)
    image.save(destination)


if __name__ == "__main__":
    main()
