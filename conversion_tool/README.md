# E Ink Spectra 6 Image Converter

This Python conversion tool prepares images for use with E Ink Spectra 6 (E6) 6-color photo frames.

The converter:

- resizes images to the frame's native resolution
- supports both letterbox scaling and crop-to-fill modes
- applies a firmware-friendly fixed palette with pure color mapping
- optionally performs mild sharpening for better detail
- supports batch conversion for all valid image files in the current directory

## Requirements

- Python 3
- Pillow (`pip install pillow`)

## Supported input formats

- `.jpg`, `.jpeg`, `.png`, `.bmp`, `.webp`, `.gif`

## Usage

From the `conversion_tool` folder, run:

```bash
python convert.py <image_file>
```

### Output

- The processed image is saved as a BMP file next to the original.
- By default the output filename is `<input>_scale.bmp` or `<input>_cut.bmp`.

## Options

```bash
python convert.py --help
```

Key options include:

- `--all`, `-a`
  - Convert all supported images in the current directory.
- `--dir {landscape,portrait}`
  - Force output orientation to `800×480` or `480×800`.
- `--mode {scale,cut}`
  - `scale`: fit image with letterbox padding
  - `cut`: crop image to completely fill the target resolution
- `--dither {none,floydsteinberg}`
  - `none`: no dithering
  - `floydsteinberg`: apply Floyd–Steinberg dithering
- `--output <filename>`
  - Save the result to a custom filename (single-file mode only)
- `--no-sharpen`
  - Disable the tool's default mild sharpening step.

## Examples

Single image conversion with default scaling:

```bash
python convert.py example.jpg
```

Convert all supported images in the current folder:

```bash
python convert.py --all
```

Force portrait output and crop to fill:

```bash
python convert.py example.jpg --dir portrait --mode cut
```

Save output to a custom filename:

```bash
python convert.py example.png --output converted.bmp
```

## Notes

- The converter uses a hard-coded 7-color palette that matches common E Ink photo-frame firmware expectations.
- `--mode cut` always fills the entire frame resolution and may crop image edges.
- `--mode scale` preserves the whole image, adding white bars as needed.
