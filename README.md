# Arcade Stick

Firmware for DIY Bluetooth arcade sticks. One firmware runs on every stick: each stick's colour (and so its Bluetooth name) and wiring are set in its own menu. Each one uses:
- a Freenove ESP32-S3 2.8" board (FNK0104) with an MCP23017 I/O expander
- a Sanwa JLF stick, 6 × 30 mm and 3 × 24 mm buttons, and the board's speaker

They're built for SteamOS and RetroArch over Bluetooth.

The full manual is in [docs/MANUAL.md](docs/MANUAL.md). It covers wiring, flashing, the menu, shortcuts, the Moves page and sounds.

## Updating a stick

Download files from the [latest release](../../releases/latest).

| Way | What to do |
|---|---|
| **Check for update** (v2.3+) | On the stick: **Menu → Check for update → B4**. It finds the newest release here and installs it. You need to save your home Wi-Fi once first (see the manual). |
| Wi-Fi from a phone | **Menu → Wi-Fi update**, join `ArcadeStick-<colour>` (password `arcadestick`), open http://192.168.4.1, upload the `_wifi.bin` file. |
| USB cable | **Menu → USB update**, then flash the `_usb.bin` file at address 0x0. |

The same files work on every stick. New sticks use the standard wiring in the manual.

## Making a release

1. Bump `FW_VERSION` in `src/main.cpp`, e.g. `"2.4"`.
2. Run `tools/make_release.sh` to build into `release/`.
3. Create a GitHub release tagged `v2.4`. Attach all four `.bin` files (`_usb`, `_wifi`, and the `_red_wifi` / `_blue_wifi` copies for sticks still on v2.3).

The sticks compare the release tag with their own version. v2.4+ download the asset ending in `_wifi.bin`. Sticks still on v2.3 look for `_red_wifi.bin` / `_blue_wifi.bin`, which is why the release keeps those copies.

## Layout

| Path | What |
|---|---|
| `src/main.cpp` | Firmware: input, Bluetooth/USB gamepad, screen, menu, sound, updates |
| `src/moves.h` | Fighting game move lists and the move recogniser |
| `test/test_moves.cpp` | Host tests for the move recogniser: `g++ -std=c++17 -Isrc test/test_moves.cpp && ./a.out` |
| `platformio.ini` | Builds `arcade_stick` |
| `tools/make_release.sh` | Builds the four release files |
