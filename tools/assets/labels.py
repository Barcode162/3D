# Draws the bottle labels (the remake, J35.4): 16 labels in a 4 x 4 picture
# (1024 x 1024, 256 a label), each a made-up brand (no real company's name
# or look: Charter), with its kind, a small line of details and an emblem,
# in real type (DejaVu and Liberation, free fonts that allow this) instead
# of blocks. Run with the system Python (it needs Pillow), before the
# models that use props_lib's labels:
#   python3 tools/assets/labels.py <texture dir>      → <texture dir>/bottle_labels.png
import os
import sys

from PIL import Image, ImageDraw, ImageFont

FONTS = "/usr/share/fonts/truetype"
SERIF_B = f"{FONTS}/dejavu/DejaVuSerif-Bold.ttf"
SERIF = f"{FONTS}/dejavu/DejaVuSerif.ttf"
SANS = f"{FONTS}/dejavu/DejaVuSans.ttf"
SANS_B = f"{FONTS}/dejavu/DejaVuSans-Bold.ttf"
SCRIPT = f"{FONTS}/liberation/LiberationSerif-Italic.ttf"

# (brand, kind, details, paper, ink, accent, style)
LABELS = [
    ("KESTREL RIDGE", "Straight Bourbon Whiskey", "AGED 8 YEARS · 45% VOL", "#e8dcc0", "#2a1a0e", "#9b2d1f", 0),
    ("HALVARD", "Single Malt Scotch", "12 YEARS · 70 CL · 40%", "#14110f", "#d9b56a", "#d9b56a", 1),
    ("CORVANE", "London Dry Gin", "DISTILLED · 47% VOL", "#f2f0e8", "#1f3b5a", "#3f7f6a", 2),
    ("DUSKMERE", "Premium Vodka", "TRIPLE FILTERED · 40%", "#e9eef2", "#202428", "#8a9aa8", 3),
    ("MIREVAL", "Tequila Reposado", "100% AGAVE · 38% VOL", "#b8432b", "#f4e6c8", "#f4e6c8", 0),
    ("PELLICOTT", "Dark Spiced Rum", "CARIBBEAN · 40% VOL", "#20160f", "#e2a650", "#b84a24", 1),
    ("THORNQUIST", "Fine Cognac V.S.O.P.", "GRANDE CHAMPAGNE · 40%", "#efe5cc", "#3a2412", "#a8842e", 2),
    ("ASHGROVE", "Irish Whiskey", "TRIPLE DISTILLED · 40%", "#1e3d2a", "#efe2b8", "#c9a24a", 3),
    ("BELLWYN", "Amaretto Liqueur", "ALMOND · 28% VOL", "#7a1d22", "#f1ddb0", "#e0b860", 0),
    ("OSTRANDER", "Kentucky Rye", "SMALL BATCH · 50% VOL", "#d9c79a", "#2d1d10", "#5a2e14", 1),
    ("SALTMARSH", "Coastal Gin", "SEA HERBS · 43% VOL", "#d7e6e2", "#173a3a", "#2f6f6a", 2),
    ("VERENNA", "Aperitivo Rosso", "BITTER · 25% VOL", "#c9302c", "#fff2dc", "#ffd27a", 3),
    ("QUILLON", "Triple Sec", "ORANGE · 40% VOL", "#f3a33c", "#3a1c08", "#ffffff", 0),
    ("HARROWGATE", "Blended Scotch", "SMOOTH · 70 CL · 40%", "#2b2b30", "#e8e3d6", "#b0222a", 1),
    ("COPPERLINE", "Craft Vodka", "COPPER POT · 41% VOL", "#c87a45", "#1b120c", "#fbe3c8", 2),
    ("OLD LOOPWOOD", "Reserve Whiskey", "J PRODUCTION · EST. 2024", "#ebe3cf", "#1d1a17", "#7d1b1b", 3),
]

W, H = 187, 256  # drawn at the band's shape (a little taller than wide) ...
CELL = 256       # ... then fitted to a square cell


def fit(draw, text, font_path, max_w, size):
    while size > 8:
        f = ImageFont.truetype(font_path, size)
        if draw.textlength(text, font=f) <= max_w:
            return f
        size -= 1
    return ImageFont.truetype(font_path, 8)


def label(brand, kind, details, paper, ink, accent, style):
    im = Image.new("RGB", (W * 2, H * 2), paper)  # 2x, then down: smooth edges
    d = ImageDraw.Draw(im)
    w, h = W * 2, H * 2
    # Border: double rule, or a thick band, by style.
    if style in (0, 2):
        d.rectangle([8, 8, w - 9, h - 9], outline=ink, width=4)
        d.rectangle([18, 18, w - 19, h - 19], outline=accent, width=2)
    else:
        d.rectangle([0, 0, w, 34], fill=accent)
        d.rectangle([0, h - 34, w, h], fill=accent)
    # Emblem: a ring with a star or initial.
    cx, cy, r = w // 2, int(h * 0.24), 46
    d.ellipse([cx - r, cy - r, cx + r, cy + r], outline=ink, width=4)
    d.ellipse([cx - r + 10, cy - r + 10, cx + r - 10, cy + r - 10], outline=accent, width=2)
    f = ImageFont.truetype(SERIF_B, 44)
    d.text((cx, cy + 2), brand[0], font=f, fill=ink, anchor="mm")
    # Brand name.
    fb = fit(d, brand, SERIF_B, w - 50, 64)
    d.text((cx, int(h * 0.5)), brand, font=fb, fill=ink, anchor="mm")
    d.line([60, int(h * 0.58), w - 60, int(h * 0.58)], fill=accent, width=3)
    # Kind, in italic.
    fk = fit(d, kind, SCRIPT, w - 50, 40)
    d.text((cx, int(h * 0.66)), kind, font=fk, fill=ink, anchor="mm")
    # Details, small capitals.
    fd = fit(d, details, SANS, w - 60, 22)
    d.text((cx, int(h * 0.8)), details, font=fd, fill=ink, anchor="mm")
    return im.resize((CELL, CELL), Image.LANCZOS)


def main():
    out_dir = sys.argv[1]
    atlas = Image.new("RGB", (CELL * 4, CELL * 4))
    for k, spec in enumerate(LABELS):
        r, c = divmod(k, 4)
        # props_lib reads cell (r, c) with v up from the bottom: row 0 is the
        # picture's bottom row.
        atlas.paste(label(*spec), (c * CELL, (3 - r) * CELL))
    os.makedirs(out_dir, exist_ok=True)
    path = os.path.join(out_dir, "bottle_labels.png")
    atlas.save(path)
    print("LABELS", path)


main()
