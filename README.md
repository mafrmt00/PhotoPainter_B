# PhotoPainter (B)

A enhanced version of the Waveshare PhotoPainter (B) demo firmware.

This repository is based on the original Waveshare PhotoPainter example and extends it with improved real-color recognition, reliable RTC-backed timekeeping, and configurable display overlays.

---

## What’s New in This Fork

### 1. Real recognition of colors

The firmware now uses a source-driven color classification function in `lib/GUI/GUI_BMPfile.c`.
It analyzes each 24-bit BMP pixel in HSL space and maps it to the e-paper display’s native 6-color palette:
- black
- white
- yellow
- red
- blue
- green

This is not a simple fixed palette remap. The code uses brightness, saturation and hue thresholds to detect real colors from image pixels, so the frame displays more faithful six-color output.

### 2. RTC keeps real time now

The project integrates the `PCF85063` RTC chip via `lib/RTC/waveshare_PCF85063.c`.
Key behavior from the source:
- on boot, the RTC is initialized and its time is validated against the firmware build timestamp
- if the RTC time is older than the build time, it is set from the firmware build date/time
- periodic wake-up alarms are scheduled with `rtcRunAlarm()`
- the displayed overlay is sourced from `PCF85063_GetTime()` so the display can show live current time information

This means the device maintains real wall-clock time and can refresh display content on a timer rather than only at power events.

### 3. Configurable overlay for additional information

The display path in `examples/EPD_7in3e_test.c` supports selectable overlays via `overlayId`:
- `1` = month calendar overlay
- `2` = week calendar overlay
- `3` = current date/time overlay

The main application currently chooses overlay `3` by default, but the source is structured so it can be changed to show different overlay content.

Additional on-screen information includes:
- low-voltage warning text when battery falls below threshold
- battery voltage displayed in the overlay

---

## Repository Structure

- `main.c` — main firmware entry point, RTC alarm and SD-card image playback logic
- `examples/EPD_7in3e_test.c` — BMP display and overlay composition logic
- `lib/GUI/GUI_BMPfile.c` — 6-color BMP reader and real-color pixel classification
- `lib/RTC/waveshare_PCF85063.c` — PCF85063 RTC initialization, epoch/time handling, and alarm scheduling
- `conversion_tool/convert.py` — Python image conversion helper for preparing 6-color BMPs

---

## How It Works

### Display pipeline

1. On startup, the system initializes the Pico hardware, RTC, LEDs, ADC and SD card.
2. It scans the SD card for BMP images in one of three modes:
   - `Mode 0` = auto-scan and sort image filenames
   - `Mode 1` = auto-scan without sorting
   - `Mode 2` = manual file list via `fileList.txt`
3. Each BMP is read via `GUI_ReadBmp_RGB_6Color()` and converted into native e-paper pixels.
4. An overlay is drawn on top of the image using the current RTC time.
5. The frame updates the e-paper display and then enters sleep or waits for the next RTC alarm.

### Charging and low-power behavior

- If USB power is present, the board enters charging mode and keeps refreshing based on the RTC alarm.
- If battery voltage drops below ~3.1V, the firmware disables the RTC alarm and powers down to preserve battery health.

---

## Build and Flash

This project uses the Raspberry Pi Pico SDK and standard CMake build flow.

From the repository root:

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

Then flash the generated UF2 file to the Pico RP2040 board.

---

## Image Conversion

For 6-color BMP conversion, use the helper in `conversion_tool/convert.py`.
It produces frame-ready BMP files using the same six-color palette expected by the firmware.

---

## Original Project Reference

This repo is derived from the Waveshare PhotoPainter (B) example code.
Original documentation and reference materials are available at:
- https://www.waveshare.net/wiki/PhotoPainter
- https://www.waveshare.com/wiki/E-Paper_Floyd-Steinberg

---

## Notes

- The project is designed for the e-paper frame’s 800×480 panel.
- The embedded overlay system is customizable in source by changing `overlayId` in `examples/EPD_7in3e_test.c`.
- RTC and alarm configuration are controlled in `main.c` via `RTC_ALARM_INTERVAL`.
- The real-color recognition logic is implemented in `lib/GUI/GUI_BMPfile.c`.
