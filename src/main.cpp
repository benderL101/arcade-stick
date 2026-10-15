/*
 * =====================================================================
 *  DIY ARCADE STICK FIRMWARE  v2.2
 *  Freenove ESP32-S3 2.8" Display (FNK0104, non-touch)
 *  + MCP23017 I/O expander (all buttons and stick)
 *
 *  - Wired USB gamepad OR Bluetooth (BLE) gamepad
 *  - Live button display on the 2.8" screen
 *  - Menu (hold HOME 1 second): layout profiles, remap, turbo,
 *    stick mode, connection, brightness, screen flip, input test
 *  - Settings saved on the board
 *  - Retro sound effects on the board's speaker (ES8311 codec)
 *
 *  Arduino IDE settings (Tools menu):
 *    Board ............ ESP32S3 Dev Module
 *    USB Mode ......... USB-OTG (TinyUSB)        <- required
 *    USB CDC On Boot .. Disabled                 <- required
 *    Upload Mode ...... USB-OTG CDC (TinyUSB)
 *    Flash Size ....... 16MB
 *    PSRAM ............ OPI PSRAM
 *    Partition Scheme . 16M Flash (3MB APP/9.9MB FATFS) or any 3MB+ app
 *  Libraries (Library Manager):
 *    LovyanGFX, ESP32-BLE-Gamepad (lemmingDev), NimBLE-Arduino (h2zero)
 * =====================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>
#include <atomic>
#include "USB.h"
#include "USBCDC.h"
#include "USBHIDGamepad.h"
#include "esp32-hal-tinyusb.h"   // usb_persist_restart(): reboot into flash mode
#include <BleGamepad.h>
#include <NimBLEDevice.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "esp_sleep.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include "moves.h"
#include "ESP_I2S.h"

#if ARDUINO_USB_MODE
#error "Set Tools > USB Mode to 'USB-OTG (TinyUSB)'"
#endif
#if ARDUINO_USB_CDC_ON_BOOT
#error "Set Tools > USB CDC On Boot to 'Disabled'"
#endif

// ======================= HARDWARE CONFIG =============================
// Freenove FNK0104 (2.8" ILI9341) pins, from Freenove's TFT_eSPI setup
#define PIN_TFT_SCLK 12
#define PIN_TFT_MOSI 11
#define PIN_TFT_MISO 13
#define PIN_TFT_CS   10
#define PIN_TFT_DC   46
#define PIN_TFT_BL   45
#define STICK_LED_PIN  42      // on-board WS2812
#define PIN_BAT_ADC  9       // battery voltage / 2
#define PIN_I2C_SDA  16      // 4-pin I2C port
#define PIN_I2C_SCL  15
#define I2C_HZ       400000  // try 1000000 for even lower latency

// If colours look wrong, flip these:
#define PANEL_INVERT    true   // IPS panel needs inversion (as Freenove's setup)
#define PANEL_RGB_ORDER false  // set true if red and blue are swapped

#define MCP_ADDR 0x20          // A0/A1/A2 all low

// Speaker: ES8311 codec (I2C 0x18, same bus as the MCP23017) + amplifier.
// Pins from Freenove's FNK0104 "Music" example.
#define PIN_I2S_MCLK 4
#define PIN_I2S_BCLK 5
#define PIN_I2S_WS   7
#define PIN_I2S_DOUT 8
#define PIN_AMP_EN   1       // LOW = amplifier on
#define ES8311_ADDR  0x18

#ifdef STICK_BLUE
#define DEVICE_NAME "Arcade Stick Blue"   // own name, so the two sticks are easy to tell apart
#else
#define DEVICE_NAME "Arcade Stick"
#endif
#ifdef STICK_BLUE
#define STICK_COLOUR_NAME "Blue"
#define STICK_ACCENT      0x2B7F   // blue
#define DEFAULT_PLAYER    1        // P2
#else
#define STICK_COLOUR_NAME "Red"
#define STICK_ACCENT      0xE145   // red
#define DEFAULT_PLAYER    0        // P1
#endif
#define FW_VERSION  "2.3"
#define UPDATE_REPO "benderL101/arcade-stick"   // GitHub repo the stick checks for new releases

// Power off: hold START this long. While off the board naps and checks
// START/HOME every SLEEP_POLL_MS; hold either for ~half a second to turn on.
#define POWER_OFF_HOLD_MS 5000
#define LAYER_HOLD_MS     500    // hold SELECT alone this long to switch on the shortcuts
#define SLEEP_POLL_MS     250

// ======================= INPUT DEFINITIONS ===========================
// Physical inputs. Buttons B1-B3 = top row (left->right),
// B4-B6 = bottom row (left->right), EX = optional 7th button.
enum Phys : uint8_t {
  P_UP, P_DOWN, P_LEFT, P_RIGHT, P_START, P_SELECT, P_HOME,
  P_B1, P_B2, P_B3, P_B4, P_B5, P_B6, P_EX, PHYS_COUNT
};
// MCP23017 pin for each input. GPA0-6 = bits 0-6, GPB0-6 = bits 8-14.
// GPA7 and GPB7 are NOT used: on current MCP23017 silicon they are
// output-only (Microchip datasheet rev. D+).
#ifdef STICK_BLUE
// Blue stick's wiring (build with -DSTICK_BLUE):
const uint8_t MCP_BIT[PHYS_COUNT] = {
  3, 2, 0, 1,                 // Up = GPA3, Down = GPA2, Left = GPA0, Right = GPA1
  5, 6, 4,                    // Start = GPA5, Select = GPA6, Home = GPA4
  10, 9, 8, 13, 12, 11,       // B1 = GPB2, B2 = GPB1, B3 = GPB0, B4 = GPB5, B5 = GPB4, B6 = GPB3
  14                          // EX = GPB6 (the spare pin)
};
const char *PHYS_PIN[PHYS_COUNT] = {
  "GPA3", "GPA2", "GPA0", "GPA1", "GPA5", "GPA6", "GPA4",
  "GPB2", "GPB1", "GPB0", "GPB5", "GPB4", "GPB3", "GPB6"
};
#else
// Red stick's wiring (default build):
const uint8_t MCP_BIT[PHYS_COUNT] = {
  0, 1, 2, 3,                 // GPA0..GPA3 : Up Down Left Right
  5, 6, 4,                    // Start = GPA5, Select = GPA6, Home = GPA4
  9, 11, 8, 14, 12, 13,       // B1 = GPB1, B2 = GPB3, B3 = GPB0, B4 = GPB6, B5 = GPB4, B6 = GPB5
  10                          // EX = GPB2 (the spare pin)
};
const char *PHYS_PIN[PHYS_COUNT] = {
  "GPA0", "GPA1", "GPA2", "GPA3", "GPA5", "GPA6", "GPA4",
  "GPB1", "GPB3", "GPB0", "GPB6", "GPB4", "GPB5", "GPB2"
};
#endif
const char *PHYS_NAME[PHYS_COUNT] = {
  "Up", "Down", "Left", "Right", "Start", "Select", "Home",
  "B1 top-L", "B2 top-M", "B3 top-R", "B4 bot-L", "B5 bot-M", "B6 bot-R", "EX extra"
};
#define ACT_COUNT 7            // B1..B6 + EX
#define ACT_FIRST P_B1

// Gamepad outputs. Value-1 = HID button index; matches Android's
// standard layout (A B C X Y Z L1 R1 L2 R2 Select Start Home L3 R3).
enum Out : uint8_t {
  O_NONE, O_A, O_B, O_C, O_X, O_Y, O_Z, O_L1, O_R1, O_L2, O_R2,
  O_SELECT, O_START, O_HOME, O_L3, O_R3, OUT_COUNT
};
const char *OUT_NAME[OUT_COUNT] = {
  "-", "A", "B", "C", "X", "Y", "Z", "L1", "R1", "L2", "R2",
  "SEL", "STA", "HOM", "L3", "R3"
};

// Layout profiles: outputs for B1 B2 B3 / B4 B5 B6 / EX.
// Names follow the standard (Xbox-style) Android/RetroArch gamepad, so
// RetroArch cores pick them up without manual binding.
#define PROFILE_COUNT 5
#define PROFILE_CUSTOM 3       // index kept at 3 so saved settings still point to Custom
#define PROFILE_STEAM  4       // added in v1.5
const char *PROFILE_NAME[PROFILE_COUNT] = {"Fighting", "SNES", "Mega Drive", "Custom", "SteamOS"};
const uint8_t PROFILE_MAP[PROFILE_COUNT][ACT_COUNT] = {
  // Street Fighter / modern fightstick: LP MP HP / LK MK HK
  {O_X, O_Y, O_R1,  O_A, O_B, O_R2,  O_L1},
  // SNES: Y X L / B A R
  {O_X, O_Y, O_L1,  O_A, O_B, O_R1,  O_L2},
  // Mega Drive 6-button (RetroArch Genesis Plus GX): X Y Z / A B C
  {O_L1, O_Y, O_R1, O_X, O_A, O_B,   O_L2},
  // (Custom lives in cfg.custom)
  {O_X, O_Y, O_R1,  O_A, O_B, O_R2,  O_L1},
  // SteamOS menus: X Y RB / A B LB. A (confirm) sits nearest the stick with
  // B (back) beside it; RB / LB (next / previous tab) share the right column.
  // Select = View, Start = Menu, Home = Steam button.
  {O_X, O_Y, O_R1,  O_A, O_B, O_L1,  O_R2},
};
// Order the Profile setting steps through in the menu
const uint8_t PROFILE_ORDER[PROFILE_COUNT] = {PROFILE_STEAM, 0, 1, 2, PROFILE_CUSTOM};

enum StickMode : uint8_t { SM_DPAD, SM_LEFT, SM_RIGHT, SM_COUNT };
const char *STICK_NAME[SM_COUNT] = {"D-pad", "Left stick", "Right stick"};
enum Conn : uint8_t { C_USB, C_BLE };
const uint8_t TURBO_HZ[] = {10, 15, 20, 30};
#define TURBO_HZ_COUNT 4
const uint8_t SLEEP_MIN[] = {0, 1, 3, 5, 10};          // screen off after (0 = never)
const char *SLEEP_NAME[] = {"Never", "1 min", "3 min", "5 min", "10 min"};
#define SLEEP_COUNT 5
const uint8_t AUTOOFF_MIN[] = {0, 10, 20, 30};         // power off when idle and not connected
const char *AUTOOFF_NAME[] = {"Off", "10 min", "20 min", "30 min"};
#define AUTOOFF_COUNT 4
const char *SOCD_NAME[] = {"Cancel out", "Last wins"};
#define SOCD_COUNT 2
#define THEME_COUNT 6
const char *THEME_NAME[THEME_COUNT] = {"Stick colour", "Amber", "Red", "Blue", "Green", "Purple"};
const uint16_t THEME_COL[THEME_COUNT] = {STICK_ACCENT, 0xFD20, 0xE145, 0x2B7F, 0x3E4B, 0xAA9C};
enum Page : uint8_t { PG_BUTTONS, PG_INPUTS, PG_MOVES, PAGE_COUNT };
const char *PAGE_NAME[PAGE_COUNT] = {"Buttons", "Inputs", "Moves"};

// ======================= SETTINGS ===================================
struct Settings {
  uint8_t version = 2;
  uint8_t profile = 0;
  uint8_t custom[ACT_COUNT] = {O_X, O_Y, O_R1, O_A, O_B, O_R2, O_L1};
  uint8_t turboMask = 0;     // bit per action button
  uint8_t turboHzIdx = 1;
  uint8_t stickMode = SM_DPAD;
  uint8_t conn = C_USB;
  uint8_t brightness = 80;   // percent
  uint8_t flip = 0;
  uint8_t sound = 2;         // 0 off, 1 low, 2 medium, 3 high   (added v1.3)
  uint8_t clicks = 0;        // click on every button press       (added v1.3)
  uint8_t screenSleep = 2;   // index into SLEEP_MIN               (added v2.0)
  uint8_t autoOff = 2;       // index into AUTOOFF_MIN
  uint8_t socd = 0;          // Left + Right together: 0 cancel out, 1 last wins
  uint8_t player = DEFAULT_PLAYER;
  uint8_t theme = 0;         // 0 = stick colour
  uint8_t page = 0;          // status screen page
  uint8_t game = 0;          // move list game
  uint8_t fighter = 0;       // move list character
  uint8_t moveCheck = 2;     // 0 off, 1 show on screen, 2 screen + sound   (added v2.2)
} cfg;
Preferences prefs;
uint8_t bootConn;            // connection mode actually started at boot

const uint8_t *activeMap() {
  return cfg.profile == PROFILE_CUSTOM ? cfg.custom : PROFILE_MAP[cfg.profile];
}
void loadSettings() {
  prefs.begin("stick", true);
  Settings s;
  // Older firmware saved a shorter struct: read what's there and keep
  // defaults for fields added since (so an update keeps your settings).
  size_t len = prefs.getBytesLength("cfg");
  if (len >= 10 && len <= sizeof(Settings)) {
    prefs.getBytes("cfg", &s, len);
    if (s.version == cfg.version) cfg = s;
  }
  prefs.end();
  if (cfg.profile >= PROFILE_COUNT) cfg.profile = 0;
  if (cfg.stickMode >= SM_COUNT) cfg.stickMode = SM_DPAD;
  if (cfg.turboHzIdx >= TURBO_HZ_COUNT) cfg.turboHzIdx = 1;
  cfg.brightness = constrain(cfg.brightness, 10, 100);
  if (cfg.sound > 3) cfg.sound = 2;
  cfg.clicks = cfg.clicks ? 1 : 0;
  if (cfg.screenSleep >= SLEEP_COUNT) cfg.screenSleep = 2;
  if (cfg.autoOff >= AUTOOFF_COUNT) cfg.autoOff = 2;
  if (cfg.socd >= SOCD_COUNT) cfg.socd = 0;
  if (cfg.player > 3) cfg.player = DEFAULT_PLAYER;
  if (cfg.theme >= THEME_COUNT) cfg.theme = 0;
  if (cfg.page >= PAGE_COUNT) cfg.page = 0;
  if (cfg.game >= GAME_COUNT) cfg.game = 0;
  if (cfg.fighter >= GAMES[cfg.game].n) cfg.fighter = 0;
  if (cfg.moveCheck > 2) cfg.moveCheck = 2;
}
void saveSettings() {
  prefs.begin("stick", false);
  prefs.putBytes("cfg", &cfg, sizeof(Settings));
  prefs.end();
}

// ======================= DISPLAY ====================================
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9341 _panel;
  lgfx::Bus_SPI _bus;
  lgfx::Light_PWM _light;
public:
  LGFX() {
    {
      auto c = _bus.config();
      c.spi_host = SPI2_HOST;   // pins 10-13 are SPI2's native IO_MUX pins
      c.spi_mode = 0;
      c.freq_write = 40000000;
      c.freq_read = 16000000;
      c.spi_3wire = false;
      c.use_lock = true;
      c.dma_channel = SPI_DMA_CH_AUTO;
      c.pin_sclk = PIN_TFT_SCLK;
      c.pin_mosi = PIN_TFT_MOSI;
      c.pin_miso = PIN_TFT_MISO;
      c.pin_dc = PIN_TFT_DC;
      _bus.config(c);
      _panel.setBus(&_bus);
    }
    {
      auto c = _panel.config();
      c.pin_cs = PIN_TFT_CS;
      c.pin_rst = -1;
      c.pin_busy = -1;
      c.panel_width = 240;
      c.panel_height = 320;
      c.readable = true;
      c.invert = PANEL_INVERT;
      c.rgb_order = PANEL_RGB_ORDER;
      c.bus_shared = false;
      _panel.config(c);
    }
    {
      auto c = _light.config();
      c.pin_bl = PIN_TFT_BL;
      c.invert = false;
      c.freq = 12000;
      c.pwm_channel = 7;
      _light.config(c);
      _panel.setLight(&_light);
    }
    setPanel(&_panel);
  }
};
LGFX tft;
LGFX_Sprite canvas(&tft);
bool haveCanvas = false;

// ======================= GAMEPAD OUTPUT =============================
USBHIDGamepad usbPad;
USBCDC debugSerial;          // serial monitor + lets the IDE auto-reset for upload
BleGamepad blePad(DEVICE_NAME, "DIY", 100);
volatile bool usbMounted = false;
volatile bool radioOff = false;      // Bluetooth shut down (Wi-Fi update mode)

struct Report {
  uint32_t buttons;
  uint8_t hat;
  int8_t lx, ly, rx, ry;
  bool operator!=(const Report &o) const {
    return buttons != o.buttons || hat != o.hat || lx != o.lx || ly != o.ly || rx != o.rx || ry != o.ry;
  }
};
Report lastSent = {0, 0, 0, 0, 0, 0};
bool forceSend = true;

// ======================= SHARED STATE ===============================
volatile uint16_t physState = 0;     // debounced, bit = Phys index, 1 = pressed
std::atomic<uint32_t> pressEdges{0}; // latched presses for the UI task (cross-core safe)
volatile bool menuOpen = false;
volatile bool menuRequest = false;
volatile bool mcpOk = false;
volatile bool uiDirty = true;
volatile uint32_t uiInputTime = 0;
volatile uint32_t startDownAt = 0;    // when the current START hold began (0 = not held)
volatile bool powerOffRequest = false;
volatile bool shuttingDown = false;   // input loop stops so the UI task owns the I2C bus
bool startArmed = false;              // START must be released once after power-on

// SELECT shortcut layer: hold SELECT alone for half a second, then press a button
enum : uint8_t { LE_PROF_PREV = 1, LE_PROF_NEXT = 2, LE_PAGE_PREV = 4, LE_PAGE_NEXT = 8, LE_FIGHTER = 16 };
std::atomic<uint8_t> layerEvents{0};
volatile bool selLayer = false;       // shortcuts active (SELECT held alone long enough)
volatile int8_t lastH = 0, lastV = 0; // most recent horizontal / vertical press ("last wins")

// Input history for the Inputs page (written by the input loop)
struct HistEntry { uint8_t dir; uint16_t btns; uint32_t t; };
#define HIST_LEN 12
HistEntry inputHist[HIST_LEN];
std::atomic<uint8_t> histHead{0};

// Move recognition for the Moves page
MotionReader motion;
volatile int8_t moveHit = -1;
volatile bool moveHitMirror = false;
volatile uint32_t moveHitAt = 0;
std::atomic<uint32_t> moveHitSeq{0};

#define ACTION_MASK (((1u << ACT_COUNT) - 1) << ACT_FIRST)
#define TOP_ROW     ((1u << P_B1) | (1u << P_B2) | (1u << P_B3))
#define BOTTOM_ROW  ((1u << P_B4) | (1u << P_B5) | (1u << P_B6))

// Stick direction as a numpad digit (5 = centre)
static inline uint8_t numpadDir(uint16_t s) {
  int x = (int)((s >> P_RIGHT) & 1) - (int)((s >> P_LEFT) & 1);
  int y = (int)((s >> P_UP) & 1) - (int)((s >> P_DOWN) & 1);
  return 5 + x + 3 * y;
}

static inline bool held(uint16_t s, uint8_t p) { return s & (1u << p); }
uint16_t takeEdges() { return (uint16_t)pressEdges.exchange(0); }

void usbEvent(void *, esp_event_base_t base, int32_t id, void *) {
  if (base != ARDUINO_USB_EVENTS) return;
  if (id == ARDUINO_USB_STARTED_EVENT || id == ARDUINO_USB_RESUME_EVENT) usbMounted = true;
  if (id == ARDUINO_USB_STOPPED_EVENT) usbMounted = false;
  forceSend = true;
  uiDirty = true;
}

bool hostConnected() {
  if (bootConn == C_BLE && radioOff) return false;
  return bootConn == C_USB ? usbMounted : blePad.isConnected();
}

// ======================= MCP23017 ===================================
bool mcpWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MCP_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}
bool mcpInit() {
  // IOCON.BANK=0 (power-on default): IODIRA=0x00 IODIRB=0x01 GPPUA=0x0C GPPUB=0x0D
  // IPOLA=0x02 IPOLB=0x03 (invert so pressed reads 1)
  return mcpWrite(0x00, 0x7F) && mcpWrite(0x01, 0x7F) &&   // GPx0-6 inputs, GPx7 output
         mcpWrite(0x0C, 0x7F) && mcpWrite(0x0D, 0x7F) &&   // pull-ups on inputs
         mcpWrite(0x02, 0x7F) && mcpWrite(0x03, 0x7F) &&   // invert: pressed = 1
         mcpWrite(0x14, 0x00) && mcpWrite(0x15, 0x00);     // GPA7/GPB7 driven low
}
// returns true on success; raw = GPB:GPA, 1 = pressed
bool mcpRead(uint16_t &raw) {
  Wire.beginTransmission(MCP_ADDR);
  Wire.write(0x12);                        // GPIOA (auto-increments to GPIOB)
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)MCP_ADDR, (uint8_t)2) != 2) return false;
  uint8_t a = Wire.read(), b = Wire.read();
  raw = ((uint16_t)b << 8 | a) & 0x7F7F;
  return true;
}

// ======================= INPUT -> REPORT ============================
#define DEBOUNCE_MS 5
uint32_t lockUntil[PHYS_COUNT] = {0};
uint32_t homeDownAt = 0;
bool homeEligible = false;           // Home press began outside the menu
uint32_t homePulseUntil = 0;

void scanInputs() {
  uint16_t raw;
  if (!mcpRead(raw)) {
    if (mcpOk) { mcpOk = false; uiDirty = true; }
    static uint32_t lastRetry = 0;
    if (millis() - lastRetry > 1000) { lastRetry = millis(); if (mcpInit()) { mcpOk = true; uiDirty = true; } }
    return;
  }
  if (!mcpOk) { mcpOk = true; uiDirty = true; }

  uint32_t now = millis();
  uint16_t state = physState;
  uint16_t newEdges = 0;
  for (uint8_t p = 0; p < PHYS_COUNT; p++) {
    bool rawOn = raw & (1u << MCP_BIT[p]);
    bool cur = held(state, p);
    // "eager" debounce: react instantly, then ignore bounce for 5 ms
    if (rawOn != cur && (int32_t)(now - lockUntil[p]) >= 0) {
      lockUntil[p] = now + DEBOUNCE_MS;
      if (rawOn) { state |= (1u << p); pressEdges.fetch_or(1u << p); newEdges |= (1u << p); }
      else state &= ~(1u << p);
    }
  }
  if (state != physState) { physState = state; uiDirty = true; uiInputTime = now; }

  // SOCD bookkeeping: remember the latest of each opposing pair
  if (newEdges & (1u << P_LEFT))  lastH = -1;
  if (newEdges & (1u << P_RIGHT)) lastH = 1;
  if (newEdges & (1u << P_UP))    lastV = -1;
  if (newEdges & (1u << P_DOWN))  lastV = 1;

  // HOME: tap = Home (Steam) button, hold 1 s = open menu
  bool home = held(state, P_HOME);
  static bool homePrev = false;
  if (home && !homePrev) { homeDownAt = now; homeEligible = !menuOpen; }
  if (home && homeEligible && !menuOpen && now - homeDownAt >= 1000) {
    homeEligible = false;
    menuRequest = true;
  }
  if (!home && homePrev && homeEligible && !menuOpen && now - homeDownAt < 600) {
    homePulseUntil = now + 80;
  }
  homePrev = home;

  // SELECT: always passes through to the game. Held ALONE for LAYER_HOLD_MS,
  // it switches on the shortcuts (quick SELECT combos stay normal game input).
  bool sel = held(state, P_SELECT);
  static bool selPrev = false, selClean = false;
  static uint32_t selDownAt = 0;
  if (sel && !selPrev) { selDownAt = now; selClean = !menuOpen && (state & ~(1u << P_SELECT)) == 0; }
  if (sel && !selLayer) {
    if (newEdges & ~(1u << P_SELECT)) selClean = false;        // used in a combo: not a shortcut
    if (selClean && !menuOpen && now - selDownAt >= LAYER_HOLD_MS) { selLayer = true; uiDirty = true; }
  }
  if (!sel || menuOpen) { if (selLayer) uiDirty = true; selLayer = false; }
  if (selLayer) {
    uint8_t ev = 0;
    if (newEdges & (1u << P_LEFT))  ev |= LE_PROF_PREV;
    if (newEdges & (1u << P_RIGHT)) ev |= LE_PROF_NEXT;
    if (newEdges & (1u << P_UP))    ev |= LE_PAGE_PREV;
    if (newEdges & (1u << P_DOWN))  ev |= LE_PAGE_NEXT;
    if (newEdges & (1u << P_START)) ev |= LE_FIGHTER;
    if (ev) layerEvents.fetch_or(ev);
  }
  selPrev = sel;

  // Input history + move recognition (not while the shortcuts are active)
  if (!menuOpen && !selLayer) {
    static uint8_t lastDir = 5;
    uint8_t d = numpadDir(state);
    uint16_t btnEdges = newEdges & (ACTION_MASK | (1u << P_START) | (1u << P_SELECT));
    if (d != lastDir) { motion.dir(d, now); lastDir = d; }
    if (d != inputHist[(histHead + HIST_LEN - 1) % HIST_LEN].dir || btnEdges) {
      uint8_t h = histHead;
      inputHist[h] = {d, btnEdges, now};
      histHead = (h + 1) % HIST_LEN;
    }
    if (cfg.page == PG_MOVES && cfg.moveCheck) {
      uint8_t held6 = (state >> ACT_FIRST) & 0x3F, new6 = (newEdges >> ACT_FIRST) & 0x3F;
      for (uint8_t b = 0; b < 6; b++) {
        if (!(new6 & (1u << b))) continue;
        const Game &gm = GAMES[cfg.game];
        bool mir = false;
        int idx = motion.press(b, held6, now, gm, gm.fighters[cfg.fighter], mir);
        if (idx >= 0) { moveHit = idx; moveHitMirror = mir; moveHitAt = now; moveHitSeq++; uiDirty = true; }
      }
    }
  }

  // START: hold 5 s = power off. Not counted in the menu, and only after
  // START has been let go once (so the press that woke it doesn't count).
  bool start = held(state, P_START);
  if (!start || menuOpen) { startArmed = !start; startDownAt = 0; }
  else if (startArmed) {
    if (!startDownAt) startDownAt = now ? now : 1;
    if (now - startDownAt >= POWER_OFF_HOLD_MS) {
      startArmed = false; startDownAt = 0; powerOffRequest = true;
    }
  }
}

// RetroArch shortcuts: with SELECT held (RetroArch's "Hotkey Enable"), each
// button sends the output below, whatever the profile. Bind them once in RetroArch.
struct Hotkey { uint8_t phys; uint8_t out; const char *name; };
const Hotkey HOTKEYS[] = {
  {P_B1, O_R1, "Save state"}, {P_B2, O_L1, "Load state"}, {P_B3, O_X, "Menu"},
  {P_B4, O_A, "Rewind"},      {P_B5, O_B, "Fast fwd"},    {P_B6, O_Y, "Pause"},
};
#define HOTKEY_COUNT (sizeof(HOTKEYS) / sizeof(HOTKEYS[0]))

Report buildReport() {
  Report r = {0, 0, 0, 0, 0, 0};
  if (menuOpen) return r;              // controller goes neutral in the menu
  uint16_t s = physState;
  uint32_t now = millis();

  static uint16_t layerHeld = 0;       // buttons used for a shortcut: ignored until let go
  if (selLayer) {
    r.buttons = 1UL << (O_SELECT - 1);                    // SELECT stays held (hotkey enable)
    for (uint8_t i = 0; i < HOTKEY_COUNT; i++)
      if (held(s, HOTKEYS[i].phys)) r.buttons |= 1UL << (HOTKEYS[i].out - 1);
    layerHeld |= s & ~(1u << P_SELECT);
    return r;                                             // stick and buttons stay with the shortcuts
  }
  layerHeld &= s;
  s &= ~layerHeld;

  const uint8_t *amap = activeMap();
  uint32_t period = 500 / TURBO_HZ[cfg.turboHzIdx];
  bool turboPhase = ((now / period) & 1) == 0;
  for (uint8_t i = 0; i < ACT_COUNT; i++) {
    if (!held(s, ACT_FIRST + i)) continue;
    if ((cfg.turboMask & (1 << i)) && !turboPhase) continue;
    uint8_t o = amap[i];
    if (o != O_NONE) r.buttons |= 1UL << (o - 1);
  }
  if (held(s, P_START)) r.buttons |= 1UL << (O_START - 1);
  if (held(s, P_SELECT)) r.buttons |= 1UL << (O_SELECT - 1);
  if ((int32_t)(homePulseUntil - now) > 0) r.buttons |= 1UL << (O_HOME - 1);

  // Opposite directions together (only possible on a leverless/hitbox build):
  // "Cancel out" = Left+Right neutral and Up+Down = Up, "Last wins" = newest press
  bool u = held(s, P_UP), d = held(s, P_DOWN), l = held(s, P_LEFT), rt = held(s, P_RIGHT);
  if (l && rt) { if (cfg.socd == 1) { l = lastH < 0; rt = lastH > 0; } else l = rt = false; }
  if (u && d)  { if (cfg.socd == 1) { u = lastV < 0; d = lastV > 0; } else d = false; }
  int8_t x = rt ? 1 : (l ? -1 : 0);
  int8_t y = d ? 1 : (u ? -1 : 0);

  if (cfg.stickMode == SM_DPAD) {
    static const uint8_t HAT[3][3] = {   // [y+1][x+1]
      {8, 1, 2},   // up-left, up, up-right
      {7, 0, 3},   // left, centre, right
      {6, 5, 4}};  // down-left, down, down-right
    r.hat = HAT[y + 1][x + 1];
  } else if (cfg.stickMode == SM_LEFT) {
    r.lx = x * 127; r.ly = y * 127;
  } else {
    r.rx = x * 127; r.ry = y * 127;
  }
  return r;
}

void sendReport(const Report &r) {
  if (bootConn == C_USB) {
    if (!usbMounted) return;
    // send(x, y, z, rz, rx, ry, hat, buttons): right stick on Z/RZ (Android standard)
    if (usbPad.send(r.lx, r.ly, r.rx, r.ry, 0, 0, r.hat, r.buttons)) {
      lastSent = r; forceSend = false;
    }
  } else {
    if (radioOff || !blePad.isConnected()) return;
    for (uint8_t b = 0; b < 15; b++) {
      bool want = r.buttons & (1UL << b), had = lastSent.buttons & (1UL << b);
      if (want != had || forceSend) { if (want) blePad.press(b + 1); else blePad.release(b + 1); }
    }
    blePad.setHat1(r.hat);
    // setAxes(x, y, z, rX, rY, rZ, slider1, slider2); only X, Y, Z, RZ are enabled
    blePad.setAxes(r.lx * 258, r.ly * 258, r.rx * 258, 0, 0, r.ry * 258, 0, 0);
    blePad.sendReport();
    lastSent = r; forceSend = false;
  }
}

// ======================= RGB LED ====================================
void updateLed() {
  static uint32_t last = 0;
  uint32_t now = millis();
  if (now - last < 50) return;
  last = now;
  uint8_t r = 0, g = 0, b = 0;
  bool blink = (now / 500) & 1;
  if (!mcpOk)              { r = blink ? 20 : 0; }               // red blink: expander missing
  else if (menuOpen)       { r = 12; b = 12; }                   // purple: menu
  else if (hostConnected()) { if (bootConn == C_USB) g = 15; else b = 20; }
  else if (bootConn == C_BLE) { b = blink ? 20 : 0; }            // blue blink: pairing
  else                     { r = 12; g = 6; }                    // amber: waiting for USB host
  rgbLedWrite(STICK_LED_PIN, r, g, b);
}

// ======================= BATTERY ====================================
float batteryVolts = 0;
volatile uint8_t chargeState = 0;    // 0 = on battery, 1 = charging, 2 = charged
int batteryPercent() {
  if (batteryVolts < 2.5f) return -1;          // no battery fitted
  return lipoPercent(batteryVolts);
}
void readBattery() {
  static uint32_t last = 0, lastTrend = 0;
  static float trend[9]; static uint8_t trendN = 0;
  uint32_t now = millis();
  if (now - last < 2000 && last) return;
  last = now;
  float v = analogReadMilliVolts(PIN_BAT_ADC) * 2.0f / 1000.0f;
  batteryVolts = batteryVolts == 0 ? v : batteryVolts * 0.8f + v * 0.2f;
  // charging shows as a rising voltage (sampled every 20 s over ~3 min)
  if (now - lastTrend >= 20000 || !lastTrend) {
    lastTrend = now;
    if (trendN == 9) { memmove(trend, trend + 1, sizeof(float) * 8); trendN = 8; }
    trend[trendN++] = batteryVolts;
  }
  bool rising = trendN >= 6 && trend[trendN - 1] - trend[0] >= 0.025f;
  if (batteryPercent() < 0) chargeState = 0;
  else if (usbMounted || rising || batteryVolts >= 4.19f)          // on a USB host, or charger
    chargeState = (batteryVolts >= 4.16f && !rising) ? 2 : 1;
  else chargeState = 0;
}

// ======================= SOUND ======================================
// A tiny chiptune-style synth: two voices (square / triangle / noise) with
// pitch slides and a simple envelope, mixed at 24 kHz and sent to the
// ES8311 over I2S. It runs as its own low-priority task on core 0, so it
// never touches the button-scanning loop on core 1. All sounds are original.
#define SND_RATE   24000
#define SND_FRAMES 240                       // 10 ms per I2S write

enum Wave : uint8_t { W_SQ12, W_SQ25, W_SQ50, W_TRI, W_NOISE, W_REST };
struct Note { uint16_t f0, f1, ms; uint8_t wave, vol; };   // f1 = slide target (0 = none)
#define N(f, ms, w, v)       {f, 0, ms, w, v}
#define SL(f0, f1, ms, w, v) {f0, f1, ms, w, v}
#define R(ms)                {0, 0, ms, W_REST, 0}

// Note frequencies (Hz)
enum : uint16_t { F_A2 = 110, F_E3 = 165, F_A3 = 220, F_C4 = 262, F_E4 = 330, F_G4 = 392, F_A4 = 440, F_B4 = 494,
  F_C5 = 523, F_CS5 = 554, F_D5 = 587, F_E5 = 659, F_FS5 = 740, F_G5 = 784, F_A5 = 880, F_B5 = 988, F_C6 = 1047,
  F_CS6 = 1109, F_E6 = 1319, F_A6 = 1760 };

// --- the sounds ---
static const Note BOOT_A[] = { SL(180, 1400, 110, W_SQ12, 45), R(40),
  N(F_A4, 65, W_SQ25, 80), N(F_CS5, 65, W_SQ25, 80), N(F_E5, 65, W_SQ25, 80), N(F_A5, 65, W_SQ25, 85),
  N(F_CS6, 65, W_SQ25, 85), N(F_E6, 420, W_SQ25, 90) };
static const Note BOOT_B[] = { R(150), N(F_A2, 260, W_TRI, 100), N(F_E3, 130, W_TRI, 100), N(F_A3, 420, W_TRI, 100) };
static const Note OFF_A[]  = { N(F_E6, 60, W_SQ25, 80), N(F_CS6, 60, W_SQ25, 80), N(F_A5, 60, W_SQ25, 80),
  N(F_E5, 60, W_SQ25, 80), N(F_CS5, 60, W_SQ25, 80), SL(F_A4, 60, 420, W_SQ50, 70) };
static const Note OFF_B[]  = { R(300), SL(F_A3, 40, 420, W_TRI, 100) };
static const Note CONN_A[] = { N(F_E5, 70, W_SQ25, 80), N(F_B5, 70, W_SQ25, 85), N(F_E6, 160, W_SQ25, 85) };
static const Note DISC_A[] = { N(F_E6, 70, W_SQ25, 80), N(F_B5, 70, W_SQ25, 80), SL(F_E5, 330, 180, W_SQ25, 80) };
static const Note OPEN_A[] = { SL(500, 1200, 45, W_SQ25, 70), N(1200, 60, W_SQ12, 70) };
static const Note SAVE_A[] = { N(F_C6, 55, W_SQ25, 80), N(F_G5, 55, W_SQ25, 80), N(F_C6, 55, W_SQ25, 80), N(1568, 140, W_SQ25, 85) };
static const Note MOVE_A[] = { N(1800, 12, W_SQ12, 45) };
static const Note CHG_A[]  = { SL(900, 1350, 28, W_SQ25, 65) };
static const Note SEL_A[]  = { N(F_A5, 35, W_SQ25, 75), N(F_E6, 70, W_SQ25, 75) };
static const Note BACK_A[] = { N(F_E6, 35, W_SQ25, 75), N(F_A5, 70, W_SQ25, 75) };
static const Note TICK_A[] = { N(1000, 70, W_SQ50, 70) };
static const Note LOW_A[]  = { N(F_A4, 120, W_SQ50, 75), R(70), N(F_A4, 120, W_SQ50, 75), R(70), SL(F_A4, 220, 260, W_SQ50, 75) };
static const Note ERR_A[]  = { N(150, 140, W_SQ50, 80), R(40), N(110, 220, W_SQ50, 80) };
static const Note CLICK_A[] = { N(4000, 7, W_NOISE, 40) };
static const Note COMBO_A[] = { N(F_C6, 45, W_SQ25, 80), N(F_E6, 45, W_SQ25, 80), N(1568, 45, W_SQ25, 85), N(2093, 130, W_SQ12, 85) };

enum SoundId : uint8_t { SND_BOOT, SND_OFF, SND_CONNECT, SND_DISCONNECT, SND_MENU_OPEN, SND_SAVED,
  SND_MOVE, SND_CHANGE, SND_SELECT, SND_BACK, SND_TICK, SND_LOWBAT, SND_ERROR, SND_CLICK, SND_COMBO, SND_COUNT };
struct Sound { const Note *a; uint8_t na; const Note *b; uint8_t nb; uint8_t prio; bool queue; };
#define LEN(x) (uint8_t)(sizeof(x) / sizeof(x[0]))
// prio: a sound never cuts off one with a higher priority.
// queue: true = wait for the current sound to finish instead of cutting in.
static const Sound SOUNDS[SND_COUNT] = {
  {BOOT_A, LEN(BOOT_A), BOOT_B, LEN(BOOT_B), 3, true},    // SND_BOOT
  {OFF_A,  LEN(OFF_A),  OFF_B,  LEN(OFF_B),  4, false},   // SND_OFF
  {CONN_A, LEN(CONN_A), nullptr, 0, 2, true},             // SND_CONNECT
  {DISC_A, LEN(DISC_A), nullptr, 0, 2, true},             // SND_DISCONNECT
  {OPEN_A, LEN(OPEN_A), nullptr, 0, 1, false},            // SND_MENU_OPEN
  {SAVE_A, LEN(SAVE_A), nullptr, 0, 1, false},            // SND_SAVED
  {MOVE_A, LEN(MOVE_A), nullptr, 0, 1, false},            // SND_MOVE
  {CHG_A,  LEN(CHG_A),  nullptr, 0, 1, false},            // SND_CHANGE
  {SEL_A,  LEN(SEL_A),  nullptr, 0, 1, false},            // SND_SELECT
  {BACK_A, LEN(BACK_A), nullptr, 0, 1, false},            // SND_BACK
  {TICK_A, LEN(TICK_A), nullptr, 0, 2, false},            // SND_TICK (power-off countdown)
  {LOW_A,  LEN(LOW_A),  nullptr, 0, 2, true},             // SND_LOWBAT
  {ERR_A,  LEN(ERR_A),  nullptr, 0, 2, true},             // SND_ERROR (expander lost)
  {CLICK_A, LEN(CLICK_A), nullptr, 0, 0, false},          // SND_CLICK
  {COMBO_A, LEN(COMBO_A), nullptr, 0, 1, false},          // SND_COMBO (move recognised)
};
static const int16_t SND_LEVEL[4] = {0, 3000, 7000, 14000};   // Off / Low / Medium / High
static const char *SOUND_NAME[4] = {"Off", "Low", "Medium", "High"};

I2SClass i2s;
QueueHandle_t soundQueue = nullptr;
volatile bool soundOk = false;        // codec found and I2S running
volatile bool soundBusy = false;      // something is playing or waiting
volatile bool soundStopReq = false, soundStopped = false;

struct Voice {
  const Note *notes = nullptr; uint8_t n = 0, idx = 0;
  uint32_t pos = 0, len = 0, phase = 0; uint16_t lfsr = 0xACE1; int8_t noiseOut = 1;
  void start(const Note *ns, uint8_t count) { notes = ns; n = count; idx = 0; pos = 0; len = count ? ns[0].ms * (SND_RATE / 1000) : 0; }
  bool active() const { return notes && idx < n; }
  int32_t next() {                      // one sample, roughly -1000..1000
    if (!active()) return 0;
    const Note &nt = notes[idx];
    int32_t out = 0;
    if (nt.wave != W_REST) {
      float f = nt.f1 ? nt.f0 + (float)(nt.f1 - nt.f0) * pos / len : nt.f0;
      uint32_t prev = phase;
      phase += (uint32_t)(f * (4294967296.0f / SND_RATE));
      uint8_t ph = phase >> 24;
      int32_t w;
      switch (nt.wave) {
        case W_SQ12: w = ph < 32 ? 1000 : -1000; break;
        case W_SQ25: w = ph < 64 ? 1000 : -1000; break;
        case W_SQ50: w = ph < 128 ? 1000 : -1000; break;
        case W_TRI:  w = ph < 128 ? -1000 + ph * 2000 / 128 : 1000 - (ph - 128) * 2000 / 128; break;
        default:     // noise: new random level each cycle
          if (phase < prev) { uint16_t bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1;
                              lfsr = (lfsr >> 1) | (bit << 15); noiseOut = (lfsr & 1) ? 1 : -1; }
          w = noiseOut * 1000; break;
      }
      // envelope: 3 ms attack, 12 ms release, gentle decay across the note
      const uint32_t att = SND_RATE * 3 / 1000, rel = SND_RATE * 12 / 1000;
      float env = 1.0f - 0.35f * pos / len;
      if (pos < att) env *= (float)pos / att;
      if (len - pos < rel) env *= (float)(len - pos) / rel;
      out = (int32_t)(w * env) * nt.vol / 100;
    }
    if (++pos >= len) { if (++idx < n) { pos = 0; len = notes[idx].ms * (SND_RATE / 1000); } }
    return out;
  }
};

bool esWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(ES8311_ADDR); Wire.write(reg); Wire.write(val);
  return Wire.endTransmission() == 0;
}
uint8_t esRead(uint8_t reg) {
  Wire.beginTransmission(ES8311_ADDR); Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0;
  if (Wire.requestFrom((uint8_t)ES8311_ADDR, (uint8_t)1) != 1) return 0;
  return Wire.read();
}

// ES8311 set-up for 24 kHz, 16-bit I2S, MCLK = 256 x 24 kHz from the ESP32.
// Register sequence follows Espressif's es8311 driver (Apache-2.0), as used
// in Freenove's FNK0104 examples; done here with Wire so it shares the bus.
bool codecInit() {
  if (!esWrite(0x00, 0x1F)) return false;          // reset (also tells us the chip is there)
  delay(20);
  esWrite(0x00, 0x00);
  esWrite(0x00, 0x80);                             // power on, slave mode
  esWrite(0x01, 0x3F);                             // clocks on, MCLK from the MCLK pin
  esWrite(0x02, esRead(0x02) & 0x07);              // pre-divide 1, multiply 1
  esWrite(0x03, 0x10);                             // ADC single speed, OSR 16
  esWrite(0x04, 0x10);                             // DAC OSR 16
  esWrite(0x05, 0x00);                             // ADC/DAC clock divide 1
  esWrite(0x06, (esRead(0x06) & 0xE0) | 0x03);     // BCLK not inverted, divide 4
  esWrite(0x07, esRead(0x07) & 0xC0);              // LRCK divider high = 0
  esWrite(0x08, 0xFF);                             // LRCK divider low  = 255 (256 x fs)
  esWrite(0x00, esRead(0x00) & 0xBF);              // serial port slave
  esWrite(0x09, 0x0C);                             // DAC input: I2S, 16-bit
  esWrite(0x0A, 0x0C);                             // ADC output: I2S, 16-bit
  esWrite(0x0D, 0x01);                             // power up analog
  esWrite(0x0E, 0x02);
  esWrite(0x12, 0x00);                             // power up DAC
  esWrite(0x13, 0x10);                             // enable output driver
  esWrite(0x1C, 0x6A);
  esWrite(0x37, 0x08);                             // bypass DAC EQ
  esWrite(0x31, 0x00);                             // unmute
  return esWrite(0x32, 0xBF);                      // DAC volume ~75 %; level is set in software
}

void applySoundSettings() {
  digitalWrite(PIN_AMP_EN, (soundOk && cfg.sound) ? LOW : HIGH);   // amp off when sound is Off
}

void playSound(SoundId id) {
  if (!soundOk || !soundQueue || cfg.sound == 0) return;
  if (id == SND_CLICK && soundBusy) return;          // clicks never pile up
  uint8_t v = id;
  xQueueSend(soundQueue, &v, 0);
}

void soundTask(void *) {
  static int16_t buf[SND_FRAMES * 2];
  Voice va, vb;
  uint8_t curPrio = 0;
  uint8_t pending[6]; uint8_t pendN = 0;              // queued (wait-your-turn) sounds
  for (;;) {
    if (soundStopReq) { soundStopped = true; vTaskDelay(portMAX_DELAY); }
    uint8_t id;
    while (xQueueReceive(soundQueue, &id, 0) == pdTRUE) {
      const Sound &sd = SOUNDS[id];
      bool playing = va.active() || vb.active();
      if (sd.queue && playing) { if (pendN < sizeof pending) pending[pendN++] = id; continue; }
      if (playing && sd.prio < curPrio) continue;      // don't cut off something more important
      if (!sd.queue) pendN = 0;                        // a UI sound clears the backlog
      va.start(sd.a, sd.na); vb.start(sd.b, sd.nb); curPrio = sd.prio;
    }
    if (!va.active() && !vb.active() && pendN) {
      id = pending[0]; memmove(pending, pending + 1, --pendN);
      const Sound &sd = SOUNDS[id];
      va.start(sd.a, sd.na); vb.start(sd.b, sd.nb); curPrio = sd.prio;
    }
    bool active = va.active() || vb.active();
    soundBusy = active || pendN || uxQueueMessagesWaiting(soundQueue);
    int32_t level = SND_LEVEL[cfg.sound];
    for (int i = 0; i < SND_FRAMES; i++) {
      int32_t smp = active ? (va.next() + vb.next()) * level / 1000 : 0;
      smp = constrain(smp, -32000, 32000);
      buf[2 * i] = buf[2 * i + 1] = (int16_t)smp;
    }
    if (!active) curPrio = 0;
    i2s.write((uint8_t *)buf, sizeof buf);            // blocks until there's room: paces the task
  }
}

// Call after Wire.begin(). Safe if the codec isn't there (sound just stays off).
void soundBegin() {
  pinMode(PIN_AMP_EN, OUTPUT);
  digitalWrite(PIN_AMP_EN, HIGH);                    // amp off until everything is running
  i2s.setPins(PIN_I2S_BCLK, PIN_I2S_WS, PIN_I2S_DOUT, -1, PIN_I2S_MCLK);
  if (!i2s.begin(I2S_MODE_STD, SND_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) return;
  if (!codecInit()) { i2s.end(); return; }
  soundQueue = xQueueCreate(8, 1);
  soundOk = true;
  xTaskCreatePinnedToCore(soundTask, "snd", 4096, nullptr, 2, nullptr, 0);
  delay(30);                                         // let silence reach the DAC, then amp on
  applySoundSettings();
}

// Wait for the current sound to finish (up to maxMs), then stop audio for sleep.
void soundShutdown(uint32_t maxMs) {
  if (!soundOk) return;
  uint32_t t0 = millis();
  delay(30);
  while (soundBusy && millis() - t0 < maxMs) delay(10);
  digitalWrite(PIN_AMP_EN, HIGH);                    // amp off first: no pop
  soundStopReq = true;
  t0 = millis();
  while (!soundStopped && millis() - t0 < 200) delay(5);
  i2s.end();
  soundOk = false;
}

// ======================= UI =========================================
enum Screen : uint8_t { S_STATUS, S_MENU, S_REMAP, S_TURBO, S_TEST, S_MESSAGE, S_UPDATE, S_WIFI, S_ONLINE };
Screen screen = S_STATUS;
int menuIndex = 0;
int remapSel = 0;                    // which action button is being edited
bool settingsDirty = false;
const char *messageText = "";
uint32_t messageUntil = 0;

enum MenuItem { M_PROFILE, M_REMAP, M_TURBO, M_STICK, M_GAME, M_FIGHTER, M_MOVECHECK, M_CONN, M_FORGET, M_BRIGHT, M_SLEEP, M_AUTOOFF,
                M_SOUND, M_CLICKS, M_THEME, M_PLAYER, M_SOCD, M_FLIP, M_TEST, M_ONLINE, M_UPDATE, M_WIFI, M_EXIT, M_COUNT };
const char *MENU_LABEL[M_COUNT] = {
  "Profile", "Remap buttons", "Turbo", "Stick", "Game", "Fighter", "Move check", "Connection",
  "Forget BT pairings", "Brightness", "Screen sleep", "Auto power off", "Sound", "Button clicks",
  "Colour", "Player", "Left + Right", "Flip screen", "Input test", "Check for update", "USB update", "Wi-Fi update", "Save & exit"
};

// colours (RGB565)
static constexpr uint16_t C_BG = 0x0861;   // near-black blue
static constexpr uint16_t C_PANEL = 0x18E3;
static constexpr uint16_t C_LINE = 0x4A69;
static constexpr uint16_t C_TEXT = 0xFFFF;
static constexpr uint16_t C_DIM = 0x9CF3;
uint16_t C_ACCENT = STICK_ACCENT;                // set from the Colour setting
static constexpr uint16_t C_TURBO = 0x07FF;   // cyan
static constexpr uint16_t C_OK = 0x47E8;   // green
static constexpr uint16_t C_BLUE = 0x3C9F;
static constexpr uint16_t C_WARN = 0xF9A6;   // red

// Button positions on screen, from the case's Vewlix layout (mm -> px)
struct Pt { int16_t x, y; };
Pt btnPos(uint8_t i) {
  static const int16_t mm[6][2] = {{66, 10}, {99, 24}, {135, 24}, {59, -28}, {92, -15}, {128, -15}};
  if (i >= 6) return {292, 196};       // EX shown bottom-right
  return {(int16_t)(150 + (mm[i][0] - 59) * 1.6f), (int16_t)(112 - mm[i][1] * 1.6f)};
}

void applyTheme() { C_ACCENT = THEME_COL[cfg.theme]; }

// Toast: short message over the status screen (profile / page / fighter changes)
const char *toastText = nullptr;
uint32_t toastUntil = 0;
void toast(const char *t) { toastText = t; toastUntil = millis() + 1300; }

void applyDisplaySettings() {
  tft.setRotation(cfg.flip ? 3 : 1);
  tft.setBrightness(map(cfg.brightness, 0, 100, 0, 255));
}

void drawHeader(LGFX_Sprite &g, const char *title) {
  g.fillRect(0, 0, 320, 28, C_PANEL);
  g.setFont(&fonts::FreeSansBold9pt7b);
  g.setTextColor(C_TEXT);
  g.setTextDatum(middle_left);
  g.drawString(title, 10, 14);
  int tw = g.textWidth(title);
  char pl[4]; snprintf(pl, sizeof pl, "P%d", cfg.player + 1);
  g.setFont(&fonts::Font2);
  g.fillRoundRect(18 + tw, 6, 26, 16, 8, C_ACCENT);
  g.setTextColor(C_BG);
  g.setTextDatum(middle_center);
  g.drawString(pl, 31 + tw, 14);

  const char *txt; uint16_t col;
  if (!mcpOk) { txt = "NO EXPANDER"; col = C_WARN; }
  else if (bootConn == C_USB) { txt = usbMounted ? "USB" : "USB: no host"; col = usbMounted ? C_OK : C_ACCENT; }
  else if (radioOff) { txt = "BT off"; col = C_DIM; }
  else { bool c = hostConnected(); txt = c ? "BLUETOOTH" : "BT pairing"; col = c ? C_BLUE : C_ACCENT; }
  g.setFont(&fonts::Font2);
  int w = g.textWidth(txt) + 22;
  g.fillRoundRect(314 - w, 5, w, 18, 9, C_BG);
  g.fillCircle(314 - w + 9, 14, 4, col);
  g.setTextColor(col);
  g.setTextDatum(middle_left);
  g.drawString(txt, 314 - w + 17, 14);
}

// ---- small drawing helpers ----
static const int8_t DIR_VEC[10][2] = {{0,0},{-1,1},{0,1},{1,1},{-1,0},{0,0},{1,0},{-1,-1},{0,-1},{1,-1}};
void drawArrow(LGFX_Sprite &g, int cx, int cy, uint8_t d, uint16_t col, int size = 7) {
  if (d == 5 || d > 9) { g.fillCircle(cx, cy, 2, col); return; }
  float vx = DIR_VEC[d][0], vy = DIR_VEC[d][1];
  if (vx && vy) { vx *= 0.72f; vy *= 0.72f; }
  int x0 = cx - vx * size, y0 = cy - vy * size, x1 = cx + vx * size, y1 = cy + vy * size;
  g.drawWideLine(x0, y0, x1, y1, 1.6f, col);
  // arrow head
  float px = -vy, py = vx;
  g.fillTriangle(x1 + vx * 3, y1 + vy * 3, x1 - vx * 3 + px * 4, y1 - vy * 3 + py * 4,
                 x1 - vx * 3 - px * 4, y1 - vy * 3 - py * 4, col);
}
// draws a move's input (first alternative) at x, returns the x after it
int drawMotion(LGFX_Sprite &g, int x, int cy, const char *m, uint16_t col) {
  g.setFont(&fonts::Font2);
  g.setTextColor(col);
  g.setTextDatum(middle_left);
  if (!strncmp(m, "tap", 3)) { g.drawString("rapid", x, cy); return x + g.textWidth("rapid") + 4; }
  if (!strncmp(m, "hold", 4)) { g.drawString("hold, let go", x, cy); return x + g.textWidth("hold, let go") + 4; }
  if (!strncmp(m, "360", 3)) {
    g.drawCircle(x + 9, cy, 8, col); g.drawCircle(x + 9, cy, 7, col);
    g.fillTriangle(x + 15, cy - 9, x + 21, cy - 6, x + 15, cy - 2, col);
    return x + 26;
  }
  if (m[0] == '[') {                                      // charge: held arrow, then release arrow
    drawArrow(g, x + 9, cy, m[1] - '0', col);
    g.drawFastHLine(x + 1, cy + 10, 17, col);
    g.setFont(&fonts::Font0); g.setTextDatum(middle_center);
    g.drawString("hold", x + 9, cy - 11);
    drawArrow(g, x + 30, cy, m[3] - '0', col);
    return x + 42;
  }
  for (const char *c = m; *c && *c != '|'; c++) { drawArrow(g, x + 9, cy, *c - '0', col); x += 19; }
  return x;
}
int drawBtnChip(LGFX_Sprite &g, int x, int cy, const char *spec, const Game &gm) {
  if (!spec[0]) return x;
  bool hold = !strncmp(spec, "hold ", 5);
  const char *txt = hold ? spec + 5 : spec;
  // colour: punches accent, kicks cyan, anything else (block, run) grey
  uint16_t bg = C_DIM;
  if (txt[0] == 'P') bg = C_ACCENT;
  else if (txt[0] == 'K' && (txt[1] == 0 || txt[1] == 'K' || txt[1] == '+')) bg = C_TURBO;
  else for (uint8_t i = 0; i < 6; i++) {
    size_t l = strlen(gm.btnName[i]);
    if (l && !strncmp(txt, gm.btnName[i], l) && (txt[l] == 0 || txt[l] == '+'))
      { bg = (gm.punchMask >> i) & 1 ? C_ACCENT : ((gm.kickMask >> i) & 1 ? C_TURBO : C_DIM); break; }
  }
  g.setFont(&fonts::Font2);
  if (hold) {
    g.setTextColor(C_DIM); g.setTextDatum(middle_left);
    g.drawString("hold", x, cy); x += g.textWidth("hold") + 4;
  }
  int w = g.textWidth(txt) + 10;
  g.fillRoundRect(x, cy - 8, w, 16, 8, bg);
  g.setTextColor(C_BG);
  g.setTextDatum(middle_center);
  g.drawString(txt, x + w / 2, cy);
  return x + w + 6;
}

void drawPageButtons(LGFX_Sprite &g, uint16_t s) {
  const uint8_t *amap = activeMap();
  // joystick
  const int jx = 62, jy = 112;
  g.drawCircle(jx, jy, 44, C_LINE);
  g.drawCircle(jx, jy, 45, C_LINE);
  int dx = held(s, P_RIGHT) - held(s, P_LEFT);
  int dy = held(s, P_DOWN) - held(s, P_UP);
  bool moved = dx || dy;
  int bx = jx + dx * 24, by = jy + dy * 24;
  g.drawLine(jx, jy, bx, by, moved ? C_ACCENT : C_LINE);
  g.fillCircle(bx, by, 16, moved ? C_ACCENT : C_PANEL);
  g.drawCircle(bx, by, 16, moved ? C_ACCENT : C_DIM);
  g.setFont(&fonts::Font2);
  g.setTextColor(C_DIM);
  g.setTextDatum(top_center);
  g.drawString(STICK_NAME[cfg.stickMode], jx, jy + 52);

  // action buttons
  for (uint8_t i = 0; i < ACT_COUNT; i++) {
    Pt p = btnPos(i);
    int rad = i < 6 ? 22 : 13;
    bool on = held(s, ACT_FIRST + i);
    bool turbo = cfg.turboMask & (1 << i);
    if (turbo) g.drawCircle(p.x, p.y, rad + 3, C_TURBO);
    g.fillCircle(p.x, p.y, rad, on ? C_ACCENT : C_PANEL);
    g.drawCircle(p.x, p.y, rad, on ? C_ACCENT : C_LINE);
    if (i < 6) g.setFont(&fonts::FreeSansBold9pt7b); else g.setFont(&fonts::Font2);
    g.setTextColor(on ? C_BG : C_TEXT);
    g.setTextDatum(middle_center);
    g.drawString(OUT_NAME[amap[i]], p.x, p.y + 1);
  }

  // Select / Start / Home
  const char *small[3] = {"SELECT", "START", "HOME"};
  const uint8_t sp[3] = {P_SELECT, P_START, P_HOME};
  for (int i = 0; i < 3; i++) {
    int x = 22 + i * 62, y = 196;
    bool on = held(s, sp[i]);
    g.fillRoundRect(x - 2, y - 9, 56, 18, 9, on ? C_ACCENT : C_PANEL);
    g.setFont(&fonts::Font0);
    g.setTextColor(on ? C_BG : C_DIM);
    g.setTextDatum(middle_center);
    g.drawString(small[i], x + 26, y);
  }

}

// Inputs page: recent inputs, newest at the top, with the gap in frames (1/60 s)
void drawPageInputs(LGFX_Sprite &g) {
  const uint8_t *amap = activeMap();
  g.setFont(&fonts::Font0);
  g.setTextColor(C_DIM);
  g.setTextDatum(top_left);
  g.drawString("RECENT INPUTS", 10, 33);
  g.setTextDatum(top_right);
  g.drawString("FRAMES", 310, 33);
  uint8_t head = histHead;
  for (int i = 0; i < 9; i++) {
    const HistEntry &e = inputHist[(head + HIST_LEN - 1 - i) % HIST_LEN];
    const HistEntry &prev = inputHist[(head + HIST_LEN - 2 - i) % HIST_LEN];
    if (!e.t) break;
    int y = 52 + i * 18;
    uint16_t col = i == 0 ? C_TEXT : C_DIM;
    if (i == 0) g.fillRoundRect(4, y - 9, 312, 18, 5, C_PANEL);
    drawArrow(g, 22, y, e.dir, i == 0 ? C_ACCENT : col, 6);
    int x = 42;
    for (uint8_t b = 0; b < ACT_COUNT; b++)
      if (e.btns & (1u << (ACT_FIRST + b))) {
        g.setFont(&fonts::Font2);
        const char *nm = OUT_NAME[amap[b]];
        int w = g.textWidth(nm) + 8;
        g.fillRoundRect(x, y - 7, w, 14, 7, b < 3 ? C_ACCENT : C_TURBO);
        g.setTextColor(C_BG); g.setTextDatum(middle_center);
        g.drawString(nm, x + w / 2, y);
        x += w + 4;
      }
    g.setFont(&fonts::Font2); g.setTextColor(col); g.setTextDatum(middle_left);
    if (e.btns & (1u << P_START))  { g.drawString("START", x, y);  x += 44; }
    if (e.btns & (1u << P_SELECT)) { g.drawString("SELECT", x, y); x += 50; }
    if (prev.t && i < 8) {
      char buf[8];
      uint32_t f = (e.t - prev.t) * 60 / 1000;
      snprintf(buf, sizeof buf, "%lu", (unsigned long)min<uint32_t>(f, 999));
      g.setTextDatum(middle_right);
      g.drawString(buf, 308, y);
    }
  }
}

// Moves page: the selected fighter's special moves; recognised moves light up
uint16_t moveCount[8];
void drawPageMoves(LGFX_Sprite &g) {
  const Game &gm = GAMES[cfg.game];
  const Fighter &f = gm.fighters[cfg.fighter];
  g.setFont(&fonts::FreeSansBold12pt7b);
  g.setTextColor(C_TEXT);
  g.setTextDatum(middle_center);
  g.drawString(f.name, 160, 46);
  g.setFont(&fonts::Font0);
  g.setTextColor(C_DIM);
  char buf[64];
  snprintf(buf, sizeof buf, "%s   (%d/%d)   change in the menu", gm.name, cfg.fighter + 1, gm.n);
  g.drawString(buf, 160, 64);

  uint32_t now = millis();
  bool fresh = cfg.moveCheck && moveHit >= 0 && now - moveHitAt < 1500;
  for (uint8_t i = 0; i < f.n && i < 6; i++) {
    const Move &m = f.moves[i];
    int y = 82 + i * 20;
    bool lit = fresh && moveHit == i;
    if (lit) g.fillRoundRect(4, y - 10, 312, 20, 6, C_ACCENT);
    uint16_t col = lit ? C_BG : C_TEXT;
    int x = drawMotion(g, 10, y, m.motion, col);
    if (m.motion[0]) x += 2;
    x = drawBtnChip(g, x, y, m.btn, gm);
    g.setFont(&fonts::Font2);
    g.setTextColor(col);
    g.setTextDatum(middle_left);
    g.drawString(m.name, max(x + 4, 150), y);
    if (moveCount[i] && cfg.moveCheck) {
      snprintf(buf, sizeof buf, "x%u", moveCount[i]);
      g.setTextDatum(middle_right);
      g.setTextColor(lit ? C_BG : C_DIM);
      g.drawString(buf, 310, y);
    }
  }
  // banner
  if (fresh) {
    g.setFont(&fonts::FreeSansBold12pt7b);
    g.setTextColor(C_ACCENT);
    g.setTextDatum(middle_center);
    g.drawString(f.moves[moveHit].name, 160, 200);
    if (moveHitMirror) {
      g.setFont(&fonts::Font0); g.setTextColor(C_DIM);
      g.drawString("(facing left)", 160, 213);
    }
  } else {
    g.setFont(&fonts::Font0);
    g.setTextColor(C_DIM);
    g.setTextDatum(middle_center);
    g.drawString(cfg.moveCheck ? "Do a move and it lights up here. Works facing either way."
                               : "Move check is off (turn it on in the menu)", 160, 206);
  }
}

void drawFooter(LGFX_Sprite &g) {
  g.fillRect(0, 216, 320, 24, C_PANEL);
  g.setFont(&fonts::Font2);
  g.setTextColor(C_TEXT);
  g.setTextDatum(middle_left);
  g.drawString(PROFILE_NAME[cfg.profile], 8, 228);
  // page dots
  for (int i = 0; i < PAGE_COUNT; i++) {
    int x = 148 + i * 12;
    if (i == cfg.page) g.fillCircle(x, 228, 4, C_ACCENT); else g.drawCircle(x, 228, 3, C_DIM);
  }
  int pct = batteryPercent();
  char buf[24];
  uint16_t col = C_DIM;
  if (pct < 0) snprintf(buf, sizeof buf, "USB power");
  else if (chargeState == 1) { snprintf(buf, sizeof buf, "Charging"); col = C_OK; }
  else if (chargeState == 2) { snprintf(buf, sizeof buf, "Charged"); col = C_OK; }
  else { snprintf(buf, sizeof buf, "Battery %d%%", pct); if (pct < 15) col = C_WARN; }
  g.setTextDatum(middle_right);
  g.setTextColor(col);
  g.drawString(buf, 312, 228);
}

// While HOME is held: what each button does
void drawShortcutHelp(LGFX_Sprite &g) {
  g.fillRoundRect(14, 34, 292, 176, 10, C_PANEL);
  g.drawRoundRect(14, 34, 292, 176, 10, C_ACCENT);
  g.setFont(&fonts::Font2);
  g.setTextColor(C_ACCENT);
  g.setTextDatum(top_center);
  g.drawString("SELECT shortcuts", 160, 40);
  for (uint8_t i = 0; i < 6; i++) {               // the six buttons as on the panel
    int col = i % 3, row = i / 3;
    int x = 30 + col * 92, y = 64 + row * 40;
    g.fillRoundRect(x, y, 84, 32, 8, C_BG);
    g.setTextColor(C_DIM); g.setTextDatum(top_center); g.setFont(&fonts::Font0);
    char b[4]; snprintf(b, sizeof b, "B%d", i + 1);
    g.drawString(b, x + 42, y + 3);
    g.setTextColor(C_TEXT); g.setFont(&fonts::Font2);
    g.drawString(HOTKEYS[i].name, x + 42, y + 13);
  }
  g.setFont(&fonts::Font2);
  g.setTextDatum(top_left);
  g.setTextColor(C_TEXT);
  g.drawString("Left/Right  profile", 30, 148);
  g.drawString("Up/Down  page", 170, 148);
  g.drawString("START  next fighter (Moves page)", 30, 168);
  g.setTextColor(C_DIM);
  g.setTextDatum(top_center);
  g.drawString("let go of SELECT when done", 160, 189);
}

void drawStatus(LGFX_Sprite &g) {
  uint16_t s = physState;
  drawHeader(g, DEVICE_NAME);
  switch (cfg.page) {
    case PG_INPUTS: drawPageInputs(g); break;
    case PG_MOVES:  drawPageMoves(g); break;
    default:        drawPageButtons(g, s); break;
  }
  drawFooter(g);
  uint32_t now = millis();
  if (selLayer) drawShortcutHelp(g);
  if (toastText && (int32_t)(toastUntil - now) > 0) {
    g.setFont(&fonts::FreeSansBold12pt7b);
    int w = g.textWidth(toastText) + 30;
    g.fillRoundRect(160 - w / 2, 96, w, 40, 10, C_ACCENT);
    g.setTextColor(C_BG);
    g.setTextDatum(middle_center);
    g.drawString(toastText, 160, 117);
  }
  // power-off countdown while START is held
  uint32_t sd = startDownAt;
  if (sd && millis() - sd >= 1500) {
    uint32_t left = (POWER_OFF_HOLD_MS - min<uint32_t>(millis() - sd, POWER_OFF_HOLD_MS) + 999) / 1000;
    g.fillRoundRect(50, 88, 220, 64, 10, C_PANEL);
    g.drawRoundRect(50, 88, 220, 64, 10, C_ACCENT);
    g.setFont(&fonts::FreeSansBold12pt7b);
    g.setTextColor(C_TEXT);
    g.setTextDatum(middle_center);
    char buf[24];
    snprintf(buf, sizeof buf, "Power off in %lu", (unsigned long)left);
    g.drawString(buf, 160, 112);
    g.setFont(&fonts::Font2);
    g.setTextColor(C_DIM);
    g.drawString("keep holding START", 160, 138);
  }
}

// Attract mode: shown (dimmed) when the stick has been idle for a while
void drawAttract(LGFX_Sprite &g) {
  struct Star { int16_t x, y; uint8_t v; };
  static Star stars[60];
  static bool init = false;
  if (!init) { for (auto &st : stars) st = {(int16_t)random(320), (int16_t)random(240), (uint8_t)random(1, 4)}; init = true; }
  g.fillScreen(TFT_BLACK);
  for (auto &st : stars) {
    st.x -= st.v; if (st.x < 0) { st.x = 319; st.y = random(240); }
    uint16_t c = st.v == 3 ? C_TEXT : (st.v == 2 ? C_DIM : C_LINE);
    if (st.v == 3) g.fillRect(st.x, st.y, 2, 2, c); else g.drawPixel(st.x, st.y, c);
  }
  uint32_t t = millis();
  int bob = (int)(sinf(t / 600.0f) * 6);
  g.setTextDatum(middle_center);
  g.setFont(&fonts::FreeSansBold18pt7b);
  g.setTextColor(C_LINE);
  g.drawString("ARCADE STICK", 162, 82 + bob + 2);
  g.setTextColor(C_ACCENT);
  g.drawString("ARCADE STICK", 160, 80 + bob);
  g.setFont(&fonts::FreeSansBold9pt7b);
  g.setTextColor(C_TEXT);
  char buf[32]; snprintf(buf, sizeof buf, "%s  -  PLAYER %d", STICK_COLOUR_NAME, cfg.player + 1);
  g.drawString(buf, 160, 120);
  if ((t / 600) & 1) {
    g.setFont(&fonts::FreeSansBold12pt7b);
    g.setTextColor(C_ACCENT);
    g.drawString("INSERT COIN", 160, 172);
  }
  g.setFont(&fonts::Font0);
  g.setTextColor(C_DIM);
  g.drawString("press any button", 160, 196);
  int pct = batteryPercent();
  if (pct >= 0) { snprintf(buf, sizeof buf, "%d%%", pct); g.setTextDatum(bottom_right); g.drawString(buf, 314, 236); }
}

void menuValue(int item, char *buf, size_t n) {
  buf[0] = 0;
  switch (item) {
    case M_PROFILE: snprintf(buf, n, "< %s >", PROFILE_NAME[cfg.profile]); break;
    case M_STICK:   snprintf(buf, n, "< %s >", STICK_NAME[cfg.stickMode]); break;
    case M_CONN:    snprintf(buf, n, "< %s >%s", cfg.conn == C_USB ? "USB" : "Bluetooth",
                             cfg.conn != bootConn ? " *" : ""); break;
    case M_BRIGHT:  snprintf(buf, n, "< %d%% >", cfg.brightness); break;
    case M_FLIP:    snprintf(buf, n, "< %s >", cfg.flip ? "On" : "Off"); break;
    case M_SOUND:   snprintf(buf, n, soundOk || !cfg.sound ? "< %s >" : "< %s > (no speaker)", SOUND_NAME[cfg.sound]); break;
    case M_CLICKS:  snprintf(buf, n, "< %s >", cfg.clicks ? "On" : "Off"); break;
    case M_SLEEP:   snprintf(buf, n, "< %s >", SLEEP_NAME[cfg.screenSleep]); break;
    case M_AUTOOFF: snprintf(buf, n, "< %s >", AUTOOFF_NAME[cfg.autoOff]); break;
    case M_THEME:   snprintf(buf, n, "< %s >", THEME_NAME[cfg.theme]); break;
    case M_PLAYER:  snprintf(buf, n, "< P%d >", cfg.player + 1); break;
    case M_SOCD:    snprintf(buf, n, "< %s >", SOCD_NAME[cfg.socd]); break;
    case M_GAME:    snprintf(buf, n, "< %s >", GAMES[cfg.game].name); break;
    case M_FIGHTER: snprintf(buf, n, "< %s >", GAMES[cfg.game].fighters[cfg.fighter].name); break;
    case M_MOVECHECK: snprintf(buf, n, "< %s >", cfg.moveCheck == 0 ? "Off" : (cfg.moveCheck == 1 ? "On, no sound" : "On + sound")); break;
    case M_REMAP: case M_TURBO: case M_TEST: case M_ONLINE: case M_UPDATE: case M_WIFI: snprintf(buf, n, ">"); break;
    default: break;
  }
}

void drawFooterHint(LGFX_Sprite &g, const char *hint) {
  g.fillRect(0, 216, 320, 24, C_PANEL);
  g.setFont(&fonts::Font2);
  g.setTextColor(C_DIM);
  g.setTextDatum(middle_center);
  g.drawString(hint, 160, 228);
}

void drawMenu(LGFX_Sprite &g) {
  drawHeader(g, "MENU");
  const int rowH = 18, top = 31, rows = 10;
  static int first = 0;                    // scroll so the selected row is visible
  if (menuIndex < first) first = menuIndex;
  if (menuIndex >= first + rows) first = menuIndex - rows + 1;
  for (int i = first; i < M_COUNT && i < first + rows; i++) {
    int y = top + (i - first) * rowH;
    bool sel = i == menuIndex;
    bool disabled = (i == M_FORGET && bootConn != C_BLE);
    if (sel) g.fillRoundRect(4, y, 312, rowH - 2, 5, C_ACCENT);
    g.setFont(&fonts::Font2);
    g.setTextColor(sel ? C_BG : (disabled ? C_LINE : C_TEXT));
    g.setTextDatum(middle_left);
    g.drawString(MENU_LABEL[i], 12, y + rowH / 2 - 1);
    char v[32];
    menuValue(i, v, sizeof v);
    g.setTextDatum(middle_right);
    g.drawString(v, 308, y + rowH / 2 - 1);
  }
  if (first > 0) g.fillTriangle(316, 33, 312, 39, 320, 39, C_DIM);                 // more above
  if (first + rows < M_COUNT) g.fillTriangle(316, 211, 312, 205, 320, 205, C_DIM); // more below
  drawFooterHint(g, cfg.conn != bootConn ? "* applies after Save & exit (restarts)"
                                         : "Stick: move/change  B4: select  HOME: back");
}

void drawRemap(LGFX_Sprite &g) {
  drawHeader(g, "REMAP");
  g.setFont(&fonts::Font2);
  g.setTextColor(C_DIM);
  g.setTextDatum(top_center);
  g.drawString("Press a button, then Up/Down to change it", 160, 34);
  for (uint8_t i = 0; i < ACT_COUNT; i++) {
    Pt p = btnPos(i);
    int rad = i < 6 ? 22 : 13;
    bool sel = i == remapSel;
    g.fillCircle(p.x, p.y, rad, sel ? C_ACCENT : C_PANEL);
    g.drawCircle(p.x, p.y, rad, sel ? C_ACCENT : C_LINE);
    if (i < 6) g.setFont(&fonts::FreeSansBold9pt7b); else g.setFont(&fonts::Font2);
    g.setTextColor(sel ? C_BG : C_TEXT);
    g.setTextDatum(middle_center);
    g.drawString(OUT_NAME[cfg.custom[i]], p.x, p.y + 1);
  }
  g.setFont(&fonts::Font2);
  g.setTextColor(C_DIM);
  g.setTextDatum(middle_center);
  g.drawString(PHYS_NAME[ACT_FIRST + remapSel], 62, 90);
  g.setFont(&fonts::FreeSansBold12pt7b);
  g.setTextColor(C_TEXT);
  g.drawString(OUT_NAME[cfg.custom[remapSel]], 62, 116);
  g.setFont(&fonts::Font2);
  g.setTextColor(C_DIM);
  g.drawString("(editing Custom)", 62, 142);
  drawFooterHint(g, "Up/Down: change   HOME: back");
}

void drawTurbo(LGFX_Sprite &g) {
  drawHeader(g, "TURBO");
  g.setFont(&fonts::Font2);
  g.setTextColor(C_DIM);
  g.setTextDatum(top_center);
  g.drawString("Press a button to toggle its turbo", 160, 34);
  const uint8_t *amap = activeMap();
  for (uint8_t i = 0; i < ACT_COUNT; i++) {
    Pt p = btnPos(i);
    int rad = i < 6 ? 22 : 13;
    bool t = cfg.turboMask & (1 << i);
    g.fillCircle(p.x, p.y, rad, t ? C_TURBO : C_PANEL);
    g.drawCircle(p.x, p.y, rad, t ? C_TURBO : C_LINE);
    if (i < 6) g.setFont(&fonts::FreeSansBold9pt7b); else g.setFont(&fonts::Font2);
    g.setTextColor(t ? C_BG : C_TEXT);
    g.setTextDatum(middle_center);
    g.drawString(OUT_NAME[amap[i]], p.x, p.y + 1);
  }
  g.setFont(&fonts::FreeSansBold12pt7b);
  g.setTextColor(C_TEXT);
  g.setTextDatum(middle_center);
  char buf[16];
  snprintf(buf, sizeof buf, "%d Hz", TURBO_HZ[cfg.turboHzIdx]);
  g.drawString(buf, 62, 112);
  g.setFont(&fonts::Font2);
  g.setTextColor(C_DIM);
  g.drawString("Left/Right: speed", 62, 140);
  drawFooterHint(g, "Buttons: toggle   HOME: back");
}

void drawTest(LGFX_Sprite &g) {
  drawHeader(g, "INPUT TEST");
  uint16_t s = physState;
  g.setFont(&fonts::Font2);
  for (uint8_t p = 0; p < PHYS_COUNT; p++) {
    int col = p < 7 ? 0 : 1;
    int row = p < 7 ? p : p - 7;
    int x = 8 + col * 158, y = 34 + row * 25;
    bool on = held(s, p);
    g.fillRoundRect(x, y, 148, 22, 5, on ? C_ACCENT : C_PANEL);
    g.setTextColor(on ? C_BG : C_TEXT);
    g.setTextDatum(middle_left);
    g.drawString(PHYS_NAME[p], x + 8, y + 11);
    g.setTextColor(on ? C_BG : C_DIM);
    g.setTextDatum(middle_right);
    g.drawString(PHYS_PIN[p], x + 140, y + 11);
  }
  drawFooterHint(g, mcpOk ? "Hold HOME 2 s to go back" : "MCP23017 not found at 0x20 - check wiring");
}

void drawUpdate(LGFX_Sprite &g) {
  drawHeader(g, "USB UPDATE");
  g.setFont(&fonts::Font2);
  g.setTextDatum(top_left);
  const char *lines[] = {
    "Restarts the stick into flash mode, so you",
    "can load new firmware without opening it.",
    "",
    "1. Plug USB-C into the computer",
    "2. Press B4 here (the screen goes dark)",
    "3. On the flashing page: Connect, address 0x0,",
    "   choose the .bin, Program",
    "4. It restarts by itself when done",
  };
  for (int i = 0; i < 8; i++) {
    g.setTextColor(i < 2 ? C_DIM : C_TEXT);
    g.drawString(lines[i], 14, 40 + i * 20);
  }
  char buf[32];
  snprintf(buf, sizeof buf, "Installed: v%s", FW_VERSION);
  g.setTextColor(C_ACCENT);
  g.drawString(buf, 14, 40 + 8 * 20 + 4);
  drawFooterHint(g, "B4: go to flash mode   HOME: back");
}

void drawMessage(LGFX_Sprite &g) {
  g.fillScreen(C_BG);
  g.setFont(&fonts::FreeSansBold12pt7b);
  g.setTextColor(C_TEXT);
  g.setTextDatum(middle_center);
  g.drawString(messageText, 160, 120);
}

void drawWifi(LGFX_Sprite &g);   // Wi-Fi update screen (further down)
void drawOnline(LGFX_Sprite &g); // Check for update screen (further down)

void render() {
  // Draw into the off-screen canvas, then push in one go (no flicker)
  LGFX_Sprite &g = canvas;
  g.fillScreen(C_BG);
  switch (screen) {
    case S_STATUS: drawStatus(g); break;
    case S_MENU: drawMenu(g); break;
    case S_REMAP: drawRemap(g); break;
    case S_TURBO: drawTurbo(g); break;
    case S_TEST: drawTest(g); break;
    case S_UPDATE: drawUpdate(g); break;
    case S_WIFI: drawWifi(g); break;
    case S_ONLINE: drawOnline(g); break;
    case S_MESSAGE: drawMessage(g); break;
  }
  g.pushSprite(0, 0);
}

void showMessage(const char *msg, uint32_t ms) {
  messageText = msg;
  messageUntil = millis() + ms;
  screen = S_MESSAGE;
  render();
}


// ======================= WI-FI UPDATE ===============================
// The stick makes its own Wi-Fi network. Join it from a phone or computer,
// open http://192.168.4.1 and upload new firmware (the *_wifi.bin file), or
// download / upload settings to copy them between sticks.
#define AP_PASS        "arcadestick"
#define SETTINGS_MAGIC 0x4B545341u            // "ASTK"
struct __attribute__((packed)) SettingsFile { uint32_t magic; uint16_t size; Settings s; };
WebServer *web = nullptr;
volatile uint8_t wifiState = 0;    // 0 off, 1 waiting, 2 receiving, 3 installed, 4 failed
volatile uint32_t otaBytes = 0;
const char *wifiError = "";
uint32_t restartAt = 0;
String scanList;

// Home Wi-Fi details, kept on the stick only (not in the settings file)
String homeSsid() { Preferences n; n.begin("net", true); String s = n.getString("ssid", ""); n.end(); return s; }
String homePass() { Preferences n; n.begin("net", true); String s = n.getString("pass", ""); n.end(); return s; }
void homeWifiSave(const String &ss, const String &pw) {
  Preferences n; n.begin("net", false); n.putString("ssid", ss); n.putString("pass", pw); n.end();
}
String htmlEsc(const String &in) {
  String o; o.reserve(in.length() + 8);
  for (char c : in) {
    if (c == '&') o += "&amp;"; else if (c == '<') o += "&lt;"; else if (c == '>') o += "&gt;";
    else if (c == '"') o += "&quot;"; else o += c;
  }
  return o;
}
String apName() { return String("ArcadeStick-") + STICK_COLOUR_NAME; }

// The other stick's firmware has the other pin map: refuse it.
// Each build carries its own ID tag; an upload must contain this stick's tag.
#ifdef STICK_BLUE
__attribute__((used)) static const char STICK_ID[] = "ASID:BLUE";
#else
__attribute__((used)) static const char STICK_ID[] = "ASID:RED!";
#endif
uint8_t markPos = 0; bool markSeen = false;
void scanForMark(const uint8_t *b, size_t n) {
  for (size_t i = 0; i < n && !markSeen; i++) {
    if (b[i] == (uint8_t)STICK_ID[markPos]) { if (++markPos == sizeof(STICK_ID) - 1) markSeen = true; }
    else markPos = (b[i] == (uint8_t)STICK_ID[0]) ? 1 : 0;
  }
}

static const char WEB_PAGE[] PROGMEM = R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>%NAME%</title>
<style>body{font-family:system-ui,sans-serif;background:#10141c;color:#e8ecf1;margin:0;padding:20px;max-width:560px}
h1{font-size:22px;margin:0 0 4px}p{color:#9aa4b2;margin:4px 0 14px}section{background:#1a2030;border-radius:10px;padding:16px;margin:14px 0}
h2{font-size:16px;margin:0 0 10px;color:%HEX%}button,a.b{background:%HEX%;color:#10141c;border:0;border-radius:8px;padding:10px 14px;font-weight:600;text-decoration:none;display:inline-block;margin-top:8px}
input{color:#e8ecf1;margin:6px 0}input[type=text],input[type=password]{background:#10141c;border:1px solid #3a4456;border-radius:6px;padding:9px;width:100%;box-sizing:border-box;font-size:15px}#s{color:%HEX%;font-weight:600}</style></head><body>
<h1>%NAME%</h1><p>Firmware v%VER% &middot; %COLOUR% stick</p>
<section><h2>Update firmware</h2><p>Choose the <b>%COLOURL%</b> stick's <b>_wifi.bin</b> file.</p>
<form id="f" method="POST" action="/update" enctype="multipart/form-data"><input type="file" name="fw" accept=".bin"><br>
<button>Install</button></form><p id="s"></p></section>
<section><h2>Home Wi-Fi (for Check for update)</h2><p>%HWSTATUS%</p>
<form method="POST" action="/homewifi"><input type="text" name="ssid" list="nets" placeholder="Wi-Fi name" value="%HWSSID%" autocomplete="off">
<datalist id="nets">%NETS%</datalist><input type="password" name="pass" placeholder="Wi-Fi password"><br>
<button>Save home Wi-Fi</button></form></section>
<section><h2>Copy settings to the other stick</h2><p>Profiles, remaps, turbo, sound and screen settings.</p>
<a class="b" href="/settings.bin">Download settings</a>
<form method="POST" action="/settings" enctype="multipart/form-data"><input type="file" name="cfg" accept=".bin"><br>
<button>Load settings file</button></form></section>
<script>
document.getElementById('f').onsubmit=function(e){var i=this.fw.files[0];if(!i){e.preventDefault();return}
var n=i.name.toLowerCase();if(n.indexOf('%COLOURL%')<0||n.indexOf('wifi')<0){if(!confirm(i.name+' does not look like the %COLOURL% stick Wi-Fi file. Install anyway?')){e.preventDefault();return}}
document.getElementById('s').textContent='Uploading - keep this page open...'};
</script></body></html>)HTML";

void sendMsg(const char *title, const char *msg) {
  String p = String("<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
                    "<body style='font-family:system-ui;background:#10141c;color:#e8ecf1;padding:24px'><h2>") +
             title + "</h2><p>" + msg + "</p><p><a style='color:#9cf' href='/'>Back</a></p>";
  web->send(200, "text/html", p);
}

void wifiStart() {
  if (bootConn == C_BLE) { radioOff = true; delay(20); NimBLEDevice::deinit(true); }   // Wi-Fi only; restarts after
  showMessage("Starting Wi-Fi...", 0);
  WiFi.mode(WIFI_AP_STA);
  scanList = "";                                    // nearby networks, for the Home Wi-Fi picker
  int found = WiFi.scanNetworks();
  for (int i = 0, added = 0; i < found && added < 20; i++) {
    String n = WiFi.SSID(i);
    if (!n.length() || scanList.indexOf("\"" + htmlEsc(n) + "\"") >= 0) continue;
    scanList += "<option value=\"" + htmlEsc(n) + "\">"; added++;
  }
  WiFi.scanDelete();
  WiFi.softAP(apName().c_str(), AP_PASS);
  screen = S_WIFI;
  web = new WebServer(80);
  web->on("/homewifi", HTTP_POST, [] {
    String ss = web->arg("ssid"), pw = web->arg("pass");
    ss.trim();
    if (!ss.length()) { homeWifiSave("", ""); sendMsg("Home Wi-Fi", "Cleared."); return; }
    if (!pw.length() && ss == homeSsid()) pw = homePass();       // re-saving the same network keeps its password
    homeWifiSave(ss, pw);
    sendMsg("Home Wi-Fi saved", ("Saved <b>" + htmlEsc(ss) + "</b>. Leave this screen on the stick, then use <b>Menu &rarr; Check for update</b>.").c_str());
  });
  web->on("/", HTTP_GET, [] {
    String p = FPSTR(WEB_PAGE);
    char hex[8]; uint16_t c = C_ACCENT;
    snprintf(hex, sizeof hex, "#%02x%02x%02x", (c >> 8) & 0xF8, (c >> 3) & 0xFC, (c << 3) & 0xF8);
    String col = STICK_COLOUR_NAME, coll = col; coll.toLowerCase();
    p.replace("%NAME%", DEVICE_NAME); p.replace("%VER%", FW_VERSION);
    p.replace("%COLOURL%", coll); p.replace("%COLOUR%", col); p.replace("%HEX%", hex);
    String ss = homeSsid();
    p.replace("%HWSTATUS%", ss.length() ? "Saved: <b>" + htmlEsc(ss) + "</b>. The stick uses it only to check GitHub for updates."
                                        : String("Not set. Pick your home network and enter its password so the stick can check GitHub for updates."));
    p.replace("%HWSSID%", htmlEsc(ss)); p.replace("%NETS%", scanList);
    web->send(200, "text/html", p);
  });
  web->on("/update", HTTP_POST, [] {
    if (wifiState == 3) { sendMsg("Installed", "The stick is restarting with the new firmware."); restartAt = millis() + 1500; }
    else sendMsg("Not installed", wifiError);
  }, [] {
    HTTPUpload &up = web->upload();
    if (up.status == UPLOAD_FILE_START) {
      otaBytes = 0; markPos = 0; markSeen = false; wifiError = ""; wifiState = 2;
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { wifiState = 4; wifiError = "Could not start the update."; }
      render();
    } else if (up.status == UPLOAD_FILE_WRITE && wifiState == 2) {
      if (otaBytes == 0) {
        // an app image has the app descriptor magic 0xABCD5432 at byte 32;
        // the merged USB .bin (bootloader first) doesn't
        if (up.currentSize < 36 || up.buf[0] != 0xE9 ||
            !(up.buf[32] == 0x32 && up.buf[33] == 0x54 && up.buf[34] == 0xCD && up.buf[35] == 0xAB)) {
          Update.abort(); wifiState = 4;
          wifiError = "That is not a Wi-Fi firmware file. Use the file ending in _wifi.bin (not the USB one).";
          return;
        }
      }
      scanForMark(up.buf, up.currentSize);
      if (Update.write(up.buf, up.currentSize) != up.currentSize) { Update.abort(); wifiState = 4; wifiError = "Write failed."; return; }
      uint32_t before = otaBytes;
      otaBytes += up.currentSize;
      if ((otaBytes >> 16) != (before >> 16)) render();      // progress every 64 KB
    } else if (up.status == UPLOAD_FILE_END && wifiState == 2) {
      bool rightStick = markSeen;
      if (!rightStick) { Update.abort(); wifiState = 4; wifiError = "That firmware is for the other stick (its buttons are wired differently)."; }
      else if (Update.end(true)) wifiState = 3;
      else { wifiState = 4; wifiError = "The file was incomplete or damaged."; }
      render();
    } else if (up.status == UPLOAD_FILE_ABORTED) {
      Update.abort(); wifiState = 4; wifiError = "Upload interrupted.";
    }
  });
  web->on("/settings.bin", HTTP_GET, [] {
    SettingsFile f; f.magic = SETTINGS_MAGIC; f.size = sizeof(Settings); f.s = cfg;
    web->sendHeader("Content-Disposition", "attachment; filename=\"arcade-stick-settings.bin\"");
    web->setContentLength(sizeof f);
    web->send(200, "application/octet-stream", "");
    web->sendContent((const char *)&f, sizeof f);
  });
  static uint8_t sbuf[sizeof(SettingsFile)]; static size_t slen = 0; static const char *smsg = "";
  web->on("/settings", HTTP_POST, [] { sendMsg("Settings", smsg); }, [] {
    HTTPUpload &up = web->upload();
    if (up.status == UPLOAD_FILE_START) slen = 0;
    else if (up.status == UPLOAD_FILE_WRITE) {
      size_t n = min(up.currentSize, sizeof(sbuf) - slen);
      memcpy(sbuf + slen, up.buf, n); slen += n;
    } else if (up.status == UPLOAD_FILE_END) {
      SettingsFile *f = (SettingsFile *)sbuf;
      if (slen < 6 || f->magic != SETTINGS_MAGIC || f->size == 0 || f->size > sizeof(Settings)) {
        smsg = "That isn't an Arcade Stick settings file.";
      } else {
        Settings in = cfg;                               // fields missing from older files keep their value
        memcpy(&in, &f->s, f->size);
        in.player = cfg.player; in.theme = cfg.theme;    // keep this stick's own player and colour
        in.conn = cfg.conn;
        cfg = in;
        saveSettings(); applyTheme(); applySoundSettings();
        smsg = "Loaded and saved. This stick keeps its own player number and colour.";
      }
    }
  });
  web->begin();
  wifiState = 1;
}

void drawWifi(LGFX_Sprite &g) {
  drawHeader(g, "WI-FI UPDATE");
  g.setFont(&fonts::Font2);
  g.setTextDatum(top_left);
  if (wifiState == 0) {
    const char *lines[] = {
      "Update firmware, copy settings, or save your",
      "home Wi-Fi (for Check for update) from a",
      "phone. Bluetooth is off until you leave.",
      "",
      "Press B4 to start the stick's Wi-Fi network.",
    };
    for (int i = 0; i < 5; i++) { g.setTextColor(i < 3 ? C_DIM : C_TEXT); g.drawString(lines[i], 14, 44 + i * 20); }
    drawFooterHint(g, "B4: start Wi-Fi   HOME: back");
    return;
  }
  char buf[64];
  g.setTextColor(C_DIM);  g.drawString("1. Join this Wi-Fi network:", 14, 38);
  g.setTextColor(C_TEXT); g.setFont(&fonts::FreeSansBold9pt7b);
  g.drawString(apName(), 30, 56);
  g.setFont(&fonts::Font2); g.setTextColor(C_DIM);
  snprintf(buf, sizeof buf, "Password: %s", AP_PASS); g.drawString(buf, 30, 78);
  g.drawString("2. Open in a browser:", 14, 102);
  g.setTextColor(C_TEXT); g.setFont(&fonts::FreeSansBold9pt7b);
  g.drawString("http://192.168.4.1", 30, 120);
  g.setFont(&fonts::FreeSansBold9pt7b);
  uint16_t col = C_ACCENT; const char *st = "Waiting...";
  if (wifiState == 2) { snprintf(buf, sizeof buf, "Receiving %lu KB", (unsigned long)(otaBytes / 1024)); st = buf; }
  if (wifiState == 3) { st = "Installed - restarting"; col = C_OK; }
  if (wifiState == 4) { st = "Not installed"; col = C_WARN; }
  g.setTextColor(col);
  g.drawString(st, 14, 152);
  if (wifiState == 4) { g.setFont(&fonts::Font0); g.setTextColor(C_DIM); g.drawString(wifiError, 14, 176); }
  drawFooterHint(g, "HOME: leave (the stick restarts)");
}

// ======================= CHECK FOR UPDATE (GitHub) ==================
// Joins the saved home Wi-Fi, asks GitHub for the latest release of
// UPDATE_REPO, and if it is newer offers to install this stick's
// "_red_wifi.bin" / "_blue_wifi.bin" file from it. The download gets the
// same checks as a Wi-Fi upload (app image, right stick) plus the image's
// own checksum. HTTPS is verified against the built-in root certificates.
extern const uint8_t crt_bundle_start[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t crt_bundle_end[]   asm("_binary_x509_crt_bundle_end");

enum OnlineState : uint8_t { OL_IDLE, OL_JOINING, OL_CHECKING, OL_UPTODATE, OL_AVAILABLE, OL_DOWNLOADING, OL_INSTALLED, OL_FAILED };
volatile uint8_t olState = OL_IDLE;
volatile bool olInstall = false;
volatile uint32_t olBytes = 0, olTotal = 0;
String olLatest, olUrl, olError;

// "v2.10" / "2.3.1" -> comparable number
uint32_t verNum(const char *v) {
  while (*v && (*v < '0' || *v > '9')) v++;
  uint32_t part[3] = {0, 0, 0};
  for (int i = 0; i < 3 && *v; i++) {
    part[i] = strtoul(v, (char **)&v, 10);
    if (*v == '.') v++; else break;
  }
  return part[0] * 1000000 + part[1] * 1000 + part[2];
}

// value of "key": "string" in a JSON text, starting at from; returns end position or -1
int jsonStr(const String &js, const char *key, int from, String &out) {
  String k = String("\"") + key + "\"";
  int i = js.indexOf(k, from);
  if (i < 0) return -1;
  i += k.length();
  while (i < (int)js.length() && (js[i] == ' ' || js[i] == ':' || js[i] == '\n' || js[i] == '\r' || js[i] == '\t')) i++;
  if (i >= (int)js.length() || js[i] != '"') return i;
  int j = js.indexOf('"', i + 1);
  if (j < 0) return -1;
  out = js.substring(i + 1, j);
  return j + 1;
}

void olFail(const char *msg) { olError = msg; olState = OL_FAILED; uiDirty = true; }

bool olGet(HTTPClient &http, NetworkClientSecure &tls, const String &url) {
  tls.setCACertBundle(crt_bundle_start, crt_bundle_end - crt_bundle_start);
  http.setConnectTimeout(15000);
  http.setTimeout(20000);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.setUserAgent("ArcadeStick/" FW_VERSION);
  return http.begin(tls, url);
}

void onlineTask(void *) {
  String ss = homeSsid(), pw = homePass();
  olState = OL_JOINING; uiDirty = true;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ss.c_str(), pw.c_str());
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) delay(200);
  if (WiFi.status() != WL_CONNECTED) {
    olFail("Couldn't join your home Wi-Fi.\nCheck its name and password on the\nWi-Fi update page.");
    vTaskDelete(nullptr);
  }

  // 1. what's the latest release?
  olState = OL_CHECKING; uiDirty = true;
  String body; int code;
  {
    NetworkClientSecure tls; HTTPClient http;
    if (!olGet(http, tls, "https://api.github.com/repos/" UPDATE_REPO "/releases/latest")) { olFail("Couldn't reach GitHub."); vTaskDelete(nullptr); }
    http.addHeader("Accept", "application/vnd.github+json");
    code = http.GET();
    if (code == 200) body = http.getString();
    http.end();
  }
  if (code == 404) { olFail("No releases on GitHub yet."); vTaskDelete(nullptr); }
  if (code != 200) {
    static char m[64]; snprintf(m, sizeof m, code < 0 ? "Couldn't reach GitHub (%d).\nIs the internet working?" : "GitHub said error %d.\nTry again later.", code);
    olFail(m); vTaskDelete(nullptr);
  }
  String tag;
  if (jsonStr(body, "tag_name", 0, tag) < 0 || !tag.length()) { olFail("Couldn't read the release info."); vTaskDelete(nullptr); }
  String want = String("_") + STICK_COLOUR_NAME + "_wifi.bin"; want.toLowerCase();
  olUrl = "";
  for (int pos = 0; pos >= 0; ) {
    String u; pos = jsonStr(body, "browser_download_url", pos, u);
    if (pos < 0) break;
    String lu = u; lu.toLowerCase();
    if (lu.endsWith(want)) { olUrl = u; break; }
  }
  body = String();                                   // free it before downloading
  olLatest = tag;
  if (verNum(tag.c_str()) <= verNum(FW_VERSION)) { olState = OL_UPTODATE; uiDirty = true; vTaskDelete(nullptr); }
  if (!olUrl.length()) { olFail("That release has no file for\nthis stick yet."); vTaskDelete(nullptr); }
  olState = OL_AVAILABLE; uiDirty = true;

  // 2. wait for B4 (HOME restarts the stick instead)
  while (!olInstall) delay(50);

  // 3. download straight into the spare app slot
  olState = OL_DOWNLOADING; olBytes = 0; olTotal = 0; uiDirty = true;
  {
    NetworkClientSecure tls; HTTPClient http;
    http.useHTTP10(true);                            // plain (not chunked) stream
    if (!olGet(http, tls, olUrl)) { olFail("Couldn't start the download."); vTaskDelete(nullptr); }
    code = http.GET();
    int len = http.getSize();
    if (code != 200 || len < 4096) {
      http.end();
      static char m[64]; snprintf(m, sizeof m, "Download failed (%d).", code);
      olFail(m); vTaskDelete(nullptr);
    }
    olTotal = len;
    if (!Update.begin(len)) { http.end(); olFail("Not enough room for the update."); vTaskDelete(nullptr); }
    markPos = 0; markSeen = false;
    NetworkClient *st = http.getStreamPtr();
    static uint8_t buf[4096];
    uint32_t lastData = millis();
    const char *err = nullptr;
    while (olBytes < (uint32_t)len) {
      size_t avail = st->available();
      if (!avail) {
        if (!http.connected() || millis() - lastData > 20000) { err = "The download stopped.\nTry again."; break; }
        delay(5); continue;
      }
      int n = st->readBytes(buf, min(avail, sizeof buf));
      if (n <= 0) continue;
      lastData = millis();
      if (olBytes == 0 && (n < 36 || buf[0] != 0xE9 || !(buf[32] == 0x32 && buf[33] == 0x54 && buf[34] == 0xCD && buf[35] == 0xAB))) {
        err = "The GitHub file isn't a\nWi-Fi firmware file."; break;
      }
      scanForMark(buf, n);
      if (Update.write(buf, n) != (size_t)n) { err = "Write failed."; break; }
      uint32_t before = olBytes;
      olBytes += n;
      if ((olBytes >> 15) != (before >> 15)) uiDirty = true;
    }
    http.end();
    if (!err && !markSeen) err = "That file is for the other stick.";
    if (err) { Update.abort(); olFail(err); vTaskDelete(nullptr); }
    if (!Update.end(true)) { olFail("The download was damaged.\nTry again."); vTaskDelete(nullptr); }
  }
  olState = OL_INSTALLED; uiDirty = true;
  restartAt = millis() + 2500;
  vTaskDelete(nullptr);
}

void onlineStart() {
  if (bootConn == C_BLE) { radioOff = true; delay(20); NimBLEDevice::deinit(true); }   // Wi-Fi only; restarts after
  olInstall = false; olError = "";
  olState = OL_JOINING;
  xTaskCreate(onlineTask, "online", 16384, nullptr, 1, nullptr);
}

void drawLines(LGFX_Sprite &g, const char *text, int x, int y, int step) {
  char line[64]; int n = 0;
  for (const char *c = text; ; c++) {
    if (*c == '\n' || *c == 0 || n == 63) { line[n] = 0; g.drawString(line, x, y); y += step; n = 0; if (!*c) break; if (*c != '\n') line[n++] = *c; }
    else line[n++] = *c;
  }
}

void drawOnline(LGFX_Sprite &g) {
  drawHeader(g, "CHECK FOR UPDATE");
  g.setTextDatum(top_left);
  g.setFont(&fonts::Font2);
  char buf[80];
  String ss = olState == OL_IDLE ? homeSsid() : String();
  switch (olState) {
    case OL_IDLE:
      g.setTextColor(C_DIM);
      drawLines(g, "Looks for new firmware on GitHub using your\nhome Wi-Fi. Bluetooth turns off while it\nchecks, and the stick restarts after.", 14, 40, 20);
      snprintf(buf, sizeof buf, "Installed: v%s", FW_VERSION);
      g.setTextColor(C_TEXT); g.drawString(buf, 14, 112);
      if (ss.length()) {
        g.drawString("Home Wi-Fi: " + ss, 14, 134);
        drawFooterHint(g, "B4: check now   HOME: back");
      } else {
        g.setTextColor(C_WARN);
        drawLines(g, "No home Wi-Fi saved yet. Open Wi-Fi update,\njoin it from your phone and fill in\nHome Wi-Fi on the page.", 14, 134, 20);
        drawFooterHint(g, "HOME: back");
      }
      return;
    case OL_JOINING:
    case OL_CHECKING:
      g.setFont(&fonts::FreeSansBold12pt7b); g.setTextColor(C_ACCENT);
      g.drawString(olState == OL_JOINING ? "Joining Wi-Fi..." : "Asking GitHub...", 14, 80);
      drawFooterHint(g, "Please wait");
      return;
    case OL_UPTODATE:
      g.setFont(&fonts::FreeSansBold12pt7b); g.setTextColor(C_OK);
      g.drawString("Up to date", 14, 70);
      g.setFont(&fonts::Font2); g.setTextColor(C_TEXT);
      snprintf(buf, sizeof buf, "Installed v%s, latest on GitHub %s", FW_VERSION, olLatest.c_str());
      g.drawString(buf, 14, 110);
      drawFooterHint(g, "HOME: leave (the stick restarts)");
      return;
    case OL_AVAILABLE:
      g.setFont(&fonts::FreeSansBold12pt7b); g.setTextColor(C_ACCENT);
      snprintf(buf, sizeof buf, "%s available", olLatest.c_str());
      g.drawString(buf, 14, 64);
      g.setFont(&fonts::Font2); g.setTextColor(C_TEXT);
      snprintf(buf, sizeof buf, "Installed: v%s", FW_VERSION);
      g.drawString(buf, 14, 104);
      g.setTextColor(C_DIM);
      drawLines(g, "Your settings are kept. Keep the stick\npowered until it restarts.", 14, 132, 20);
      drawFooterHint(g, "B4: install   HOME: cancel");
      return;
    case OL_DOWNLOADING: {
      g.setFont(&fonts::FreeSansBold12pt7b); g.setTextColor(C_ACCENT);
      int pct = olTotal ? (int)((uint64_t)olBytes * 100 / olTotal) : 0;
      snprintf(buf, sizeof buf, "Downloading %d%%", pct);
      g.drawString(buf, 14, 70);
      g.drawRect(14, 110, 292, 18, C_LINE);
      g.fillRect(16, 112, 288 * pct / 100, 14, C_ACCENT);
      drawFooterHint(g, "Keep the stick powered on");
      return;
    }
    case OL_INSTALLED:
      g.setFont(&fonts::FreeSansBold12pt7b); g.setTextColor(C_OK);
      g.drawString("Installed", 14, 70);
      g.setFont(&fonts::Font2); g.setTextColor(C_TEXT);
      snprintf(buf, sizeof buf, "Restarting with %s...", olLatest.c_str());
      g.drawString(buf, 14, 110);
      drawFooterHint(g, "");
      return;
    case OL_FAILED:
      g.setFont(&fonts::FreeSansBold12pt7b); g.setTextColor(C_WARN);
      g.drawString("No update", 14, 56);
      g.setFont(&fonts::Font2); g.setTextColor(C_TEXT);
      drawLines(g, olError.c_str(), 14, 96, 20);
      drawFooterHint(g, "HOME: leave (the stick restarts)");
      return;
  }
}

void openMenu() {
  menuOpen = true;
  forceSend = true;
  takeEdges();               // drop the HOME press that opened the menu
  screen = S_MENU;
  menuIndex = 0;
  playSound(SND_MENU_OPEN);
}

void closeMenu() {
  playSound(SND_SAVED);
  if (cfg.conn != bootConn) {
    saveSettings();
    showMessage("Restarting...", 0);
    delay(800);
    ESP.restart();
  }
  if (settingsDirty) { saveSettings(); settingsDirty = false; }
  showMessage("Saved", 0);
  delay(400);
  screen = S_STATUS;
  takeEdges();
  menuOpen = false;
  forceSend = true;
}

void cycle(uint8_t &v, int dir, int count) { v = (v + dir + count) % count; settingsDirty = true; }
// new game or fighter: clear counts and show the Moves page when the menu closes
void resetMoves() { memset(moveCount, 0, sizeof moveCount); moveHit = -1; motion.clear(); cfg.page = PG_MOVES; }

void menuChange(int dir) {
  switch (menuIndex) {
    case M_PROFILE: {
      int pos = 0;
      while (pos < PROFILE_COUNT && PROFILE_ORDER[pos] != cfg.profile) pos++;
      cfg.profile = PROFILE_ORDER[(pos + dir + PROFILE_COUNT) % PROFILE_COUNT];
      settingsDirty = true; break;
    }
    case M_STICK:   cycle(cfg.stickMode, dir, SM_COUNT); break;
    case M_CONN:    cycle(cfg.conn, dir, 2); break;
    case M_BRIGHT:  cfg.brightness = constrain(cfg.brightness + dir * 10, 10, 100);
                    settingsDirty = true; applyDisplaySettings(); break;
    case M_FLIP:    cycle(cfg.flip, dir, 2); applyDisplaySettings(); break;
    case M_SOUND:   cfg.sound = constrain(cfg.sound + dir, 0, 3); settingsDirty = true;
                    applySoundSettings(); break;
    case M_CLICKS:  cycle(cfg.clicks, dir, 2); break;
    case M_SLEEP:   cycle(cfg.screenSleep, dir, SLEEP_COUNT); break;
    case M_AUTOOFF: cycle(cfg.autoOff, dir, AUTOOFF_COUNT); break;
    case M_THEME:   cycle(cfg.theme, dir, THEME_COUNT); applyTheme(); break;
    case M_PLAYER:  cycle(cfg.player, dir, 4); break;
    case M_SOCD:    cycle(cfg.socd, dir, SOCD_COUNT); break;
    case M_GAME:    cycle(cfg.game, dir, GAME_COUNT); cfg.fighter = 0; resetMoves(); break;
    case M_FIGHTER: cycle(cfg.fighter, dir, GAMES[cfg.game].n); resetMoves(); break;
    case M_MOVECHECK: cfg.moveCheck = constrain(cfg.moveCheck + dir, 0, 2); settingsDirty = true; break;
    default: return;
  }
  playSound(SND_CHANGE);                   // also previews the new sound level
}

void menuSelect() {
  switch (menuIndex) {
    case M_REMAP:
      // start Custom from whatever profile is active
      if (cfg.profile != PROFILE_CUSTOM) memcpy(cfg.custom, activeMap(), ACT_COUNT);
      cfg.profile = PROFILE_CUSTOM; settingsDirty = true;
      remapSel = 0; screen = S_REMAP; playSound(SND_SELECT); break;
    case M_TURBO: screen = S_TURBO; playSound(SND_SELECT); break;
    case M_TEST: screen = S_TEST; playSound(SND_SELECT); break;
    case M_UPDATE: screen = S_UPDATE; playSound(SND_SELECT); break;
    case M_WIFI: screen = S_WIFI; playSound(SND_SELECT); break;
    case M_ONLINE: screen = S_ONLINE; playSound(SND_SELECT); break;
    case M_FORGET:
      if (bootConn == C_BLE) {
        NimBLEDevice::deleteAllBonds();
        playSound(SND_SELECT);
        showMessage("Pairings cleared", 1200);
      }
      break;
    case M_EXIT: closeMenu(); break;
    default: menuChange(+1); break;
  }
}

// Up/Down auto-repeat while held
bool repeatPress(uint16_t edges, uint16_t s, uint8_t p) {
  static uint32_t nextAt[PHYS_COUNT] = {0};
  uint32_t now = millis();
  if (edges & (1u << p)) { nextAt[p] = now + 380; return true; }
  if (held(s, p) && (int32_t)(now - nextAt[p]) >= 0) { nextAt[p] = now + 110; return true; }
  return false;
}

// Turn everything off and nap until START or HOME is held.
// Restart straight into the ESP32-S3's built-in flash mode (same as holding
// BOOT and tapping RESET). The USB port comes up as the flashing port.
void enterUpdateMode() {
  messageText = "Flash mode - screen off";
  messageUntil = 0;
  screen = S_MESSAGE;
  render();
  menuOpen = true; forceSend = true;      // release every button on the host
  vTaskDelay(pdMS_TO_TICKS(50));
  shuttingDown = true;                    // input loop stops
  vTaskDelay(pdMS_TO_TICKS(20));
  if (settingsDirty) { saveSettings(); settingsDirty = false; }
  soundShutdown(400);
  if (bootConn == C_BLE) NimBLEDevice::deinit(true);
  rgbLedWrite(STICK_LED_PIN, 0, 0, 0);
  delay(600);
  tft.setBrightness(0);
  usb_persist_restart(RESTART_BOOTLOADER);
}

void powerOff() {
  messageText = "Release START";
  messageUntil = 0;
  screen = S_MESSAGE;
  render();
  while (held(physState, P_START)) vTaskDelay(pdMS_TO_TICKS(10));
  messageText = "Powering off";
  render();
  playSound(SND_OFF);

  menuOpen = true; forceSend = true;      // host sees all buttons released
  vTaskDelay(pdMS_TO_TICKS(50));
  shuttingDown = true;                    // stop the input loop; this task owns I2C now
  vTaskDelay(pdMS_TO_TICKS(20));
  if (settingsDirty) { saveSettings(); settingsDirty = false; }
  soundShutdown(1500);                    // let the power-off tune finish, then amp off

  if (bootConn == C_BLE) NimBLEDevice::deinit(true);   // radio off
  rgbLedWrite(STICK_LED_PIN, 0, 0, 0);
  tft.setBrightness(0);
  tft.sleep();                                         // LCD controller to sleep
  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, LOW);                       // backlight hard off
  debugSerial.println("Powered off (light sleep)");
  delay(20);

  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_POLL_MS * 1000ULL);
  const uint16_t wakeMask = (1u << MCP_BIT[P_START]) | (1u << MCP_BIT[P_HOME]);
  uint8_t hits = 0;
  for (;;) {
    if (esp_light_sleep_start() != ESP_OK) vTaskDelay(pdMS_TO_TICKS(SLEEP_POLL_MS));  // sleep refused: just wait
    uint16_t raw;
    bool pressed = mcpRead(raw) && (raw & wakeMask);
    hits = pressed ? hits + 1 : 0;
    if (hits >= 2) ESP.restart();                      // held ~0.5 s: power on
  }
}

void uiStep() {
  if (powerOffRequest) { powerOffRequest = false; powerOff(); }
  if (menuRequest) { menuRequest = false; openMenu(); uiDirty = true; }
  if (screen == S_MESSAGE) {
    if (messageUntil && (int32_t)(millis() - messageUntil) >= 0) { screen = S_MENU; uiDirty = true; }
    return;
  }
  if (screen == S_STATUS) {
    uint16_t e = takeEdges();
    if (e && cfg.clicks && !selLayer) playSound(SND_CLICK);
    if (selLayer) uiDirty = true;                                   // shortcut help overlay
    return;
  }

  uint16_t e = takeEdges();
  uint16_t s = physState;
  bool up = repeatPress(e, s, P_UP), down = repeatPress(e, s, P_DOWN);
  bool left = e & (1u << P_LEFT), right = e & (1u << P_RIGHT);
  bool home = e & (1u << P_HOME);
  bool any = e || up || down;

  switch (screen) {
    case S_MENU:
      if (up)   { menuIndex = (menuIndex + M_COUNT - 1) % M_COUNT; playSound(SND_MOVE); }
      if (down) { menuIndex = (menuIndex + 1) % M_COUNT; playSound(SND_MOVE); }
      if (left) menuChange(-1);
      if (right) { if (menuIndex == M_REMAP || menuIndex == M_TURBO || menuIndex == M_TEST || menuIndex == M_ONLINE || menuIndex == M_UPDATE || menuIndex == M_WIFI) menuSelect(); else menuChange(+1); }
      if (e & (1u << P_B4)) menuSelect();
      if (e & (1u << P_START)) { menuIndex = M_EXIT; menuSelect(); }
      if (home || (e & (1u << P_B5))) closeMenu();
      break;
    case S_WIFI:
      if (wifiState == 0 && (e & (1u << P_B4))) { playSound(SND_SELECT); wifiStart(); }
      if (home) {
        if (wifiState == 0) { screen = S_MENU; playSound(SND_BACK); }
        else { if (settingsDirty) saveSettings(); showMessage("Restarting...", 0); delay(500); ESP.restart(); }
      }
      any = true;
      break;
    case S_ONLINE:
      if (olState == OL_IDLE) {
        if ((e & (1u << P_B4)) && homeSsid().length()) { playSound(SND_SELECT); onlineStart(); }
        if (home) { screen = S_MENU; playSound(SND_BACK); }
      } else if (olState == OL_AVAILABLE && (e & (1u << P_B4))) { playSound(SND_SELECT); olInstall = true; }
      else if (home && olState != OL_DOWNLOADING && olState != OL_INSTALLED) {
        if (settingsDirty) saveSettings();
        showMessage("Restarting...", 0); delay(500); ESP.restart();
      }
      any = true;
      break;
    case S_UPDATE:
      if (e & (1u << P_B4)) enterUpdateMode();
      if (home) { screen = S_MENU; playSound(SND_BACK); }
      break;
    case S_REMAP:
      for (uint8_t i = 0; i < ACT_COUNT; i++) if (e & (1u << (ACT_FIRST + i))) { remapSel = i; playSound(SND_MOVE); }
      if (up)   { cycle(cfg.custom[remapSel], +1, OUT_COUNT); playSound(SND_CHANGE); }
      if (down) { cycle(cfg.custom[remapSel], -1, OUT_COUNT); playSound(SND_CHANGE); }
      if (home) { screen = S_MENU; playSound(SND_BACK); }
      break;
    case S_TURBO:
      for (uint8_t i = 0; i < ACT_COUNT; i++)
        if (e & (1u << (ACT_FIRST + i))) { cfg.turboMask ^= (1 << i); settingsDirty = true;
                                           playSound(cfg.turboMask & (1 << i) ? SND_SELECT : SND_BACK); }
      if (left)  { cycle(cfg.turboHzIdx, -1, TURBO_HZ_COUNT); playSound(SND_CHANGE); }
      if (right) { cycle(cfg.turboHzIdx, +1, TURBO_HZ_COUNT); playSound(SND_CHANGE); }
      if (home)  { screen = S_MENU; playSound(SND_BACK); }
      break;
    case S_TEST: {
      // every button is live here, so leave with a 2 s HOME hold
      static uint32_t homeSince = 0;
      if (e) playSound(SND_MOVE);            // tick for every press while testing
      if (held(s, P_HOME)) { if (!homeSince) homeSince = millis(); else if (millis() - homeSince > 2000) { homeSince = 0; screen = S_MENU; takeEdges(); playSound(SND_BACK); } }
      else homeSince = 0;
      any = true;
      break;
    }
    default: break;
  }
  if (any) uiDirty = true;
}

// UI runs on core 0 so screen updates never delay button reports (core 1)
// Sounds for things that happen on their own: connect / disconnect,
// expander lost, low battery, and the power-off countdown.
void soundEvents() {
  uint32_t now = millis();
  static bool lastConn = false, lastMcp = true;
  static uint32_t errAt = 0;
  bool c = hostConnected();
  if (c != lastConn) { lastConn = c; playSound(c ? SND_CONNECT : SND_DISCONNECT); }
  bool m = mcpOk;
  if (m != lastMcp) { lastMcp = m; if (!m && now - errAt > 5000) { errAt = now; playSound(SND_ERROR); } }

  static int8_t warned = 100;            // lowest threshold already warned about
  int pct = batteryPercent();
  if (pct >= 0 && now > 10000) {          // give the reading time to settle
    if (pct <= 5 && warned > 5)        { warned = 5;  playSound(SND_LOWBAT); }
    else if (pct <= 15 && warned > 15) { warned = 15; playSound(SND_LOWBAT); }
    else if (pct >= 25)                  warned = 100;   // charged back up
  }

  static uint32_t lastLeft = 0;
  uint32_t sd = startDownAt;
  if (sd && now - sd >= 1500) {
    uint32_t left = (POWER_OFF_HOLD_MS - min<uint32_t>(now - sd, POWER_OFF_HOLD_MS) + 999) / 1000;
    if (left != lastLeft && left > 0) { lastLeft = left; playSound(SND_TICK); }
  } else lastLeft = 0;
}

// HOME + stick / SELECT shortcuts (profile, page, fighter)
uint32_t layerChangeAt = 0;
void handleLayerEvents() {
  uint8_t le = layerEvents.exchange(0);
  if (!le) return;
  if (le & (LE_PROF_PREV | LE_PROF_NEXT)) {
    int dir = (le & LE_PROF_NEXT) ? 1 : -1, pos = 0;
    while (pos < PROFILE_COUNT && PROFILE_ORDER[pos] != cfg.profile) pos++;
    cfg.profile = PROFILE_ORDER[(pos + dir + PROFILE_COUNT) % PROFILE_COUNT];
    toast(PROFILE_NAME[cfg.profile]);
  }
  if (le & (LE_PAGE_PREV | LE_PAGE_NEXT)) {
    int dir = (le & LE_PAGE_NEXT) ? 1 : -1;
    cfg.page = (cfg.page + dir + PAGE_COUNT) % PAGE_COUNT;
    toast(PAGE_NAME[cfg.page]);
  }
  if (le & LE_FIGHTER) {
    cfg.fighter = (cfg.fighter + 1) % GAMES[cfg.game].n;
    memset(moveCount, 0, sizeof moveCount);
    moveHit = -1;
    motion.clear();
    toast(GAMES[cfg.game].fighters[cfg.fighter].name);
  }
  settingsDirty = true;
  layerChangeAt = millis();
  playSound(SND_CHANGE);
  uiDirty = true;
}

enum ScreenPower : uint8_t { SCR_ON, SCR_DIM, SCR_OFF };
uint8_t scrPower = SCR_ON;
void setScreenPower(uint8_t p) {
  if (p == scrPower) return;
  scrPower = p;
  if (p == SCR_ON) applyDisplaySettings();
  else if (p == SCR_DIM) tft.setBrightness(map(min<int>(cfg.brightness, 15), 0, 100, 0, 255));
  else tft.setBrightness(0);
  uiDirty = true;
}

void uiTask(void *) {
  uint32_t lastFrame = 0, lastIdle = 0, lastBatt = 0, seenHit = 0;
  int lastBattPct = -2;
  for (;;) {
    uiStep();
    handleLayerEvents();
    readBattery();
    updateLed();
    soundEvents();
    if (web) web->handleClient();
    uint32_t now = millis();
    if (restartAt && (int32_t)(now - restartAt) >= 0) ESP.restart();

    // recognised move: count it, cheer
    uint32_t hs = moveHitSeq;
    if (hs != seenHit) {
      seenHit = hs;
      int8_t h = moveHit;
      if (h >= 0 && h < 8 && moveCount[h] < 999) moveCount[h]++;
      if (cfg.moveCheck == 2) playSound(SND_COMBO);
    }

    // idle: dim + attract mode after 30 s, screen off after the Screen sleep time
    uint32_t idle = now - uiInputTime;
    bool onStatus = (screen == S_STATUS && !menuOpen && !startDownAt);
    uint8_t want = SCR_ON;
    if (onStatus && idle > 30000) want = SCR_DIM;
    uint32_t offMs = (uint32_t)SLEEP_MIN[cfg.screenSleep] * 60000UL;
    if (onStatus && offMs && idle > offMs) want = SCR_OFF;
    setScreenPower(want);

    // auto power off: idle and not connected (or idle for an hour regardless)
    uint32_t aoff = (uint32_t)AUTOOFF_MIN[cfg.autoOff] * 60000UL;
    if (aoff && onStatus && batteryPercent() >= 0 &&
        ((idle > aoff && !hostConnected()) || idle > 3600000UL)) powerOffRequest = true;

    // battery level to the host (shows in SteamOS next to the controller)
    if (bootConn == C_BLE && !radioOff && now - lastBatt > 30000) {
      lastBatt = now;
      int p = batteryPercent();
      if (p >= 0 && p != lastBattPct) { blePad.setBatteryLevel(p); lastBattPct = p; }
    }

    // shortcut changes are saved a few seconds later (not on every press)
    if (settingsDirty && !menuOpen && layerChangeAt && now - layerChangeAt > 4000) {
      saveSettings(); settingsDirty = false; layerChangeAt = 0;
    }

    if (now - lastIdle > 1000) { lastIdle = now; uiDirty = true; }   // refresh status bits
    if (startDownAt) uiDirty = true;                                  // power-off countdown
    if (toastText && (int32_t)(toastUntil - now) > -100) uiDirty = true;
    if (cfg.page == PG_MOVES && moveHit >= 0 && now - moveHitAt < 1700) uiDirty = true;
    if (scrPower == SCR_DIM && haveCanvas && now - lastFrame >= 66) {   // attract mode ~15 fps
      lastFrame = now;
      drawAttract(canvas);
      canvas.pushSprite(0, 0);
    } else if (scrPower == SCR_ON && uiDirty && now - lastFrame >= 33 && haveCanvas) {  // max ~30 fps
      uiDirty = false;
      lastFrame = now;
      render();
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// ======================= SETUP / LOOP ===============================
void setup() {
  loadSettings();
  applyTheme();
  bootConn = cfg.conn;

  // USB: CDC serial always (debug + upload); HID gamepad only in USB mode
  USB.productName(DEVICE_NAME);
  USB.manufacturerName("DIY");
  USB.onEvent(usbEvent);
  debugSerial.begin(115200);
  if (bootConn == C_USB) usbPad.begin();
  USB.begin();

  if (bootConn == C_BLE) {
    BleGamepadConfiguration bc;
    bc.setAutoReport(false);
    bc.setControllerType(CONTROLLER_TYPE_GAMEPAD);
    bc.setButtonCount(15);
    bc.setHatSwitchCount(1);
    bc.setWhichAxes(true, true, true, false, false, true, false, false);  // X Y Z RZ
    bc.setAxesMin(-32767);
    bc.setAxesMax(32767);
    blePad.begin(&bc);
  }

  // display
  tft.init();
  applyDisplaySettings();
  canvas.setColorDepth(16);
  canvas.setPsram(true);
  haveCanvas = canvas.createSprite(320, 240) != nullptr;
  if (!haveCanvas) {             // no PSRAM enabled: fall back to 8-bit in internal RAM
    canvas.setPsram(false);
    canvas.setColorDepth(8);
    haveCanvas = canvas.createSprite(320, 240) != nullptr;
  }
  tft.fillScreen(C_BG);

  // expander
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_HZ);
  mcpOk = mcpInit();

  // Backup way into flash mode: hold START + SELECT while turning on
  uint16_t raw0;
  if (mcpOk && mcpRead(raw0) && (raw0 & (1u << MCP_BIT[P_START])) && (raw0 & (1u << MCP_BIT[P_SELECT]))) {
    tft.fillScreen(C_BG);
    tft.setFont(&fonts::FreeSansBold12pt7b);
    tft.setTextColor(C_TEXT);
    tft.setTextDatum(middle_center);
    tft.drawString("Flash mode", 160, 110);
    tft.setFont(&fonts::Font2);
    tft.drawString("screen turns off - flash at 0x0", 160, 140);
    delay(1200);
    tft.setBrightness(0);
    usb_persist_restart(RESTART_BOOTLOADER);
  }
  soundBegin();
  playSound(SND_BOOT);
  debugSerial.printf("Arcade Stick %s | mode %s | MCP23017 %s | canvas %s | sound %s\n", FW_VERSION,
                     bootConn == C_USB ? "USB" : "BLE", mcpOk ? "OK" : "NOT FOUND",
                     haveCanvas ? "OK" : "FAILED", soundOk ? "OK" : "NO CODEC");
  debugSerial.printf("Build %s\n", STICK_ID);

  xTaskCreatePinnedToCore(uiTask, "ui", 8192, nullptr, 1, nullptr, 0);
}

void loop() {
  if (shuttingDown) { vTaskDelay(10); return; }
  scanInputs();                     // ~1 kHz: one scan per 1 ms RTOS tick
  Report r = buildReport();
  if (r != lastSent || forceSend) sendReport(r);
  vTaskDelay(1);
}
