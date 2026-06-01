#!/usr/bin/env python3
import sys

WIDTH = 8
HEIGHT = 16
FIRST = 32
LAST = 126

def parse_bdf(path):
    glyphs = {}
    font_ascent = None
    current_encoding = None
    current_bbx = None
    bitmap = []
    in_bitmap = False

    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.strip()

            if line.startswith("FONT_ASCENT "):
                font_ascent = int(line.split()[1])

            elif line.startswith("ENCODING "):
                current_encoding = int(line.split()[1])

            elif line.startswith("BBX "):
                parts = line.split()
                current_bbx = tuple(int(part) for part in parts[1:5])

            elif line == "BITMAP":
                bitmap = []
                in_bitmap = True

            elif line == "ENDCHAR":
                if current_encoding is not None and FIRST <= current_encoding <= LAST:
                    bitmap_rows = [int(x, 16) for x in bitmap]
                    rows = [0] * HEIGHT

                    if current_bbx is not None and font_ascent is not None:
                        _, glyph_height, _, yoff = current_bbx
                        top = font_ascent - (yoff + glyph_height)
                    else:
                        top = 0

                    for i, row in enumerate(bitmap_rows):
                        dst = top + i
                        if 0 <= dst < HEIGHT:
                            rows[dst] = row

                    glyphs[current_encoding] = rows

                current_encoding = None
                current_bbx = None
                bitmap = []
                in_bitmap = False

            elif in_bitmap:
                bitmap.append(line)

    return glyphs

def emit_header(glyphs, out_path):
    with open(out_path, "w") as out:
        out.write("#pragma once\n")
        out.write("#include <stdint.h>\n\n")
        out.write("#define FONT_WIDTH 8\n")
        out.write("#define FONT_HEIGHT 16\n")
        out.write("#define FONT_FIRST 32\n")
        out.write("#define FONT_LAST 126\n\n")

        out.write("static const uint8_t font8x16[95][16] = {\n")

        for code in range(FIRST, LAST + 1):
            rows = glyphs.get(code, [0] * HEIGHT)
            char = chr(code)

            safe_char = char
            if char == "\\":
                safe_char = "\\\\"
            elif char == "'":
                safe_char = "\\'"
            elif char == "\n":
                safe_char = "\\n"

            out.write(f"    /* {code} '{safe_char}' */ {{ ")
            out.write(", ".join(f"0x{row:02X}" for row in rows))
            out.write(" },\n")

        out.write("};\n")

def main():
    if len(sys.argv) != 3:
        print("usage: bdf_to_header.py input.bdf output.h")
        sys.exit(1)

    glyphs = parse_bdf(sys.argv[1])
    emit_header(glyphs, sys.argv[2])

if __name__ == "__main__":
    main()
