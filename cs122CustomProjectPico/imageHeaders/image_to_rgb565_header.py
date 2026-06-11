#!/usr/bin/env python3
"""Convert a BMP/PNG/JPEG image into a C header with RGB565 pixel data."""

import argparse
import os
import sys

try:
    from PIL import Image
except ImportError:
    print("Error: Pillow is required to run this script. Install it with: pip install pillow")
    sys.exit(1)


def rgb565(r: int, g: int, b: int, swap_bytes: bool = True) -> int:
    """Converts RGB to 16-bit RGB565. Swaps bytes by default to fix 'off-color' TFT issues."""
    val = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
    if swap_bytes:
        # Swap high and low bytes for typical SPI Big-Endian displays
        return ((val & 0xFF) << 8) | ((val >> 8) & 0xFF)
    return val


def make_identifier(name: str) -> str:
    name = os.path.splitext(os.path.basename(name))[0]
    return ''.join(c if c.isalnum() else '_' for c in name).upper()


def write_header(path: str, array_name: str, width: int, height: int, pixels: list[int], use_static: bool = True) -> None:
    guard = f"_{array_name}_H_"
    storage = 'static const' if use_static else 'const'
    
    with open(path, 'w', newline='\n') as f:
        f.write(f"#ifndef {guard}\n")
        f.write(f"#define {guard}\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"#define {array_name}_WIDTH {width}\n")
        f.write(f"#define {array_name}_HEIGHT {height}\n\n")
        f.write(f"{storage} uint16_t {array_name}_PIXELS[{width}U * {height}U] = {{\n")

        # FIX 1: Clean list processing to prevent double commas
        for i in range(0, len(pixels), 12):
            chunk = pixels[i:i+12]
            line_str = ', '.join(f"0x{p:04X}u" for p in chunk)
            
            if i + 12 < len(pixels):
                f.write(f"    {line_str},\n")
            else:
                f.write(f"    {line_str}\n") # No trailing comma on the very last line

        f.write("};\n\n")
        f.write(f"#endif // {guard}\n")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='Convert an image into a C header with RGB565 pixel data.')
    parser.add_argument('input', help='Input image file path (BMP, PNG, JPEG, etc.)')
    parser.add_argument('output', nargs='?', help='Output header file path. Defaults to <input>_rgb565.h')
    parser.add_argument('--name', help='C identifier base name for the constants and array')
    parser.add_argument('--resize', type=int, nargs=2, metavar=('WIDTH', 'HEIGHT'), help='Resize the image to this width and height')
    parser.add_argument('--no-static', action='store_true', help='Do not use static storage for the pixels array')
    parser.add_argument('--no-swap', action='store_true', help='Disable byte swapping (use if colors look wrong again)')
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    input_path = args.input

    if not os.path.isfile(input_path):
        print(f"Error: input file not found: {input_path}")
        return 1

    output_path = args.output
    if output_path is None:
        base = os.path.splitext(os.path.basename(input_path))[0]
        output_path = f"{base}_rgb565.h"

    array_name = args.name or make_identifier(input_path)
    if not array_name:
        array_name = 'IMAGE'

    image = Image.open(input_path)
    if args.resize:
        image = image.resize((args.resize[0], args.resize[1]), Image.LANCZOS)
    image = image.convert('RGB')
    width, height = image.size

    # FIX 3: Warn the user if the width is an odd number (causes skewed images)
    if width % 2 != 0:
        print(f"\n[WARNING] Image width is {width}px (an odd number).")
        print("Many TFT displays will draw odd-width arrays 'skewed' or diagonally misaligned.")
        print("Consider using the resize argument to force an even width: --resize " + str(width - 1) + f" {height}\n")

    pixels = []
    # Loop over pixels
    for y in range(height):
        for x in range(width):
            r, g, b = image.getpixel((x, y))
            # FIX 2: Swap bytes by default to fix the off-color SPI alignment
            pixels.append(rgb565(r, g, b, swap_bytes=not args.no_swap))

    write_header(output_path, array_name, width, height, pixels, use_static=not args.no_static)
    print(f"Wrote RGB565 header: {output_path}")
    print(f"Width={width}, Height={height}, Array={array_name}_PIXELS")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())