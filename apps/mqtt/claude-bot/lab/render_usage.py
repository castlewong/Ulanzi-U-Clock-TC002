#!/usr/bin/env python3
"""Render a 52x16 Claude Bot usage GIF for TC002.

Takes two cost values (today's USD, this week's USD) and produces a 52x16
animated GIF with the Claude Bot mascot + cost bars.

Usage:
  python3 lab/render_usage.py <today_cost_usd> <week_cost_usd>
  python3 lab/render_usage.py 8.07 74.49

Output (stdout):
  A single-line base64 string of the rendered GIF, suitable for embedding
  in a TC002 Custom App JSON payload.

With --file PATH, also writes the GIF to disk.
"""

import sys
import base64
from io import BytesIO
from pathlib import Path
from PIL import Image, ImageDraw

W, H = 52, 16

BG = (0, 0, 0)
BODY = (219, 116, 86)
PUP = (0, 0, 0)
BAR_FRAME = (88, 90, 96)
BAR_OK = (72, 214, 120)
BAR_WARN = (255, 198, 72)
BAR_DANGER = (255, 76, 88)
BAR_TEXT = (236, 242, 248)

TINY_FONT = {
    "0": ["111", "101", "101", "101", "111"],
    "1": ["010", "110", "010", "010", "111"],
    "2": ["110", "001", "010", "100", "111"],
    "3": ["110", "001", "010", "001", "110"],
    "4": ["101", "101", "111", "001", "001"],
    "5": ["111", "100", "110", "001", "110"],
    "6": ["011", "100", "111", "101", "111"],
    "7": ["111", "001", "010", "010", "010"],
    "8": ["111", "101", "111", "101", "111"],
    "9": ["111", "101", "111", "001", "110"],
    "D": ["111", "101", "101", "101", "111"],
    "H": ["101", "101", "111", "101", "101"],
    "K": ["101", "101", "110", "101", "101"],
    "W": ["10001", "10001", "10101", "11011", "10001"],
    "%": ["101", "001", "010", "100", "101"],
    " ": ["000", "000", "000", "000", "000"],
}

# ── Mascot body pixel art (52×16) ────────────────────────────
CLAYOUT = [
    "....................................................",
    ".................################...................",
    ".................################...................",
    ".................################...................",
    ".................################...................",
    ".................##.##########.##...................",
    ".................##.##########.##...................",
    "..............#####################.................",
    "..............#####################.................",
    "..............#####################.................",
    ".................################...................",
    ".................################...................",
    "..................#..#.....#..#.....................",
    "..................#..#.....#..#.....................",
    "..................#..#.....#..#.....................",
    "....................................................",
]

# ── Eye data ────────────────────────────────────────────────
# Left socket columns 18-19 rows 5-6, right socket columns 29-30 rows 5-6
def eyes(xl, xr):
    return ((xl, 5), (xl, 6), (xr, 5), (xr, 6))

EYE_RIGHT = eyes(19, 30)
EYE_LEFT = eyes(18, 29)
EYE_NONE = ()
SLEEP = ((18, 5), (29, 5))


def draw_frame(pupils=EYE_RIGHT, bob=0, ox=0, hide_legs=None):
    """Draw a single 52x16 mascot frame."""
    img = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(img)

    for y in range(H):
        py = y + bob
        if not (0 <= py < H):
            continue
        for x in range(W):
            cx = x + ox
            if not (0 <= cx < W):
                continue
            if CLAYOUT[y][x] != "#":
                continue
            if hide_legs and (cx, py) in hide_legs:
                continue
            draw.point((cx, py), fill=BODY)

    # Fill eye sockets with body color
    for sx, sy in ((19, 5), (19, 6), (30, 5), (30, 6)):
        qx, qy = sx + ox, sy + bob
        if 0 <= qx < W and 0 <= qy < H:
            draw.point((qx, qy), fill=BODY)

    # Draw pupils
    for px, py in pupils:
        qx, qy = px + ox, py + bob
        if 0 <= qx < W and 0 <= qy < H:
            draw.point((qx, qy), fill=PUP)

    return img


def draw_tiny_text(img, text, x, y, fill=BAR_TEXT):
    draw = ImageDraw.Draw(img)
    cursor = x
    for char in text:
        glyph = TINY_FONT.get(char.upper(), TINY_FONT[" "])
        for gy, row in enumerate(glyph):
            for gx, value in enumerate(row):
                if value == "1":
                    px, py = cursor + gx, y + gy
                    if 0 <= px < W and 0 <= py < H:
                        draw.point((px, py), fill=fill)
        cursor += len(glyph[0]) + 1


def tiny_text_width(text):
    if not text:
        return 0
    total = 0
    for char in text:
        glyph = TINY_FONT.get(char.upper(), TINY_FONT[" "])
        total += len(glyph[0]) + 1
    return max(0, total - 1)


def bar_color_for_cost(cost_usd, period):
    """Return bar color based on daily/weekly cost thresholds."""
    # Codex Plus ~$200/month ≈ $50/week ≈ $7/day
    # Green: under budget, Yellow: near budget, Red: over budget
    if period == "day":
        if cost_usd >= 12:
            return BAR_DANGER
        if cost_usd >= 8:
            return BAR_WARN
        return BAR_OK
    else:  # week
        if cost_usd >= 80:
            return BAR_DANGER
        if cost_usd >= 50:
            return BAR_WARN
        return BAR_OK


def draw_cost_bar(draw, x, y, width, cost, max_cost, fill):
    """Draw a usage bar based on cost (USD). Bar fills proportional to cost vs max_cost."""
    pct = max(0, min(100, int(cost / max_cost * 100)))
    draw.rectangle((x, y, x + width - 1, y + 1), outline=BAR_FRAME)
    inner_w = max(0, round((width - 2) * pct / 100))
    if inner_w:
        draw.line((x + 1, y, x + inner_w, y), fill=fill)


def usage_frame(day_cost, week_cost, pupils=EYE_RIGHT, bob=0):
    """Draw one animation frame of the usage display.
    day_cost: today's Codex cost in USD
    week_cost: this week's Codex cost in USD
    """
    img = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(img)

    mascot = draw_frame(pupils, bob=bob, ox=13)
    img.paste(mascot.crop((25, 0, W, H)), (25, 0))

    # Show as "$X" format
    day_label = f"${day_cost:.0f}"
    week_label = f"${week_cost:.0f}"

    # Bar max values
    day_max = max(float(day_cost), 15.0)  # bar full = $15/day
    week_max = max(float(week_cost), 100.0)  # bar full = $100/week

    draw_tiny_text(img, "5H", 1, 0)
    draw_tiny_text(img, day_label, 24 - tiny_text_width(day_label), 0)
    draw_cost_bar(draw, 1, 6, 23, day_cost, day_max, bar_color_for_cost(day_cost, "day"))

    draw_tiny_text(img, "W", 1, 9)
    draw_tiny_text(img, week_label, 24 - tiny_text_width(week_label), 9)
    draw_cost_bar(draw, 1, 14, 23, week_cost, week_max, bar_color_for_cost(week_cost, "week"))
    return img


def render_usage_gif(day_cost, week_cost):
    """Generate an animated usage GIF and return base64 string.
    day_cost: today's Codex cost (USD)
    week_cost: this week's Codex cost (USD)
    """
    day_cost = max(0, float(day_cost))
    week_cost = max(0, float(week_cost))

    frames = [
        (usage_frame(day_cost, week_cost, EYE_RIGHT, 0), 700),
        (usage_frame(day_cost, week_cost, EYE_LEFT, 0), 520),
        (usage_frame(day_cost, week_cost, EYE_NONE, 0), 160),
        (usage_frame(day_cost, week_cost, EYE_RIGHT, -1), 520),
        (usage_frame(day_cost, week_cost, EYE_LEFT, 0), 620),
    ]

    buf = BytesIO()
    frames[0][0].save(
        buf,
        format="GIF",
        save_all=True,
        append_images=[frame for frame, _duration in frames[1:]],
        duration=[duration for _frame, duration in frames],
        loop=0,
        optimize=False,
    )
    return base64.b64encode(buf.getvalue()).decode("ascii")


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--file")]
    file_path = None
    for a in sys.argv[1:]:
        if a.startswith("--file="):
            file_path = a.split("=", 1)[1]
        elif a == "--file":
            idx = sys.argv.index("--file")
            if idx + 1 < len(sys.argv):
                file_path = sys.argv[idx + 1]

    if len(args) < 2:
        print("Usage: python3 lab/render_usage.py <today_cost_usd> <week_cost_usd> [--file PATH]",
              file=sys.stderr)
        sys.exit(1)

    day_cost = float(args[0])
    week_cost = float(args[1])

    b64 = render_usage_gif(day_cost, week_cost)
    print(b64, end="")

    if file_path:
        raw = base64.b64decode(b64)
        Path(file_path).parent.mkdir(parents=True, exist_ok=True)
        Path(file_path).write_bytes(raw)
        print(f"\n# Wrote GIF to {file_path}", file=sys.stderr)


if __name__ == "__main__":
    main()
