# Arcade Stick

Firmware for two DIY Bluetooth arcade sticks (red and blue). Each one uses:
- a Freenove ESP32-S3 2.8" board (FNK0104) with an MCP23017 I/O expander
- a Sanwa JLF stick, 6 × 30 mm and 3 × 24 mm buttons, and the board's speaker

They're built for SteamOS and RetroArch over Bluetooth.

The full manual is in [docs/MANUAL.md](docs/MANUAL.md). It covers wiring, flashing, the menu, shortcuts, the Moves page and sounds.

## Updating a stick

Download files from the [latest release](../../releases/latest).

| Way | What to do |
|---|---|
| **Check for update** (v2.3+) | On the stick: **Menu → Check for update → B4**. It finds the newest release here and installs it. You need to save your home Wi-Fi once first (see the manual). |
| Wi-Fi from a phone | **Menu → Wi-Fi update**, join `ArcadeStick-Red` / `-Blue` (password `arcadestick`), open http://192.168.4.1, upload the `_wifi.bin` file. |
| USB cable | **Menu → USB update**, then flash the `_usb.bin` file at address 0x0. |

Each stick only accepts its own colour's file, because the two are wired differently.

## Making a release

1. Bump `FW_VERSION` in `src/main.cpp`, e.g. `"2.4"`.
2. Run `tools/make_release.sh` to build both sticks into `release/`.
3. Create a GitHub release tagged `v2.4`. Attach all four `.bin` files.

The sticks compare the release tag with their own version. They download the asset whose name ends in `_red_wifi.bin` or `_blue_wifi.bin`.

## Layout

| Path | What |
|---|---|
| `src/main.cpp` | Firmware: input, Bluetooth/USB gamepad, screen, menu, sound, updates |
| `src/moves.h` | Fighting game move lists and the move recogniser |
| `test/test_moves.cpp` | Host tests for the move recogniser: `g++ -std=c++17 -Isrc test/test_moves.cpp && ./a.out` |
| `platformio.ini` | Builds `arcade_stick` (red) and `arcade_stick_blue` |
| `tools/make_release.sh` | Builds the release files |
