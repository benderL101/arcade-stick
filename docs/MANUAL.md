# Arcade Stick Firmware v2.4

Firmware for the DIY arcade stick: Freenove ESP32-S3 2.8" display (FNK0104) + MCP23017 expander.

- Wired **USB gamepad** or **Bluetooth gamepad** (pick in the menu)
- Live button display on the screen
- Hold **START for 5 seconds** to power off; hold START or HOME to turn back on
- Retro sound effects on the speaker that came with the board
- Menu (**hold HOME for 1 second**): layout profiles, button remap, turbo, stick mode, connection, brightness, sound, button clicks, screen flip, input test
- Settings saved on the board

This build compiles cleanly, but it hasn't been run on real hardware yet. Use the **Input test** screen on first power-up.

---

## 1. Wiring

### Freenove board → MCP23017 (4 wires)

Use the 1.25 mm-to-Dupont cable that came with the Freenove board. Plug it into the socket marked **I2C / IIC**, not the serial or expansion socket, and match the pin labels printed next to it.

| Freenove I2C port | MCP23017 module |
|---|---|
| 3V3 | VCC |
| GND | GND |
| SDA (GPIO16) | SDA |
| SCL (GPIO15) | SCL |

On the MCP23017 module:
- Leave A0, A1 and A2 **low** (address 0x20). That's the default on most modules.
- If the module has a **RESET** pin, connect it to **VCC**.
- **Don't use GPA7 or GPB7.** On current MCP23017 chips they only work as outputs.

### Buttons and stick → MCP23017

Every switch has two terminals. One goes to its MCP pin; the other goes to the **shared ground** (daisy-chain wire → MCP GND).

Every stick runs the **same firmware**. Wire new sticks to the **standard wiring** below (the blue stick's layout). The original red stick is wired differently: **Menu → Wiring → Old red stick** handles it, so it never needs rewiring.

**Standard wiring:**

| MCP pin | Connect to | Notes |
|---|---|---|
| GPA0 | Stick **Left** | |
| GPA1 | Stick **Right** | |
| GPA2 | Stick **Down** | |
| GPA3 | Stick **Up** | |
| GPA4 | **Home** (24 mm) | tap = Home, hold 1 s = menu |
| GPA5 | **Start** (24 mm) | |
| GPA6 | **Select** (24 mm) | |
| GPB0 | **B3**: top row, right | |
| GPB1 | **B2**: top row, middle | |
| GPB2 | **B1**: top row, left | |
| GPB3 | **B6**: bottom row, right | |
| GPB4 | **B5**: bottom row, middle | also "back" in menus |
| GPB5 | **B4**: bottom row, left | also "select" in menus |
| GPB6 | **EX**: optional 7th button | leave empty if unused |
| GND | Ground daisy-chain + stick harness black wire | |

The directions are where the stick is **pushed**, as the player sees it. Check them in **Menu → Input test**.

<details><summary>Old red stick wiring (for reference)</summary>

| MCP pin | Connect to |
|---|---|
| GPA0 / GPA1 / GPA2 / GPA3 | Up / Down / Left / Right |
| GPA4 / GPA5 / GPA6 | Home / Start / Select |
| GPB0 / GPB1 / GPB3 | B3 / B1 / B2 |
| GPB4 / GPB5 / GPB6 | B5 / B6 / B4 |
| GPB2 | EX |

</details>

### Setting up a new stick

1. Wire it to the standard wiring and flash `ArcadeStick_v2.4_usb.bin` (see Flashing below).
2. **Menu → Colour**: pick its colour (Blue, Red, Green, Yellow, Orange, Purple, Pink or White). The colour sets the screen accent, the name ("**Arcade Stick Green**") in Bluetooth pairing and over USB, and its Wi-Fi name. The stick restarts when you leave the menu so the new name takes effect.
3. **Menu → Player**: set P1, P2, P3 or P4.
4. Pair it with the handheld.

**Joystick tip:** a stick's switches sit opposite the direction you push, so its harness colours don't always match. After wiring, open **Menu → Input test** and push each direction. If one shows the wrong name, swap those two wires.

---

## 2. Flashing

### Which file?

| File | Use it for |
|---|---|
| `ArcadeStick_v2.4_usb.bin` | Flashing over the USB cable (address 0x0) |
| `ArcadeStick_v2.4_wifi.bin` | **Menu → Wi-Fi update** (no cable) |

The same files work on every stick. (Releases also carry `_red_wifi.bin` and `_blue_wifi.bin` copies of the Wi-Fi file, only so sticks still on v2.3 can find the update.)

### Check for update (v2.3 and later): the easy way

The sticks look for new firmware on GitHub (github.com/benderL101/arcade-stick) over your home Wi-Fi.

**One-off setup:** save your home Wi-Fi on each stick.
1. **Menu → Wi-Fi update**, press **B4**.
2. On your phone, join **ArcadeStick-** plus the stick's colour, e.g. **ArcadeStick-Blue**. The password is **arcadestick**.
3. Open **http://192.168.4.1**. Under **Home Wi-Fi**, pick your network, type its password and press **Save home Wi-Fi**.
4. Press **HOME** on the stick (it restarts).

The Wi-Fi password is stored on the stick only. It isn't in the settings file and isn't copied to the other stick.

**Updating from then on:**
1. **Menu → Check for update**, press **B4**. Bluetooth turns off and the stick checks GitHub.
2. It shows **Up to date**, or **v2.x available**. Press **B4** to install it, or **HOME** to cancel.
3. It downloads the file, checks it, installs it and restarts. Your settings are kept.

If anything goes wrong (Wi-Fi, internet, a bad download), the old firmware stays in place and the stick just restarts into it.

### Updating over Wi-Fi from a phone (v2.0 and later)

1. **Menu → Wi-Fi update**, press **B4**. Bluetooth switches off and the stick makes its own Wi-Fi network.
2. On a phone or computer, join **ArcadeStick-** plus the stick's colour, e.g. **ArcadeStick-Blue**. The password is **arcadestick**.
3. Open **http://192.168.4.1**, choose the stick's `_wifi.bin` file and press **Install**. The stick restarts with the new firmware.
4. Press **HOME** to leave without updating (the stick restarts).

The same page has **Download settings** and **Load settings file**: download from one stick, then load the file on the other to copy profiles, remaps, turbo, sound and screen settings. Each stick keeps its own player number, colour and wiring.

### Updating over USB without opening the case (v1.6 and later)

Once v1.6 is on the stick, you never need the BOOT button again:

1. Plug the stick into the computer with a **data** USB-C cable.
2. Open **Menu → USB update** and press **B4**. The screen goes dark: the stick is now in flash mode.
   - Backup method: turn the stick on while holding **START + SELECT**.
3. Flash as in Option A below, from step 3 (Connect, address 0x0, choose the .bin, Program).
4. The stick restarts into the new firmware when flashing finishes. If the screen stays dark, use the **Reset** button in the flashing page's Console section.


### Option A: no software install (easiest)

1. In **Chrome or Edge** on a computer, open **https://espressif.github.io/esptool-js/**.
2. Plug in the Freenove board. Hold **BOOT**, tap **RESET**, then release BOOT.
3. Click **Connect** and pick the board's port.
4. Set Flash Address **0x0**, choose `ArcadeStick_v2.4_usb.bin`, then click **Program**.
5. When it finishes, tap **RESET**.

### Option B: Arduino IDE (to edit the code)

1. Install the **esp32 by Espressif** boards package, version 3.x.
2. In Library Manager, install **LovyanGFX**, **ESP32-BLE-Gamepad** (lemmingDev) and **NimBLE-Arduino** (h2zero).
3. Open `ArcadeStick/ArcadeStick.ino`. The code is in `firmware.cpp` and `moves.h` in the same folder.
4. Set these in the Tools menu:

| Setting | Value |
|---|---|
| Board | ESP32S3 Dev Module |
| USB Mode | **USB-OTG (TinyUSB)** |
| USB CDC On Boot | **Disabled** |
| Upload Mode | USB-OTG CDC (TinyUSB) |
| Flash Size | 16MB (128Mb) |
| PSRAM | OPI PSRAM |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |

If USB Mode or USB CDC On Boot are wrong, the build stops with a message telling you which one to fix.

**If an upload won't start:** hold BOOT, tap RESET, release BOOT, then upload again.

### Option C: PlatformIO

Open the `PlatformIO` folder and run `pio run -t upload`.

---

## 3. Using it

### Status screen

There are three pages. Hold **SELECT** (half a second) and push the stick **up/down** to switch between them:

- **Buttons:** the stick and every button, lit as you press them, labelled with what each one sends.
- **Inputs:** a scrolling history of your recent inputs, newest at the top, with the gap between them in frames (1/60 s). Handy for practising combos.
- **Moves:** the special moves for a fighter (see below).

The header shows your player number (**P1**/**P2**). The footer shows the profile, the page dots and the battery (or **Charging** / **Charged**).

### HOME and SELECT

| Press | Does |
|---|---|
| Tap **HOME** | Home / Steam button |
| Hold **HOME** for 1 s | Opens this stick's menu |
| Tap **SELECT** | Normal Select in games (coin, map, etc.) |
| Hold **SELECT** alone for half a second | Turns on the shortcuts below and shows them on screen until you let go |

### SELECT shortcuts

Hold SELECT on its own for half a second, then press:

| Button | Does |
|---|---|
| **B1** | RetroArch: save state |
| **B2** | RetroArch: load state |
| **B3** | RetroArch: menu |
| **B4** | RetroArch: rewind (hold) |
| **B5** | RetroArch: fast-forward (hold) |
| **B6** | RetroArch: pause |
| Stick **left/right** | Previous / next profile |
| Stick **up/down** | Previous / next screen page |
| **START** | Next fighter (Moves page) |

SELECT keeps working normally in games. The shortcuts only switch on when SELECT is held by itself for half a second, so quick combos like coin + start, or Start + Select soft resets, still go to the game. While the shortcuts are on, the stick's directions and buttons go to the shortcuts instead of the game.

**One-time RetroArch setup.** In RetroArch go to **Settings → Input → Hotkeys**, choose the **SteamOS** profile on the stick, and bind:

| RetroArch hotkey | Press this on the stick |
|---|---|
| Hotkey Enable | SELECT |
| Save State | B3 (RB) |
| Load State | B6 (LB) |
| Menu Toggle | B1 (X) |
| Rewind | B4 (A) |
| Fast-Forward (Hold) | B5 (B) |
| Pause Toggle | B2 (Y) |

Because SELECT is RetroArch's Hotkey Enable, pressing SELECT and one of those buttons together quickly does the same thing in RetroArch, as on any controller set up this way. If you use EmuDeck or RetroDeck, check their hotkey settings first; these replace them in RetroArch.

### Moves page (fighting games)

Choose the game and fighter in the menu: **Menu → Game** and **Menu → Fighter** (stick left/right to change). The status screen switches to the Moves page when you leave the menu. Holding SELECT and pressing START also steps to the next fighter.

The page lists the fighter's special moves with the inputs drawn as arrows:

- Arrows are for a fighter **facing right**. Two of the same arrow (e.g. → →) means tap twice.
- **hold** under an arrow is a charge move: hold that way about a second, then the next arrow.
- The circle is a full 360° rotation; **rapid** means press the button quickly several times; **hold, let go** means hold the button down, then release it.
- The coloured chip is the button: orange = punch, cyan = kick, grey = block / run. **P** / **K** = any punch / kick, **PPP** = all three.

**Move check** (Menu → Move check) watches what you do and lights up the move you pulled off, with a counter:
- **On + sound**: light-up and a jingle
- **On, no sound**: light-up only
- **Off**: just the list

It works facing either way. Moves marked "hold, let go" and moves with no button are shown but not checked.

**Games included**

| Game | Fighters | Buttons the list uses (top row / bottom row) |
|---|---|---|
| Street Fighter II (Turbo / Super) | 16 | LP MP HP / LK MK HK |
| Street Fighter Alpha 3 | 14 (Adon to Fei Long) | LP MP HP / LK MK HK |
| Mortal Kombat | 7 | HP BL HK / LP RUN LK |
| Mortal Kombat II | 12 | HP BL HK / LP RUN LK |
| Mortal Kombat 3 | 15 | HP BL HK / LP RUN LK |
| Killer Instinct | 10 | QP MP FP / QK MK FK |

For the button names to match the game, set up RetroArch's controls for that core so the stick's top and bottom rows are laid out as in the table (Quick Menu → Controls → Port 1 Controls, then save a core remap). Mortal Kombat 1 and II don't have RUN.

The inputs come from published move lists (arcade versions). The Alpha 3 list covers the characters in the source I could verify; more fighters and games (Tekken, Ultimate MK3, MK4, Street Fighter III, King of Fighters) can be added to `moves.h` as move lists are checked.

### Screen sleep, attract mode and auto power off

- After **30 seconds** without input, the screen dims to a retro attract screen. Any press wakes it.
- After the **Screen sleep** time (default 3 min), the screen turns off completely. This saves the most battery.
- **Auto power off** (default 20 min): when the stick is idle and not connected, it turns itself off. It also turns off after an hour idle even if connected. Turn it on again by holding START or HOME.

### Battery

- The battery percentage uses a proper LiPo curve, so it's more accurate than before.
- **Charging** / **Charged** shows when it's plugged in.
- Over Bluetooth, the stick reports its battery level to the host, so SteamOS can show it next to the controller.

Shows the connection, the stick direction, all buttons lighting up as you press them (labelled with what each one sends), turbo rings, the profile and the battery level.

### Power on / off (for battery use)

- **Turn off:** hold **START** for 5 seconds. A countdown appears after 1.5 s. Let go when it says "Release START", and the screen, light and Bluetooth switch off.
- **Turn on:** hold **START** or **HOME** for about half a second.
- While off, the board naps and checks those two buttons four times a second. It still draws a small current, so for long-term storage unplug the battery or fit a switch in its lead.
- The battery still charges over USB-C while the stick is off.
- Holding START in the menu doesn't count, and the press that turns it on won't immediately turn it off again.

### Sounds

Plug the speaker into the board's speaker socket. All the sounds are original chiptune-style effects:

| When | Sound |
|---|---|
| Power on | Start-up chime |
| Power off | Falling tune, after "Release START" |
| Holding START to power off | A beep for each second of the countdown |
| A computer or handheld connects / disconnects | Rising / falling two-note chime |
| Menu | Blips for moving, changing values, opening and saving |
| Move recognised (Moves page) | Short jingle |
| Battery drops to 15 % and 5 % | Three warning beeps |
| Expander stops responding | Low double buzz |

Set the volume, or turn sound off, in **Menu → Sound**. With sound Off, the speaker amplifier is switched off too.

### Status LED
| Colour | Meaning |
|---|---|
| Green | Connected over USB |
| Blue | Connected over Bluetooth |
| Blue, blinking | Bluetooth waiting to pair |
| Amber | USB mode, no host yet |
| Purple | Menu open |
| Red, blinking | MCP23017 not found: check its 4 wires |

### Menu (hold HOME for 1 s)
Stick up/down moves, stick left/right changes a value, **B4** selects, **HOME** or **B5** saves and exits, **Start** saves and exits.

| Item | What it does |
|---|---|
| Profile | **SteamOS** (X Y RB / A B LB, for Steam and RetroArch menus), **Fighting** (LP MP HP / LK MK HK), **SNES** (Y X L / B A R), **Mega Drive** (X Y Z / A B C), **Custom** |
| Remap buttons | Press a button, then stick up/down to change what it sends. Edits the Custom profile. |
| Turbo | Press buttons to toggle turbo; stick left/right sets 10/15/20/30 Hz |
| Stick | D-pad, Left analog or Right analog |
| Game | Which game the Moves page shows |
| Fighter | Which fighter the Moves page shows |
| Move check | Off, On (no sound) or On + sound: lights up moves you pull off |
| Connection | USB or Bluetooth. The stick restarts into the new mode. |
| Forget BT pairings | Clears saved Bluetooth devices (Bluetooth mode only) |
| Brightness | 10–100 % |
| Screen sleep | Screen off after Never / 1 / 3 / 5 / 10 min idle |
| Auto power off | Off / 10 / 20 / 30 min idle when not connected |
| Sound | Off, Low, Medium or High. Changing it plays a sample at the new level. |
| Button clicks | A short click on every button press during play. Off by default. |
| Colour | The stick's colour: Blue, Red, Green, Yellow, Orange, Purple, Pink or White. Sets the screen accent and the name in Bluetooth, USB and Wi-Fi ("Arcade Stick Green"). The stick restarts when you leave the menu if it changed. |
| Player | P1 to P4, shown on the screen. The host decides the real player order: connect P1 first. |
| Left + Right | What happens if left and right are pressed together: Cancel out or Last wins. A lever can't do this, so it only matters on a leverless build. |
| Wiring | **Standard** for every stick wired to the table in section 1; **Old red stick** for the original red stick. Takes effect straight away. |
| Flip screen | Rotates 180° if the screen is mounted the other way round |
| Input test | Shows every input and its MCP pin. **Hold HOME for 2 s** to leave. |
| Check for update | Checks GitHub for newer firmware over your home Wi-Fi and installs it if you say so. |
| USB update | Restarts into flash mode for a USB firmware update, no case opening needed. |
| Wi-Fi update | Update firmware, copy settings, or save your home Wi-Fi (for Check for update) from a phone. |

The profiles use standard Android gamepad buttons, so RetroArch picks them up without any manual binding. The Mega Drive profile matches Genesis Plus GX's default 6-button layout.

### Bluetooth pairing
Set Connection to Bluetooth, then on the handheld open Bluetooth settings and pair with **"Arcade Stick"** plus its colour, e.g. **"Arcade Stick Blue"**.

---

## 4. Tweaks at the top of `firmware.cpp`

- `PANEL_INVERT` / `PANEL_RGB_ORDER`: if the screen colours look wrong (negative, or red and blue swapped).
- `I2C_HZ`: 400 kHz default. 1 MHz gives slightly lower latency if your wires are short.
- `PROFILE_MAP`: change or add layouts.
- `MCP_BIT`: rewire inputs to different expander pins.
