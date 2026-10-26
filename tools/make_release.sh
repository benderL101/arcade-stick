#!/usr/bin/env bash
# Builds the firmware and writes the release files into ./release/
#   ArcadeStick_vX.Y_usb.bin    USB cable, address 0x0 (any stick)
#   ArcadeStick_vX.Y_wifi.bin   Wi-Fi update / Check for update (any stick)
# plus two copies of the Wi-Fi file named for sticks still on v2.3, whose
# Check for update looks for *_red_wifi.bin / *_blue_wifi.bin:
#   ArcadeStick_vX.Y_red_wifi.bin, ArcadeStick_vX.Y_blue_wifi.bin
# Attach all four to a GitHub release tagged vX.Y (must match FW_VERSION).
set -euo pipefail
cd "$(dirname "$0")/.."
VER=$(sed -n 's/^#define FW_VERSION *"\(.*\)".*/\1/p' src/main.cpp)
pio run -e arcade_stick
BOOT0=$(find ~/.platformio/packages/framework-arduinoespressif32 -name boot_app0.bin | head -1)
ESPTOOL=~/.platformio/packages/tool-esptoolpy/esptool.py
b=.pio/build/arcade_stick
mkdir -p release
python3 "$ESPTOOL" --chip esp32s3 merge-bin -o "release/ArcadeStick_v${VER}_usb.bin" \
  --flash-mode dio --flash-size 16MB \
  0x0 "$b/bootloader.bin" 0x8000 "$b/partitions.bin" 0xe000 "$BOOT0" 0x10000 "$b/firmware.bin"
cp "$b/firmware.bin" "release/ArcadeStick_v${VER}_wifi.bin"
cp "$b/firmware.bin" "release/ArcadeStick_v${VER}_red_wifi.bin"    # for v2.3 sticks
cp "$b/firmware.bin" "release/ArcadeStick_v${VER}_blue_wifi.bin"   # for v2.3 sticks
ls -l release
echo "Now create release v${VER} on GitHub and attach the four files above."
