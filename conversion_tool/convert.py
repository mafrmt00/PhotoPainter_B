# encoding: utf-8
"""
Optimized image converter for E Ink Spectra 6 (E6) 6-color photo frames.
- Uses the "magic" pure-color palette many firmwares require for color recognition
- --mode cut always crops to exactly fill target (no bars)
- Batch processing with --all
- LANCZOS resize + optional mild sharpening
"""

import sys
import os
from PIL import Image, ImageFilter
import argparse


SUPPORTED_EXTS = {'.jpg', '.jpeg', '.png', '.bmp', '.webp', '.gif'}


def is_likely_output(filename):
    """Skip files that look like previous outputs"""
    name = filename.lower()
    return '_scale.bmp' in name or '_cut.bmp' in name


def get_image_files(directory='.'):
    """Return list of supported image files in directory, excluding outputs"""
    files = []
    for f in os.listdir(directory):
        if os.path.isfile(f):
            ext = os.path.splitext(f)[1].lower()
            if ext in SUPPORTED_EXTS and not is_likely_output(f):
                files.append(f)
    return sorted(files)


def process_image(input_path, args):
    """Core processing logic for one image"""
    if not os.path.isfile(input_path):
        print(f"  Skip: {input_path} (not found)")
        return

    # Determine target resolution
    if args.dir == 'landscape':
        target = (800, 480)
    elif args.dir == 'portrait':
        target = (480, 800)
    else:
        try:
            with Image.open(input_path) as im:
                target = (800, 480) if im.width >= im.height else (480, 800)
        except Exception:
            print(f"  Skip: {input_path} (cannot open)")
            return

    target_w, target_h = target

    try:
        orig = Image.open(input_path).convert('RGB')
    except Exception as e:
        print(f"  Skip: {input_path} (open failed: {e})")
        return

    orig_w, orig_h = orig.size

    if args.mode == 'scale':
        ratio = min(target_w / orig_w, target_h / orig_h)
        new_size = (round(orig_w * ratio), round(orig_h * ratio))
        resized = orig.resize(new_size, Image.LANCZOS)

        result = Image.new('RGB', target, (255, 255, 255))
        offset_x = (target_w - new_size[0]) // 2
        offset_y = (target_h - new_size[1]) // 2
        result.paste(resized, (offset_x, offset_y))

    else:  # cut = cover + center crop → always fills exactly
        ratio = max(target_w / orig_w, target_h / orig_h)
        new_w = round(orig_w * ratio)
        new_h = round(orig_h * ratio)
        resized = orig.resize((new_w, new_h), Image.LANCZOS)

        left = (new_w - target_w) // 2
        top = (new_h - target_h) // 2
        result = resized.crop((left, top, left + target_w, top + target_h))

    # Mild sharpening (helps detail — disable if it causes issues)
    if not args.no_sharpen:
        result = result.filter(ImageFilter.UnsharpMask(radius=0.8, percent=140, threshold=2))

    # ────────────────────────────────────────────────
    #   Palette that many photo-frame firmwares expect
    #   (pure colors + duplicated black + padding)
    # ────────────────────────────────────────────────
    pal_image = Image.new("P", (1,1))
    pal_image.putpalette(
        (25,  30,  33,  # #191E21  → dark slate / near-black
        232, 232, 232,  # #e8e8e8  → light gray / near-white
        239, 222, 68,   # #efde44  → bright yellow / gold
        178, 19,  24,   # #b21318  → deep red
        33,  87,  186,  # #2157ba  → deep blue
        18,  95,  32)   # #125f20  → dark green
        + (0,0,0)*250     # pad to 256
    )

    # Quantize to the 7-color palette (Of course Atkinson is not available yet)
    if args.dither == 'floydsteinberg':
        dither_flag = Image.Dither.FLOYDSTEINBERG
    elif args.dither == 'atkinson':
        dither_flag = Image.Dither.ATKINSON
    else:
        dither_flag = Image.Dither.NONE
    quantized = result.quantize(colors=7, method=2, dither=dither_flag, palette=pal_image)

    # Convert back to RGB → 24-bit BMP with only the 7 exact colors
    final = quantized.convert('RGB')

    # Output path
    base, _ = os.path.splitext(input_path)
    out_path = f"{base}_{args.mode}.bmp" if not args.output else args.output

    final.save(out_path)
    print(f"  → {out_path}  ({target_w}×{target_h})")


def main():
    parser = argparse.ArgumentParser(description='Convert image(s) for E Ink Spectra 6 photo frames.')
    parser.add_argument('image_file', type=str, nargs='?', default=None,
                        help='Input image file (ignored when --all is used)')
    parser.add_argument('--all', '-a', action='store_true',
                        help='Convert ALL supported images in current directory')
    parser.add_argument('--dir', choices=['landscape', 'portrait'],
                        help='Force orientation (landscape=800×480, portrait=480×800)')
    parser.add_argument('--mode', choices=['scale', 'cut'], default='scale',
                        help='scale = fit with letterbox | cut = crop to fill')
    parser.add_argument('--dither', type=str, choices=['none', 'floydsteinberg'], default='floydsteinberg',
                        help='Dithering: none (recommended for picky firmwares) | floydsteinberg')
    parser.add_argument('--output', type=str,
                        help='Output filename (only useful for single-file mode)')
    parser.add_argument('--no-sharpen', action='store_true', help='Disable mild pre-sharpening')

    args = parser.parse_args()

    if args.all:
        images = get_image_files()
        if not images:
            print("No supported image files found in current directory.")
            return

        print(f"Batch mode: found {len(images)} image(s) to convert")
        for idx, fname in enumerate(images, 1):
            print(f"[{idx}/{len(images)}] Processing: {fname}")
            process_image(fname, args)
        print("Batch conversion finished.")

    else:
        if not args.image_file:
            parser.error("You must provide an image_file OR use --all")
        print(f"Single file mode: {args.image_file}")
        process_image(args.image_file, args)


if __name__ == '__main__':
    main()