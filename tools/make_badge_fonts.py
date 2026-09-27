#!/usr/bin/env python3
"""Generate the LVGL bitmap fonts used by the Hengky badge application.

Three typefaces are involved. The brand guide asks for Frutiger (Latin) and
Source Han Sans / Source Han Serif (Chinese); Frutiger cannot be embedded in a
redistributable firmware, so:

* Latin   -> Source Sans 3 (SIL OFL 1.1) for pages 1-2, and a local copy of
             Frutiger for the dedication page only
* Chinese -> Source Han Sans Heavy (SIL OFL 1.1), i.e. 思源黑体

Frutiger is a Monotype trademark. The `.c` files generated from it are kept out
of version control (see `.gitignore`) and must be deleted before the branch is
published; see `assets/README.md` for the licence note that goes with them.

The exact character inventory is the gift inscription plus printable ASCII; it
is kept here so the conversion stays reproducible.

Usage:
    python3 tools/make_badge_fonts.py --source-dir <folder with the source fonts>
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "assets" / "fonts"

# The whole visible Latin inventory of the application.
LATIN = "0x20-0x7E"
EXTRA_PUNCTUATION = "0x00B7,0x2014"  # middle dot, em dash
EM_DASH = "0x2014"
FULL_WIDTH_COMMA = "0xFF0C"
INSCRIPTION = "致千里送鹅毛礼轻情意重"  # dedication page, 11 Chinese characters
SIGNATURE_HAN = "张子威和朋友们"  # Han characters of the three-line sign-off

# lv_font_conv is fetched through npx. The certificate override works around a
# local TLS-interception setup; it is scoped to these subprocess calls.
NPX_ENV = {**os.environ, "NODE_EXTRA_CA_CERTS": os.environ.get("NODE_EXTRA_CA_CERTS", "/etc/ssl/cert.pem")}
NPX = ["npx", "--yes", "--registry=https://registry.npmjs.org", "lv_font_conv"]

# name, source file, glyph args, fallback symbol (None = no fallback).
#
# The dedication page mixes Latin and Chinese inside single strings ("致 Hengky",
# "张子威和SMT朋友们"), so each Latin face carries an explicit fallback that
# supplies the Han glyphs. `--lv-fallback` bakes that link into the
# Flash-resident descriptor at generation time, which is why no code has to
# rewrite a const font at runtime.
FONTS = (
    ("badge_sans_11", "SourceSans3-Regular.ttf", f"--range {LATIN},{EXTRA_PUNCTUATION}", None),
    ("badge_sans_14", "SourceSans3-Regular.ttf", f"--range {LATIN},{EXTRA_PUNCTUATION}", None),
    ("badge_sans_18", "SourceSans3-Regular.ttf", f"--range {LATIN},{EXTRA_PUNCTUATION}", None),
    ("badge_sans_bold_22", "SourceSans3-Semibold.ttf", f"--range {LATIN},{EXTRA_PUNCTUATION}", None),
    # Dedication page. This copy of Frutiger carries neither U+00B7 nor the
    # full-width comma, and the dedication needs only the em dash on top of
    # printable ASCII, so the Latin faces request exactly that.
    ("badge_frutiger_20", "Frutiger_bold.ttf", f"--range {LATIN},{EM_DASH}", "badge_han_heavy_20"),
    ("badge_frutiger_13", "Frutiger_bold.ttf", f"--range {LATIN},{EM_DASH}", "badge_han_heavy_11"),
    ("badge_han_heavy_20", "SourceHanSansCN-Heavy.otf",
     f"--range {FULL_WIDTH_COMMA} --symbols {INSCRIPTION}", None),
    ("badge_han_heavy_11", "SourceHanSansCN-Heavy.otf",
     f"--symbols {SIGNATURE_HAN}", None),
)

# Latin/Han pairs drawn inside the same text line. See unify_metrics().
METRIC_PAIRS = (
    ("badge_frutiger_20", "badge_han_heavy_20"),
    ("badge_frutiger_13", "badge_han_heavy_11"),
)

# Code points each generated face is expected to carry, for the coverage check.
EXPECTED = {
    "badge_frutiger_20": [chr(c) for c in range(0x20, 0x7F)] + ["\u2014"],
    "badge_frutiger_13": [chr(c) for c in range(0x20, 0x7F)] + ["\u2014"],
    "badge_han_heavy_20": list(INSCRIPTION) + ["\uff0c"],
    "badge_han_heavy_11": list(SIGNATURE_HAN),
}

# Commercial faces whose generated output must stay out of version control.
NON_REDISTRIBUTABLE = {"Frutiger_bold.ttf"}

COMMERCIAL_BANNER = """/* This subset was generated from a user-supplied copy of Frutiger, a commercial
 * typeface owned by Monotype. It is used for the private Hengky badge gift only.
 * Do not redistribute: keep this file out of version control (see .gitignore)
 * and delete it before the branch is published. Details in assets/README.md.
 */
"""


def convert(name: str, source: Path, size: int, glyph_args: str, fallback: str | None) -> Path:
    output = OUT_DIR / f"{name}.c"
    cmd = NPX + [
        "--font", str(source),
        "--size", str(size),
        "--bpp", "4",
        "--format", "lvgl",
        "--no-compress",
        "--lv-font-name", name,
        "--lv-include", "lvgl.h",
        "--output", str(output),
    ]
    if fallback:
        cmd += ["--lv-fallback", fallback]
    cmd += glyph_args.split()
    result = subprocess.run(cmd, env=NPX_ENV, capture_output=True, text=True)
    if result.returncode != 0:
        sys.stderr.write(result.stdout + result.stderr)
        raise SystemExit(f"lv_font_conv failed for {name}")
    if result.stderr.strip():
        print(f"    note: {result.stderr.strip().splitlines()[-1]}")

    text = output.read_text()
    if source.name in NON_REDISTRIBUTABLE:
        output.write_text(COMMERCIAL_BANNER + text)
    return output


def read_metrics(path: Path) -> tuple[int, int]:
    text = path.read_text()
    line_height = int(re.search(r"\.line_height = (\d+),", text).group(1))
    base_line = int(re.search(r"\.base_line = (-?\d+),", text).group(1))
    return line_height, base_line


def write_metrics(path: Path, line_height: int, base_line: int) -> None:
    text = path.read_text()
    text = re.sub(r"\.line_height = \d+,", f".line_height = {line_height},", text, count=1)
    text = re.sub(r"\.base_line = -?\d+,", f".base_line = {base_line},", text, count=1)
    path.write_text(text)


def unify_metrics(name_a: str, name_b: str) -> None:
    """Give a Latin face and its Han fallback identical vertical metrics.

    LVGL places every glyph on the baseline of the font that resolved it, using
    that font's own `line_height` and `base_line` (see `lv_draw_label.c` and
    `lv_draw_sw_letter.c`), so two fonts only share a baseline when both numbers
    agree. lv_font_conv derives them from each source font's ascent and descent,
    and Frutiger and Source Han Sans disagree -- left alone, the Han characters
    would sit a pixel or two off the Latin ones.

    Glyph offsets are baseline-relative and stay valid, so rewriting the two
    descriptor fields is enough; the larger of the two line boxes is kept so
    nothing gets clipped.
    """
    path_a, path_b = OUT_DIR / f"{name_a}.c", OUT_DIR / f"{name_b}.c"
    lh_a, bl_a = read_metrics(path_a)
    lh_b, bl_b = read_metrics(path_b)

    base_line = max(bl_a, bl_b)
    line_height = max(lh_a + (base_line - bl_a), lh_b + (base_line - bl_b))
    if (lh_a, bl_a) == (line_height, base_line) and (lh_b, bl_b) == (line_height, base_line):
        print(f"    {name_a} / {name_b}: metrics already match ({line_height}/{base_line})")
        return

    print(f"    {name_a} {lh_a}/{bl_a} + {name_b} {lh_b}/{bl_b}"
          f" -> both {line_height}/{base_line}")
    write_metrics(path_a, line_height, base_line)
    write_metrics(path_b, line_height, base_line)


def coverage_of(path: Path) -> set[int]:
    """Collect the code points a generated font actually carries.

    Reads the LVGL font description back: `FORMAT0_TINY` cmaps are one
    contiguous run starting at `range_start`, while `SPARSE_TINY` cmaps store
    offsets *relative to* `range_start` in `unicode_list`. See
    `lv_font_fmt_txt.c` for both layouts.
    """
    text = path.read_text()
    lists: dict[str, list[int]] = {}
    for match in re.finditer(r"static const uint16_t (\w+)\[\] = \{(.*?)\};", text, re.S):
        lists[match.group(1)] = [int(v, 16) for v in re.findall(r"0x[0-9a-fA-F]+", match.group(2))]

    covered: set[int] = set()
    for cmap in re.finditer(r"\.range_start = (\d+), \.range_length = (\d+)(.*?)\.type = (LV_FONT_FMT_TXT_CMAP_\w+)",
                            text, re.S):
        start, length, body, kind = int(cmap.group(1)), int(cmap.group(2)), cmap.group(3), cmap.group(4)
        if kind == "LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY":
            covered.update(range(start, start + length))
        else:
            list_name = re.search(r"\.unicode_list = (\w+),", body).group(1)
            covered.update(start + offset for offset in lists[list_name])
    return covered


def check_coverage() -> None:
    """Fail the run if a generated face is missing a code point it must render.

    A font subset that silently drops a glyph renders as a placeholder box on the
    device; the build stays green and only the panel shows it. This is the
    repeatable check the coding conventions ask for next to the generator.
    """
    for name, wanted in EXPECTED.items():
        covered = coverage_of(OUT_DIR / f"{name}.c")
        missing = sorted({c for c in wanted if ord(c) not in covered})
        if missing:
            raise SystemExit(f"{name}: missing glyphs {missing}")
        print(f"    {name}: {len(wanted)} code points checked, all covered")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-dir", type=Path, required=True,
                        help="folder holding the source .ttf/.otf files")
    args = parser.parse_args()

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    if shutil.which("npx") is None:
        raise SystemExit("npx not found; install Node.js")

    for name, source_name, glyph_args, fallback in FONTS:
        source = args.source_dir / source_name
        if not source.is_file():
            raise SystemExit(f"missing source font: {source}")
        print(f"  {name} <- {source_name}")
        convert(name, source, int(name.rsplit("_", 1)[1]), glyph_args, fallback)

    for name_a, name_b in METRIC_PAIRS:
        unify_metrics(name_a, name_b)

    check_coverage()
    print(f"Fonts written to {OUT_DIR}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())