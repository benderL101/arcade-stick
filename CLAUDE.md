# CLAUDE.md: Arcade Stick firmware

Firmware for Ben's DIY Bluetooth arcade sticks (a blue one, a red one, and more to come). **One firmware runs on every stick.** Each stick's colour (which sets its accent, its Bluetooth/USB name "Arcade Stick <Colour>" and its Wi-Fi name) and its wiring are picked in the stick's own menu and stored on the stick.

## Who uses this and how

- Owner: Ben, in Australia. Use metric units and plain language, and give step-by-step instructions for anything he does by hand.
- He flashes from a **Windows PC**. He plays on a **Legion Go S running SteamOS** over Bluetooth, with **RetroArch** on the TV.
- He doesn't edit code. Every change ships as built `.bin` files plus clear install steps.
- Keep to his decisions:
  - No extra connection modes. Plain Bluetooth works fine.
  - RetroArch shortcuts live on **SELECT** (hold 500 ms).
  - **HOME** tap = Home button. HOME held 1 s = stick menu.

## Hardware

| Part | Details |
|---|---|
| Board | Freenove ESP32-S3 2.8" (FNK0104, non-touch): 16 MB flash, OPI PSRAM, ILI9341 320×240 via LovyanGFX |
| Inputs | MCP23017 at I2C 0x20, SDA 16 / SCL 15, 400 kHz. **GPA7/GPB7 unused**: they're output-only on current silicon. |
| Audio | ES8311 codec at I2C 0x18 (same bus). I2S: MCLK 4, BCLK 5, WS 7, DOUT 8. Amplifier enable GPIO 1, **active LOW**. Synth runs at 24 kHz. |
| Other | WS2812 on GPIO 42. Battery ADC on GPIO 9 (voltage ÷ 2, 1S LiPo). Backlight on GPIO 45. |
| Controls | Sanwa JLF stick, 6 × 30 mm buttons (B1–B3 top row, B4–B6 bottom row), and 3 × 24 mm buttons (START, SELECT, HOME). EX is an optional 7th. |

Pin maps are in `src/main.cpp`:
- `MCP_BIT_STD[]`: the **standard wiring** (the blue stick's). Every new stick is wired to this.
- `MCP_BIT_RED[]`: the original red stick, picked with **Menu → Wiring → Old red stick**.

Both reflect how real sticks are **actually wired**, so never "tidy" them. START, SELECT and HOME are on the same pins in both, so the menu always opens. `MCP_BIT` / `PHYS_PIN` are pointers set by `applyWiring()`.

### Stick identity
- Stored in NVS namespace `stick` (keys `wiring`, `colour`), **not** in `Settings`, so settings copies never carry it.
- `COLOUR_NAME[]` / `COLOUR_RGB[]`: index 0 Blue (default), 1 Red, then Green, Yellow, Orange, Purple, Pink, White. Only append to this list, so saved indexes keep their meaning.
- First boot of v2.4+ with no `stick` keys: `scanOldFirmware()` reads the other OTA slot (the firmware it replaced). An old build with only `ASID:RED!` → Old red wiring + Red. Only `ASID:BLUE` → Standard + Blue. Anything else → Standard + Blue.
- `blePad` is created in `setup()` (a pointer), because its name comes from the colour. Changing colour restarts the stick when the menu closes.

## Build

```bash
pio run -e arcade_stick        # the one firmware
tools/make_release.sh          # writes the four release files to release/
g++ -std=c++17 -Wall -Isrc test/test_moves.cpp -o /tmp/tm && /tmp/tm   # move recogniser tests (must be ALL PASSED)
```

- Platform: pioarduino `55.03.30-2`, which is Arduino-ESP32 3.3.0. Partitions are `default_16MB.csv`, with app0/app1 for OTA.
- Libraries are pinned in `platformio.ini`: LovyanGFX 1.2.32, ESP32-BLE-Gamepad 0.8.0, NimBLE-Arduino 2.5.1. Don't bump them casually.
- USB merged bin: build it with `--flash-mode dio --flash-size 16MB`. The parts go at bootloader 0x0, partitions 0x8000, boot_app0 0xe000 and app 0x10000. `make_release.sh` already does this.

## Releasing (how updates reach the sticks)

1. Bump `FW_VERSION` in `src/main.cpp`, and the version in the header comment.
2. Run `tools/make_release.sh`. Then run the tests.
3. Check the tags: every `.bin` must contain `ASID:STK1`, `ASID:RED!` and `ASID:BLUE` (`grep -c` gives 1 each). STK1 is what v2.4+ accepts; the two old tags let sticks still on v2.3 accept the update and let the first boot recognise the red stick.
4. Commit and push to `main`.
5. **Ben publishes the GitHub release himself.** Cloud Claude sessions can't create releases or upload release files: the API is blocked. So send him the four files and tell him to:
   - create a release tagged with the version (`v2.4` or `2.4` both work)
   - attach all four files: `_usb.bin`, `_wifi.bin`, and the identical `_red_wifi.bin` / `_blue_wifi.bin` copies (for sticks still on v2.3)
6. Update `docs/MANUAL.md` for anything user-visible. It also ships as `README.md` in the zip package.

### Rules the on-stick updater depends on

**Menu → Check for update** (`onlineTask` in `main.cpp`) works like this:
- It joins the home Wi-Fi saved in NVS namespace `net`.
- It GETs `api.github.com/repos/benderL101/arcade-stick/releases/latest`.
- It compares the tag with `FW_VERSION` using `verNum()`, which skips non-digits.
- It downloads the asset whose URL ends in `_wifi.bin`, preferring one without `_red_`/`_blue_`. Matching is case-insensitive. (v2.3 sticks look for `_red_wifi.bin` / `_blue_wifi.bin`.)

So:
- **Asset names must keep the pattern** `ArcadeStick_vX.Y_<usb|wifi>.bin`, plus the `_red_wifi` / `_blue_wifi` copies while any stick may still be on v2.3.
- The tag must be a higher number than the installed version, or the stick reports "Up to date".
- HTTPS is verified with the root CA bundle built into the core (`_binary_x509_crt_bundle_start`). Don't switch to `setInsecure()`.
- Before installing, every Wi-Fi install (phone upload or GitHub) checks three things:
  - byte 0 is `0xE9`
  - the app descriptor magic `0xABCD5432` is at bytes 32–35, which rejects the USB merged bin
  - the file contains `ASID:STK1`, i.e. it's Arcade Stick firmware v2.4 or later

  `Update.end(true)` then verifies the image checksum.
- Keep the repo **public**. A private repo would need a token inside the firmware.

## Code map (`src/main.cpp`, about 2 300 lines, one file)

- **Inputs:** `Phys` enum, `MCP_BIT[]`, and the profile output maps.
  - `PROFILE_ORDER {4,0,1,2,3}` gives the menu order: SteamOS, Fighting, SNES, Mega Drive, Custom.
- **Report building:** `buildReport()`, with SOCD, turbo and stick mode.
  - The SELECT layer is `selLayer` plus `HOTKEYS[]`. SELECT is sent along with the hotkey target, and buttons used in the layer are suppressed until released.
  - RetroArch's Hotkey Enable is bound to SELECT.
- **Settings:** `struct Settings` lives in Preferences. Its `version` field is 2. **Only ever append new fields at the end.** `loadSettings()` reads partial lengths, so updates keep old settings. Never reorder fields or change their sizes.
  - The settings file has magic `ASTK`. Loading a file from another stick keeps the local `player` and `conn`. `theme` is unused since v2.4.
- **UI:**
  - `enum Screen` holds the screens.
  - `enum MenuItem` and `MENU_LABEL[]` must stay in the same order. Add an item to both lists, plus `menuValue()`, `menuSelect()` and the `S_MENU` right-press list.
  - Drawing is `render()` into a PSRAM sprite. Set `uiDirty = true` to request a redraw.
- **Pages on the status screen:** Buttons, Inputs (history in frames), and Moves.
- **Sound:** `playSound(SND_*)` and `soundEvents()`.
  - Note constants use an `F_` prefix, because `A2`–`A6` clash with Arduino pin names.
- **Power:**
  - Attract mode after 30 s idle.
  - Screen off after `SLEEP_MIN`.
  - Auto power off when idle and not connected.
  - START held 5 s turns the stick off.
- **Wi-Fi:**
  - `wifiStart()`: phone access point `ArcadeStick-<Colour>`, password `arcadestick`, page at 192.168.4.1. The page handles OTA upload, settings copy and the home Wi-Fi form.
  - `onlineStart()` / `onlineTask()`: the GitHub updater, running as its own 16 KB task.
- **Bluetooth shutdown:** both Wi-Fi modes call `NimBLEDevice::deinit()` and set `radioOff`. **Any Bluetooth call must check `radioOff` first**, or the stick crashes. The stick restarts when you leave either Wi-Fi mode.
- **USB flash mode without opening the case:** `usb_persist_restart(RESTART_BOOTLOADER)`, reached from Menu → USB update.

## Move lists (`src/moves.h`)

- Plain C++ with no Arduino dependencies, so it's tested on the host.
- Data types: `Game` → `Fighter` → `Move {name, motion, btn}`.
- Motions use numpad notation (6 = forward, so moves mirror automatically when facing left):
  - `236`: a sequence of directions
  - `[4]6`: charge back, then forward
  - `360`: a full circle
  - `tap`: rapid presses
  - `hold`: hold the button, then let go
  - a single digit: hold that direction
- Button specs:
  - `P` / `PP` / `PPP` / `K`…: any punch or kick, counted by class
  - a named token such as `LP`, `HP+LP` or `BL+LK`: that exact button
  - Each game's `btnName[6]` maps B1–B6.
- Games so far:
  - SF II (16 fighters), SF Alpha 3 (14, roster stops at Fei Long), MK (7), MK II (12), MK 3 (15), Killer Instinct (10).
- **Wanted:** Tekken (all), UMK3's extra fighters, MK4, SF III 3rd Strike, KOF '98, and the rest of the Alpha 3 roster.
- **Only add moves from a source you actually read.** Never guess inputs. Note the source in a comment. Add at least one test per new game to `test/test_moves.cpp`.
- Sources used so far:
  - textfiles.com mirrors (mirror.cyberbits.eu)
  - bearstrong.net (MK3)
  - criticalhits.com.br (SFA3)
  - geekyinc and sirlin (SF II)
- tekken.fandom, srk.shib.live and tekkenzaibatsu all blocked fetches from the cloud.

## Gotchas

- `showMessage(msg, 0)` still times out immediately. Set `screen` yourself after a blocking step.
- Function definitions after `render()` need forward declarations. `drawWifi` and `drawOnline` already have them.
- The PlatformIO registry isn't reachable from cloud sessions. Build where the pinned libraries are already installed.
- Commit messages end with the attribution lines the session asks for.
