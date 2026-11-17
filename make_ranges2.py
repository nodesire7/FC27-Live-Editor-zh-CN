# -*- coding: utf-8 -*-
"""Build the paired font and 16-bit glyph ranges used by the LE overlay."""

import argparse
import json
import re
import struct
import subprocess
import tempfile
import unicodedata
from pathlib import Path

from fontTools.pens.recordingPen import DecomposingRecordingPen
from fontTools.pens.transformPen import TransformPen
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import TTFont


# Font Awesome code points used by labels in FCLiveEditor.DLL 27.1.9.
ICON_CODEPOINTS = (
    0xE528, 0xE533,
    0xF002, 0xF00B, 0xF00C, 0xF00D, 0xF013, 0xF021, 0xF03A, 0xF03E,
    0xF044, 0xF04B, 0xF058, 0xF05A, 0xF06A, 0xF06E, 0xF070, 0xF071,
    0xF07B, 0xF084, 0xF0B0, 0xF0C1, 0xF0C5, 0xF0C7, 0xF0DA, 0xF0FE,
    0xF120, 0xF121, 0xF12D, 0xF15B, 0xF15C, 0xF1C0, 0xF1C9, 0xF1E3,
    0xF1F8, 0xF2C2, 0xF2D3, 0xF2EA, 0xF362, 0xF51C, 0xF522, 0xF56F,
    0xF5D2, 0xF70C,
)


def add_icons(font, icons):
    icon_cmap = icons.getBestCmap()
    missing = set(ICON_CODEPOINTS) - icon_cmap.keys()
    if missing:
        raise ValueError(f"图标字库缺少 {', '.join(f'U+{cp:04X}' for cp in sorted(missing))}")

    scale = font["head"].unitsPerEm / icons["head"].unitsPerEm
    glyph_set = icons.getGlyphSet()
    glyph_order = list(font.getGlyphOrder())
    unicode_maps = [
        cmap for cmap in font["cmap"].tables if cmap.isUnicode() and cmap.format in (4, 12)
    ]

    # LE's merged icon config selects the legacy ProggyClean ellipsis slot.
    ellipsis = font.getBestCmap()[0x2026]
    for cmap in unicode_maps:
        cmap.cmap[0x0085] = ellipsis

    for cp in ICON_CODEPOINTS:
        icon_name = icon_cmap[cp]
        name = f"le_icon_{cp:04x}"
        drawing = DecomposingRecordingPen(glyph_set)
        glyph_set[icon_name].draw(drawing)
        pen = TTGlyphPen(None)
        drawing.replay(TransformPen(pen, (scale, 0, 0, scale, 0, 0)))
        font["glyf"][name] = pen.glyph()
        font["hmtx"][name] = tuple(round(value * scale) for value in icons["hmtx"][icon_name])
        glyph_order.append(name)
        for cmap in unicode_maps:
            cmap.cmap[cp] = name

    font.setGlyphOrder(glyph_order)

    # The upstream fonts reserve their names under the SIL Open Font License.
    names = font["name"]
    copyright_notice = names.getDebugName(0) + "\nCopyright (c) 2024 Fonticons, Inc. (https://fontawesome.com)"
    replacements = {
        0: copyright_notice,
        1: "LE Han UI",
        3: "LEHanUI-Regular-1",
        4: "LE Han UI Regular",
        6: "LEHanUI-Regular",
        16: "LE Han UI",
        18: "LE Han UI Regular",
        21: "LE Han UI",
        25: "LEHanUI",
    }
    for record in names.names:
        if record.nameID in replacements:
            record.string = replacements[record.nameID]


def build_ranges(font, localize_path):
    translations = json.loads(localize_path.read_bytes())["translations"]
    required = {ord(ch) for text in translations.values() for ch in text if ch.isprintable()}

    available = {
        cp for cp in font.getBestCmap()
        if 0x20 <= cp <= 0xFFFF and unicodedata.category(chr(cp)) not in {"Cc", "Cn", "Co", "Cs"}
    } | set(ICON_CODEPOINTS) | {0x0085}

    legacy = Path(__file__).with_name("data_ranges.bin").read_bytes()
    words = struct.unpack(f"<{len(legacy) // 2}H", legacy)
    common = {cp for a, b in zip(words[:-1:2], words[1:-1:2]) for cp in range(a, b + 1)}
    common.update(ord(bytes((a, b)).decode("gb2312")) for a in range(0xB0, 0xF8) for b in range(0xA1, 0xFF) if bytes((a, b)).decode("gb2312", "ignore"))
    common.update(range(0x20, 0x530))
    common.update(range(0x2000, 0x2700))
    common.update((0x5586, 0x9555, 0xFFFD))
    available &= common | required | set(ICON_CODEPOINTS)

    missing = required - available
    if missing:
        raise ValueError(f"译文包含未覆盖字符 {', '.join(f'U+{cp:04X}' for cp in sorted(missing))}")

    runs = []
    for cp in sorted(available):
        if runs and cp == runs[-1] + 1:
            runs[-1] = cp
        else:
            runs.extend((cp, cp))

    return struct.pack(f"<{len(runs) + 1}H", *runs, 0), len(available)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--font", type=Path, required=True, help="原版 LXGWWenKai-Regular.ttf")
    parser.add_argument("--icons", type=Path, required=True, help="Font Awesome 6 Free fa-solid-900.ttf")
    parser.add_argument("--encoder", type=Path, required=True, help="ImGui binary_to_compressed_c.exe")
    parser.add_argument("--localize", type=Path, required=True, help="生成好的 localize.json")
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()

    with tempfile.TemporaryDirectory() as temp_dir:
        merged = Path(temp_dir) / "LEHanUI-Regular.ttf"
        with TTFont(args.font) as font, TTFont(args.icons) as icons:
            add_icons(font, icons)
            ranges, count = build_ranges(font, args.localize)
            font.save(merged)

        source = subprocess.check_output([args.encoder.resolve(), "-base85", merged, "LEHanUI"])
        font_data = b"".join(re.findall(rb'^\s*"([^"]*)"', source, re.MULTILINE))
        font_data = font_data.replace(rb"\?", b"?")
        if not font_data or len(font_data) % 5:
            raise ValueError("ImGui 编码器未返回有效的 base85 字体")

    args.output_dir.mkdir(parents=True, exist_ok=True)
    (args.output_dir / "FC27LEzh_font.b85").write_bytes(font_data)
    (args.output_dir / "FC27LEzh_ranges.bin").write_bytes(ranges)
    print(f"{count} 个码位，含 {len(ICON_CODEPOINTS)} 个界面图标，字体 {len(font_data)} 字节")


if __name__ == "__main__":
    main()
