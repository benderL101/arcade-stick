#!/usr/bin/env bash
# Builds both sticks and writes the four release files into ./release/
#   ArcadeStick_vX.Y_red_usb.bin   ArcadeStick_vX.Y_red_wifi.bin
#   ArcadeStick_vX.Y_blue_usb.bin  ArcadeStick_vX.Y_blue_wifi.bin
# Attach all four to a GitHub release tagged vX.Y (must match FW_VERSION).
# The sticks' "Check for update" looks for the *_red_wifi.bin / *_blue_wifi.bin files.
set -euo pipefail
cd "$(dirname "$0")/.."
VER=$(sed -n 's/^#define FW_VERSION *"\(.*\)".*/\1/p' src/main.cpp)
pio run -e arcade_stick -e arcade_stick_blue
BOOT0=$(find ~/.platformio/packages/framework-arduinoespressif32 -name boot_app0.bin | head -1)
ESPTOOL=~/.platformio/packages/tool-esptoolpy/esptool.py
mkdir -p release
for pair in arcade_stick:red arcade_stick_blue:blue; do
  env=${pair%%:*}; col=${pair##*:}; b=.pio/build/$env
  python3 "$ESPTOOL" --chip esp32s3 merge-bin -o "release/ArcadeStick_v${VER}_${col}_usb.bin" \
    --flash-mode dio --flash-size 16MB \
    0x0 "$b/bootloader.bin" 0x8000 "$b/partitions.bin" 0xe000 "$BOOT0" 0x10000 "$b/firmware.bin"
  cp "$b/firmware.bin" "release/ArcadeStick_v${VER}_${col}_wifi.bin"
done
ls -l release
echo "Now create release v${VER} on GitHub and attach the files above."
