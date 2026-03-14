"""
Generate hg_icon.png (256x256) and hg_icon.ico from scratch using Pillow.
Run from the repo root: python tools/gen_icon.py
"""

from PIL import Image, ImageDraw, ImageFont
import os, math

def make_icon_image(size=256):
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # ── colours ────────────────────────────────────────────────────────────
    BG      = (30,  30,  30,  255)   # #1E1E1E  charcoal background
    WHITE   = (245, 245, 245, 255)   # #F5F5F5  open cells
    BLACK_C = (10,  10,  10,  255)   # #0A0A0A  blocked cells
    RED     = (211,  47,  47,  255)  # #D32F2F  accent (single H cell)
    GAP_COL = (30,  30,  30,  255)   # same as BG so gaps read as background

    # rounded background
    r = round(size * 36 / 256)
    draw.rounded_rectangle([0, 0, size - 1, size - 1], radius=r, fill=BG)

    # ── 5×5 grid ────────────────────────────────────────────────────────────
    def px(v):  return round(v / 256 * size)

    cell = px(42)
    gap  = px(3)
    off  = px(19)
    cr   = max(1, round(4 / 256 * size))   # cell corner radius

    # False = open/white, True = blocked/black, str = red accent cell with letter
    PATTERN = [
        [True,  False, False, True,  False],
        [False, False, True,  False, False],
        [False, True,  "H",   "G",   False],   # two red cells: H and G
        [False, False, True,  False, False],
        [False, True,  False, False, True ],
    ]

    for row in range(5):
        for col in range(5):
            x0 = off + col * (cell + gap)
            y0 = off + row * (cell + gap)
            x1 = x0 + cell
            y1 = y0 + cell
            val = PATTERN[row][col]
            if val is True:
                colour = BLACK_C
            elif isinstance(val, str):
                colour = RED
            else:
                colour = WHITE
            draw.rounded_rectangle([x0, y0, x1, y1], radius=cr, fill=colour)

    # ── Letters on the red accent cells ─────────────────────────────────────
    font_size = max(8, round(26 / 256 * size))
    try:
        font = ImageFont.truetype("arialbd.ttf", font_size)
    except OSError:
        try:
            font = ImageFont.truetype("Arial Bold.ttf", font_size)
        except OSError:
            font = ImageFont.load_default()

    for row in range(5):
        for col in range(5):
            val = PATTERN[row][col]
            if isinstance(val, str):
                cx = off + col * (cell + gap) + cell // 2
                cy = off + row * (cell + gap) + cell // 2
                bbox = draw.textbbox((0, 0), val, font=font)
                tw = bbox[2] - bbox[0]
                th = bbox[3] - bbox[1]
                draw.text((cx - tw // 2, cy - th // 2 - bbox[1]), val, font=font, fill=WHITE)

    return img


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    icons_dir  = os.path.join(script_dir, "..", "assets", "icons")
    os.makedirs(icons_dir, exist_ok=True)

    # PNG 256×256
    img256 = make_icon_image(256)
    png_path = os.path.join(icons_dir, "hg_icon.png")
    img256.save(png_path, "PNG")
    print(f"Saved: {png_path}")

    # ICO with multiple sizes (256, 128, 64, 48, 32, 16)
    sizes   = [256, 128, 64, 48, 32, 16]
    ico_imgs = [make_icon_image(s) for s in sizes]
    ico_path = os.path.join(icons_dir, "hg_icon.ico")
    ico_imgs[0].save(
        ico_path,
        format="ICO",
        sizes=[(s, s) for s in sizes],
        append_images=ico_imgs[1:],
    )
    print(f"Saved: {ico_path}")
    print("Done.")


if __name__ == "__main__":
    main()
