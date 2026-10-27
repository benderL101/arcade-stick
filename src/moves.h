// =====================================================================
//  Fighting-game move lists + live motion recognition
//  Pure C++ (no Arduino), so it can be tested on a PC.
//
//  Motion strings use numpad notation for a player FACING RIGHT:
//      7 8 9      e.g. "236"   = down, down-forward, forward  (quarter circle forward)
//      4 5 6           "623"   = forward, down, down-forward  (dragon punch)
//      1 2 3           "41236" = half circle forward
//  Special forms:
//      "[4]6"  charge back ~1 s, then forward      "[2]8"  charge down, then up
//      "360"   full circle                         "tap"   press the button rapidly
//      ""      buttons only (e.g. all three punches)
//      "a|b"   either motion is accepted (the first one is shown)
//  Buttons: P = any punch (top row), K = any kick (bottom row),
//           PPP / KKK = all three together.
// =====================================================================
#pragma once
#include <stdint.h>
#include <string.h>

// A game says what its six buttons are called (B1..B6 = top row left->right,
// then bottom row) and which of them count as punches and kicks.
struct Move    { const char *name; const char *motion; const char *btn; };
struct Fighter { const char *name; const Move *moves; uint8_t n; };
struct Game {
  const char *name; const Fighter *fighters; uint8_t n;
  const char *btnName[6];     // "" = not used by this game
  uint8_t punchMask, kickMask; // bit 0 = B1 ... bit 5 = B6
};
//  btn: tokens joined by '+'. "P" / "K" = any punch / kick, "PP" / "PPP" etc. =
//  that many together, or a button name from the game ("HP", "BL", "QP" ...).
//  "" = no button (shown only), "hold X" = hold and release (shown only).

#define MLEN(x) (uint8_t)(sizeof(x) / sizeof(x[0]))
#define SF_BUTTONS  {"LP", "MP", "HP", "LK", "MK", "HK"}, 0x07, 0x38
#define MK_BUTTONS  {"HP", "BL", "HK", "LP", "RUN", "LK"}, 0x09, 0x24
#define KI_BUTTONS  {"QP", "MP", "FP", "QK", "MK", "FK"}, 0x07, 0x38

// =====================================================================
//  Street Fighter II (Turbo / Super)
// =====================================================================
static const Move SF2_RYU[] = {{"Hadoken", "236", "P"}, {"Shoryuken", "623", "P"}, {"Tatsumaki", "214", "K"}};
static const Move SF2_KEN[] = {{"Hadoken", "236", "P"}, {"Shoryuken", "623", "P"}, {"Tatsumaki", "214", "K"}};
static const Move SF2_GUILE[] = {{"Sonic Boom", "[4]6", "P"}, {"Flash Kick", "[2]8", "K"}};
static const Move SF2_CHUN[] = {{"Kikoken", "[4]6", "P"}, {"Spinning Bird Kick", "[2]8", "K"}, {"Lightning Legs", "tap", "K"}};
static const Move SF2_BLANKA[] = {{"Rolling Attack", "[4]6", "P"}, {"Vertical Roll", "[2]8", "K"}, {"Electricity", "tap", "P"}};
static const Move SF2_HONDA[] = {{"Sumo Headbutt", "[4]6", "P"}, {"Sumo Smash", "[2]8", "K"}, {"Hundred Hand Slap", "tap", "P"}};
static const Move SF2_ZANGIEF[] = {{"Spinning Piledriver", "360", "P"}, {"Double Lariat", "", "PPP"}};
static const Move SF2_DHALSIM[] = {{"Yoga Flame", "41236", "P"}, {"Yoga Fire", "236", "P"}};
static const Move SF2_BALROG[] = {{"Dash Straight", "[4]6", "P"}, {"Dash Upper", "[4]6", "K"}, {"Buffalo Headbutt", "[2]8", "P"}};
static const Move SF2_VEGA[] = {{"Rolling Crystal Flash", "[4]6", "P"}, {"Sky High Claw", "[2]8", "P"}, {"Flying Barcelona", "[2]8", "K"}};
static const Move SF2_SAGAT[] = {{"Tiger Uppercut", "623", "P"}, {"Tiger Knee", "2369|623", "K"}, {"Tiger Shot", "236", "P"}, {"Ground Tiger Shot", "236", "K"}};
static const Move SF2_BISON[] = {{"Psycho Crusher", "[4]6", "P"}, {"Scissor Kick", "[4]6", "K"}, {"Head Stomp", "[2]8", "K"}};
static const Move SF2_CAMMY[] = {{"Cannon Spike", "623", "K"}, {"Spiral Arrow", "236", "K"}};
static const Move SF2_FEILONG[] = {{"Flame Kick", "421", "K"}, {"Rekka Ken (x3)", "236", "P"}};
static const Move SF2_THAWK[] = {{"Mexican Typhoon", "360", "P"}, {"Tomahawk Buster", "623", "P"}};
static const Move SF2_DEEJAY[] = {{"Air Slasher", "[4]6", "P"}, {"Double Rolling Sobat", "[4]6", "K"}, {"Machine Gun Upper", "[2]8", "P"}};
static const Fighter SF2_FIGHTERS[] = {
  {"Ryu", SF2_RYU, MLEN(SF2_RYU)}, {"Ken", SF2_KEN, MLEN(SF2_KEN)}, {"Guile", SF2_GUILE, MLEN(SF2_GUILE)},
  {"Chun-Li", SF2_CHUN, MLEN(SF2_CHUN)}, {"Blanka", SF2_BLANKA, MLEN(SF2_BLANKA)}, {"E. Honda", SF2_HONDA, MLEN(SF2_HONDA)},
  {"Zangief", SF2_ZANGIEF, MLEN(SF2_ZANGIEF)}, {"Dhalsim", SF2_DHALSIM, MLEN(SF2_DHALSIM)}, {"Balrog", SF2_BALROG, MLEN(SF2_BALROG)},
  {"Vega", SF2_VEGA, MLEN(SF2_VEGA)}, {"Sagat", SF2_SAGAT, MLEN(SF2_SAGAT)}, {"M. Bison", SF2_BISON, MLEN(SF2_BISON)},
  {"Cammy", SF2_CAMMY, MLEN(SF2_CAMMY)}, {"Fei Long", SF2_FEILONG, MLEN(SF2_FEILONG)}, {"T. Hawk", SF2_THAWK, MLEN(SF2_THAWK)},
  {"Dee Jay", SF2_DEEJAY, MLEN(SF2_DEEJAY)},
};

// =====================================================================
//  Street Fighter Alpha 3 (special moves; supers and throws left out)
// =====================================================================
static const Move SFA3_ADON[] = {{"Jaguar Tooth", "63214", "K"}, {"Rising Jaguar", "623", "K"}, {"Jaguar Kick", "421", "K"}};
static const Move SFA3_AKUMA[] = {{"Shakunetsu Hadouken", "63214", "P"}, {"Gou Shoryuken", "623", "P"}, {"Gou Hadouken", "236", "P"},
  {"Tatsumaki Zankuukyaku", "214", "K"}, {"Zenpou Tenshin", "214", "P"}, {"Ashura Senkuu", "623|421", "PPP"}};
static const Move SFA3_BALROG[] = {{"Dash Straight", "[4]6", "P"}, {"Dash Upper", "[4]6", "K"}, {"Dash Ground Straight", "[4]3", "P"},
  {"Dash Ground Upper", "[4]3", "K"}, {"Buffalo Headbutt", "[2]8", "P"}, {"Turn Punch", "hold", "PPP"}};
static const Move SFA3_BIRDIE[] = {{"Murderer Chain", "360", "P"}, {"Bandit Chain", "360", "K"}, {"Bull Head", "[4]6", "P"}, {"Bull Horn", "hold", "PP"}};
static const Move SFA3_BLANKA[] = {{"Rolling Attack", "[4]6", "P"}, {"Backstep Rolling", "[4]6", "K"}, {"Vertical Rolling", "[2]8", "K"},
  {"Electric Thunder", "tap", "P"}, {"Surprise Forward", "6", "KKK"}, {"Surprise Back", "4", "KKK"}};
static const Move SFA3_CAMMY[] = {{"Hooligan Combination", "412369", "P"}, {"Axle Spin Knuckle", "63214", "P"}, {"Cannon Spike", "623", "K"},
  {"Spiral Arrow", "236", "K"}, {"Cannon Revenge", "214", "P"}};
static const Move SFA3_CHARLIE[] = {{"Sonic Boom", "[4]6", "P"}, {"Somersault Shell", "[2]8", "K"}};
static const Move SFA3_CHUN[] = {{"Kikouken", "41236", "P"}, {"Sen'en Shuu", "63214", "K"}, {"Tenshoukyaku", "[2]8", "K"},
  {"Sou Hakkei", "[4]6", "P"}, {"Hyakuretsu Kyaku", "tap", "K"}};
static const Move SFA3_CODY[] = {{"Bad Stone", "236", "P"}, {"Ruffian Kick", "236", "K"}, {"Criminal Upper", "214", "P"}};
static const Move SFA3_DAN[] = {{"Koryuken", "623", "P"}, {"Gadouken", "236", "P"}, {"Dankuukyaku", "214", "K"}};
static const Move SFA3_DEEJAY[] = {{"Air Slasher", "[4]6", "P"}, {"Double Rolling Sobat", "[4]6", "K"}, {"Machine Gun Upper", "[2]8", "P"},
  {"Jackknife Maximum", "[2]8", "K"}};
static const Move SFA3_DHALSIM[] = {{"Yoga Flame", "41236|63214", "P"}, {"Yoga Blast", "41236|63214", "K"}, {"Yoga Fire", "236", "P"},
  {"Yoga Teleport", "623|421", "PPP"}};
static const Move SFA3_HONDA[] = {{"Ooichou Nage", "360", "P"}, {"Super Zutsuki", "[4]6", "P"}, {"Super Hyakkan Otoshi", "[2]8", "K"},
  {"Hyakuretsu Harite", "tap", "P"}};
static const Move SFA3_FEILONG[] = {{"Shien Kyaku", "421", "K"}, {"Rekka Ken (x3)", "236", "P"}};
static const Fighter SFA3_FIGHTERS[] = {
  {"Adon", SFA3_ADON, MLEN(SFA3_ADON)}, {"Akuma", SFA3_AKUMA, MLEN(SFA3_AKUMA)}, {"Balrog", SFA3_BALROG, MLEN(SFA3_BALROG)},
  {"Birdie", SFA3_BIRDIE, MLEN(SFA3_BIRDIE)}, {"Blanka", SFA3_BLANKA, MLEN(SFA3_BLANKA)}, {"Cammy", SFA3_CAMMY, MLEN(SFA3_CAMMY)},
  {"Charlie", SFA3_CHARLIE, MLEN(SFA3_CHARLIE)}, {"Chun-Li", SFA3_CHUN, MLEN(SFA3_CHUN)}, {"Cody", SFA3_CODY, MLEN(SFA3_CODY)},
  {"Dan", SFA3_DAN, MLEN(SFA3_DAN)}, {"Dee Jay", SFA3_DEEJAY, MLEN(SFA3_DEEJAY)}, {"Dhalsim", SFA3_DHALSIM, MLEN(SFA3_DHALSIM)},
  {"E. Honda", SFA3_HONDA, MLEN(SFA3_HONDA)}, {"Fei Long", SFA3_FEILONG, MLEN(SFA3_FEILONG)},
};

// =====================================================================
//  Mortal Kombat (arcade). Taps: 66 = forward, forward
// =====================================================================
static const Move MK1_LIU[] = {{"Fireball", "66", "HP"}, {"Flying Kick", "66", "HK"}};
static const Move MK1_CAGE[] = {{"Green Bolt", "46", "LP"}, {"Shadow Kick", "46", "LK"}, {"Ball Breaker", "", "BL+LP"}};
static const Move MK1_KANO[] = {{"Knife Throw", "46", "hold BL"}};
static const Move MK1_RAIDEN[] = {{"Lightning", "41236", "LP"}, {"Body Launch", "446", ""}, {"Teleport", "28", ""}};
static const Move MK1_SCORPION[] = {{"Teleport Punch", "63214", "HP"}, {"Spear", "44", "LP"}};
static const Move MK1_SUBZERO[] = {{"Freeze", "41236", "LP"}, {"Slide", "4", "LP+LK+BL"}};
static const Move MK1_SONYA[] = {{"Flying Punch", "64", "HP"}, {"Leg Throw", "", "LP+LK+BL"}};
static const Fighter MK1_FIGHTERS[] = {
  {"Liu Kang", MK1_LIU, MLEN(MK1_LIU)}, {"Johnny Cage", MK1_CAGE, MLEN(MK1_CAGE)}, {"Kano", MK1_KANO, MLEN(MK1_KANO)},
  {"Raiden", MK1_RAIDEN, MLEN(MK1_RAIDEN)}, {"Scorpion", MK1_SCORPION, MLEN(MK1_SCORPION)}, {"Sub-Zero", MK1_SUBZERO, MLEN(MK1_SUBZERO)},
  {"Sonya", MK1_SONYA, MLEN(MK1_SONYA)},
};

// =====================================================================
//  Mortal Kombat II (arcade)
// =====================================================================
static const Move MK2_LIU[] = {{"Fireball", "66", "HP"}, {"Low Fireball", "66", "LP"}, {"Flying Kick", "66", "HK"}, {"Bicycle Kick", "hold", "LK"}};
static const Move MK2_KUNGLAO[] = {{"Hat Throw", "46", "LP"}, {"Whirlwind", "88", "BL+LK"}, {"Teleport", "28", ""}, {"Air Kick (jump)", "2", "HK"}};
static const Move MK2_CAGE[] = {{"Shadow Uppercut", "424", "HP"}, {"Low Bolt", "426", "LP"}, {"High Bolt", "624", "HP"}, {"Shadow Kick", "46", "LK"},
  {"Nut Punch", "", "LP+BL"}};
static const Move MK2_SHANG[] = {{"3 Skulls", "4466", "HP"}, {"2 Skulls", "446", "HP"}, {"1 Skull", "44", "HP"}};
static const Move MK2_REPTILE[] = {{"Invisible", "882", "BL+HP"}, {"Acid Spit", "66", "HP"}, {"Forceball", "44", "HP+LP"}, {"Slide", "4", "LP+BL+LK"}};
static const Move MK2_SUBZERO[] = {{"Iceball", "26", "LP"}, {"Ground Freeze", "24", "LK"}, {"Slide", "4", "LP+BL+LK"}};
static const Move MK2_SCORPION[] = {{"Leg Grab", "624", "LK"}, {"Spear", "44", "LP"}, {"Teleport Punch", "24", "HP"}};
static const Move MK2_RAIDEN[] = {{"Lightning", "26", "LP"}, {"Body Launch", "446", ""}, {"Electrocution", "hold", "HP"}};
static const Move MK2_JAX[] = {{"Energy Wave", "624", "HK"}, {"Gotcha Grab", "66", "LP"}, {"Ground Smash", "hold", "LK"}};
static const Move MK2_KITANA[] = {{"Air Punch", "624", "HP"}, {"Fan Lift", "444", "HP"}, {"Fan Throw", "66", "HP+LP"}, {"Fan Swipe", "4", "HP"}};
static const Move MK2_MILEENA[] = {{"Ground Roll", "442", "HK"}, {"Teleport Kick", "66", "LK"}, {"Sai Throw", "hold", "HP"}};
static const Move MK2_BARAKA[] = {{"Blade Fury", "444", "LP"}, {"Blue Bolt", "24", "HP"}, {"Blade Swipe", "4", "HP"}};
static const Fighter MK2_FIGHTERS[] = {
  {"Liu Kang", MK2_LIU, MLEN(MK2_LIU)}, {"Kung Lao", MK2_KUNGLAO, MLEN(MK2_KUNGLAO)}, {"Johnny Cage", MK2_CAGE, MLEN(MK2_CAGE)},
  {"Shang Tsung", MK2_SHANG, MLEN(MK2_SHANG)}, {"Reptile", MK2_REPTILE, MLEN(MK2_REPTILE)}, {"Sub-Zero", MK2_SUBZERO, MLEN(MK2_SUBZERO)},
  {"Scorpion", MK2_SCORPION, MLEN(MK2_SCORPION)}, {"Raiden", MK2_RAIDEN, MLEN(MK2_RAIDEN)}, {"Jax", MK2_JAX, MLEN(MK2_JAX)},
  {"Kitana", MK2_KITANA, MLEN(MK2_KITANA)}, {"Mileena", MK2_MILEENA, MLEN(MK2_MILEENA)}, {"Baraka", MK2_BARAKA, MLEN(MK2_BARAKA)},
};

// =====================================================================
//  Mortal Kombat 3 (arcade)
// =====================================================================
static const Move MK3_CYRAX[] = {{"Far Grenade", "66", "LK+HK"}, {"Close Grenade", "44", "LK+HK"}, {"Net", "44", "LK"}, {"Teleport", "62", "BL"}};
static const Move MK3_JAX[] = {{"2 Missiles", "6644", "HP"}, {"1 Missile", "46", "HP"}, {"Gotcha Grab", "66", "LP"}, {"Dashing Punch", "66", "HK"},
  {"Ground Pound", "hold", "LK"}};
static const Move MK3_KABAL[] = {{"Ground Saw", "444", "RUN"}, {"Top Spin", "46", "LK"}, {"Eye Spark", "44", "HP"}};
static const Move MK3_KANO[] = {{"Knife Throw", "24", "HP"}, {"Knife Uppercut", "26", "HP"}, {"Grab and Shake", "26", "LP"}, {"Cannon Ball", "hold", "LK"}};
static const Move MK3_KUNGLAO[] = {{"Spin", "626", "RUN"}, {"Hat Throw", "46", "LP"}, {"Flying Kick (jump)", "82", "HK"}, {"Teleport", "28", ""}};
static const Move MK3_LIU[] = {{"High Fireball", "66", "HP"}, {"Low Fireball", "66", "LP"}, {"Flying Kick", "66", "HK"}, {"Bicycle Kick", "hold", "LK"}};
static const Move MK3_NIGHTWOLF[] = {{"Glow", "444", "HK"}, {"Arrow", "24", "LP"}, {"Hatchet Uppercut", "26", "HP"}, {"Shadow Shoulder", "66", "LK"}};
static const Move MK3_SEKTOR[] = {{"Smart Missile", "624", "HP"}, {"Teleport Uppercut", "66", "LK"}, {"Missile", "66", "LP"}};
static const Move MK3_SHANG[] = {{"3 Fireballs", "4466", "HP"}, {"2 Fireballs", "446", "HP"}, {"1 Fireball", "44", "HP"}, {"Volcanic Eruption", "644", "LK"}};
static const Move MK3_SHEEVA[] = {{"Stomp", "424", "HK"}, {"Fireball", "26", "HP"}, {"Teleport Stomp", "28", ""}};
static const Move MK3_SINDEL[] = {{"Scream", "666", "HP"}, {"Fly", "446", "HK"}, {"Fireball", "66", "LP"}, {"Air Fireball (jump)", "26", "LK"}};
static const Move MK3_SMOKE[] = {{"Harpoon", "44", "LP"}, {"Teleport Uppercut", "66", "LK"}, {"Invisibility", "88", "RUN"}};
static const Move MK3_SONYA[] = {{"Bicycle Kick", "442", "HK"}, {"Energy Rings", "26", "LP"}, {"Square Wave Punch", "64", "HP"}, {"Leg Grab", "2", "LP+BL"}};
static const Move MK3_STRYKER[] = {{"High Grenade", "24", "HP"}, {"Low Grenade", "24", "LP"}, {"Baton Trip", "64", "LP"}, {"Baton Toss", "66", "HK"}};
static const Move MK3_SUBZERO[] = {{"Ice Shower (front)", "264", "HP"}, {"Ice Shower", "26", "HP"}, {"Freeze", "26", "LP"}, {"Ice Statue", "24", "LP"},
  {"Slide", "4", "LP+BL+LK"}};
static const Fighter MK3_FIGHTERS[] = {
  {"Cyrax", MK3_CYRAX, MLEN(MK3_CYRAX)}, {"Jax", MK3_JAX, MLEN(MK3_JAX)}, {"Kabal", MK3_KABAL, MLEN(MK3_KABAL)},
  {"Kano", MK3_KANO, MLEN(MK3_KANO)}, {"Kung Lao", MK3_KUNGLAO, MLEN(MK3_KUNGLAO)}, {"Liu Kang", MK3_LIU, MLEN(MK3_LIU)},
  {"Nightwolf", MK3_NIGHTWOLF, MLEN(MK3_NIGHTWOLF)}, {"Sektor", MK3_SEKTOR, MLEN(MK3_SEKTOR)}, {"Shang Tsung", MK3_SHANG, MLEN(MK3_SHANG)},
  {"Sheeva", MK3_SHEEVA, MLEN(MK3_SHEEVA)}, {"Sindel", MK3_SINDEL, MLEN(MK3_SINDEL)}, {"Smoke", MK3_SMOKE, MLEN(MK3_SMOKE)},
  {"Sonya", MK3_SONYA, MLEN(MK3_SONYA)}, {"Stryker", MK3_STRYKER, MLEN(MK3_STRYKER)}, {"Sub-Zero", MK3_SUBZERO, MLEN(MK3_SUBZERO)},
};

// =====================================================================
//  Killer Instinct (arcade / SNES). QP MP FP = quick / medium / fierce punch
// =====================================================================
static const Move KI_TJ[] = {{"Swinging Backhand", "46", "QP"}, {"Double Roll & Punch", "46", "MP"}, {"Charge and Punch", "46", "FP"},
  {"Straight Knee", "46", "QK"}, {"Flying Tiger Knee", "46", "FK"}};
static const Move KI_JAGO[] = {{"Fireball", "41236", "P"}, {"Uppercut", "626", "P"}, {"Blade Swipe", "214", "FP"}, {"Flying Heel Kick", "214", "K"}};
static const Move KI_GLACIUS[] = {{"Slide", "63214", "FP"}, {"Puddle Port", "41236", "QK"}, {"Puddle Punch", "41236", "FK"}, {"Blade Arm", "62", "QP"},
  {"Shoulder Slam", "46", "P"}};
static const Move KI_SPINAL[] = {{"Teleport Front", "222", "FP"}, {"Teleport Back", "222", "FK"}, {"Flaming Skull", "41236", "P"},
  {"Charge", "66", "MP"}, {"Super Slash", "46", "MP"}, {"Absorption Shield", "4", "QP"}};
static const Move KI_ORCHID[] = {{"Spinning Slash", "63214", "FK"}, {"Shadow", "63214", "MP"}, {"Laser Boomerang", "236", "MP"},
  {"Helicopter Blade", "621", "P"}, {"Helicopter Kick", "46", "MK"}, {"Backflip Kick", "4", "FK"}};
static const Move KI_CHIEF[] = {{"Head Attack", "63214", "FP"}, {"Reverse Head Attack", "41236", "FP"}, {"Tomahawk Throw", "41236", "K"},
  {"Spinning Chop", "46", "P"}};
static const Move KI_FULGORE[] = {{"Electric Bolts", "41236", "P"}, {"Reflector", "63214", "P"}, {"Uppercut", "626", "P"},
  {"Teleport Near", "424", "P"}, {"Teleport Far", "424", "K"}, {"Lightning Bolt", "214", "FK"}};
static const Move KI_CINDER[] = {{"Outline", "63214", "MP"}, {"Invisible", "63214", "FP"}, {"Flip Kick", "626", "K"},
  {"Human Fireball", "66", "P"}, {"Flame Thrower", "66", "K"}, {"Flaming Palm", "44", "QP"}};
static const Move KI_SABREWULF[] = {{"Lunge", "63214", "K"}, {"Throw Bat", "41236", "P"}, {"Spinning Claw", "46", "P"},
  {"Rolling Slash", "46", "QK"}, {"Lunge (far)", "46", "FK"}};
static const Move KI_RIPTOR[] = {{"Uppercut Slash", "63214", "QP"}, {"Fireball", "63214", "P"}, {"Tail", "63214", "MK"},
  {"Big Leap", "41236", "QK"}, {"Charge", "46", "P"}, {"Leaping Claws", "46", "K"}};
static const Fighter KI_FIGHTERS[] = {
  {"T.J. Combo", KI_TJ, MLEN(KI_TJ)}, {"Jago", KI_JAGO, MLEN(KI_JAGO)}, {"Glacius", KI_GLACIUS, MLEN(KI_GLACIUS)},
  {"Spinal", KI_SPINAL, MLEN(KI_SPINAL)}, {"B. Orchid", KI_ORCHID, MLEN(KI_ORCHID)}, {"Chief Thunder", KI_CHIEF, MLEN(KI_CHIEF)},
  {"Fulgore", KI_FULGORE, MLEN(KI_FULGORE)}, {"Cinder", KI_CINDER, MLEN(KI_CINDER)}, {"Sabrewulf", KI_SABREWULF, MLEN(KI_SABREWULF)},
  {"Riptor", KI_RIPTOR, MLEN(KI_RIPTOR)},
};

static const Game GAMES[] = {
  {"Street Fighter II", SF2_FIGHTERS, MLEN(SF2_FIGHTERS), SF_BUTTONS},
  {"Street Fighter Alpha 3", SFA3_FIGHTERS, MLEN(SFA3_FIGHTERS), SF_BUTTONS},
  {"Mortal Kombat", MK1_FIGHTERS, MLEN(MK1_FIGHTERS), MK_BUTTONS},
  {"Mortal Kombat II", MK2_FIGHTERS, MLEN(MK2_FIGHTERS), MK_BUTTONS},
  {"Mortal Kombat 3", MK3_FIGHTERS, MLEN(MK3_FIGHTERS), MK_BUTTONS},
  {"Killer Instinct", KI_FIGHTERS, MLEN(KI_FIGHTERS), KI_BUTTONS},
};
#define GAME_COUNT MLEN(GAMES)

// ---------------------------------------------------------------------
//  Motion recognition
// ---------------------------------------------------------------------
// Timing (ms). Generous, like the original games.
#define MR_WINDOW      600    // a normal motion must be done within this
#define MR_WINDOW_LONG 900    // half circles
#define MR_WINDOW_360  1100   // full circle
#define MR_LATE        250    // button may come this long after the last direction
#define MR_CHARGE      800    // hold time for charge moves
#define MR_CHARGE_GAP  350    // charge release -> forward/up
#define MR_TAP_WINDOW  550    // 3 presses within this = rapid tap

static inline uint8_t mirrorDir(uint8_t d) {
  switch (d) { case 1: return 3; case 3: return 1; case 4: return 6; case 6: return 4;
               case 7: return 9; case 9: return 7; default: return d; }
}
static inline uint32_t mrMin(uint32_t a, uint32_t b) { return a < b ? a : b; }
static inline bool isBack(uint8_t d) { return d == 1 || d == 4 || d == 7; }
static inline bool isFwd(uint8_t d)  { return d == 3 || d == 6 || d == 9; }
static inline bool isDown(uint8_t d) { return d == 1 || d == 2 || d == 3; }
static inline bool isUp(uint8_t d)   { return d == 7 || d == 8 || d == 9; }

// Trainer feedback for one target move (MotionReader::coach)
enum CoachCode : uint8_t {
  CO_NONE,      // no attempt yet
  CO_OK,        // got it
  CO_BUTTON,    // motion right, wrong button
  CO_NOMOTION,  // button only, no motion
  CO_MISSED,    // a direction was missed: dir says which (0 = not sure)
  CO_SLOW,      // all directions, but too slowly: ms = how long it took
  CO_LATE,      // button pressed too long after the motion: ms = gap
  CO_CHARGE,    // charge too short: ms = how long it was held
  CO_CIRCLE,    // 360 not completed
  CO_TAP,       // not tapped fast enough
  CO_HOLDDIR,   // direction (dir) not held while pressing
};
struct Coach { uint8_t code; uint8_t dir; uint16_t ms; bool mirrored; };

class MotionReader {
 public:
  struct Ev { uint8_t dir; uint32_t t; };
  static const uint8_t HIST = 40;

  // Call whenever the stick direction changes (numpad 1-9, 5 = neutral).
  void dir(uint8_t d, uint32_t t) {
    if (count && last().dir == d) return;
    ev[head] = {d, t};
    head = (head + 1) % HIST;
    if (count < HIST) count++;
  }

  // Call when one of the six buttons is pressed. btn = 0..5 (B1..B6),
  // held = bit mask of the six buttons held now (including btn).
  // Returns the index of the recognised move in f.moves, or -1.
  // mirrored = it was done facing left.
  int press(uint8_t btn, uint8_t held, uint32_t t, const Game &g, const Fighter &f, bool &mirrored) {
    int type = noteTap(btn, t, g);
    int best = -1, bestScore = -1;
    for (uint8_t i = 0; i < f.n; i++) {
      const Move &m = f.moves[i];
      int bs = buttonScore(m.btn, btn, held, g);
      if (bs < 0) continue;
      for (int mir = 0; mir < 2; mir++) {
        int sc = matchAny(m.motion, t, mir, type);
        if (sc < 0) continue;
        sc = sc * 4 + bs;                              // motion first, then the more exact button
        if (sc > bestScore) { bestScore = sc; best = i; mirrored = mir; }
      }
    }
    return best;
  }

  // Trainer: the player pressed `btn` while practising move `m`. Says whether
  // it worked and, if not, the most likely reason. Call instead of press().
  Coach coach(const Move &m, uint8_t btn, uint8_t held, uint32_t t, const Game &g) {
    int type = noteTap(btn, t, g);
    bool btnOk = buttonScore(m.btn, btn, held, g) >= 0;
    int s0 = matchAny(m.motion, t, false, type), s1 = matchAny(m.motion, t, true, type);
    if (s0 >= 0 || s1 >= 0) return {uint8_t(btnOk ? CO_OK : CO_BUTTON), 0, 0, s1 > s0};
    char buf[16];                                     // diagnose the first written form
    size_t len = strcspn(m.motion, "|");
    if (len >= sizeof buf) len = sizeof buf - 1;
    memcpy(buf, m.motion, len); buf[len] = 0;
    if (!strcmp(buf, "tap")) return {CO_TAP, 0, 0, false};
    if (len == 1 && buf[0] >= '1' && buf[0] <= '9') return {CO_HOLDDIR, uint8_t(buf[0] - '0'), 0, false};
    if (!strcmp(buf, "360")) return {CO_CIRCLE, 0, 0, false};
    if (buf[0] == '[') {
      uint32_t h0 = longestHold(buf[1] - '0', t, false), h1 = longestHold(buf[1] - '0', t, true);
      bool mir = h1 > h0;
      return {CO_CHARGE, uint8_t(mir ? mirrorDir(buf[1] - '0') : buf[1] - '0'), uint16_t(mrMin(mir ? h1 : h0, 9999)), mir};
    }
    Diag d0 = seqDiag(buf, t, false), d1 = seqDiag(buf, t, true);
    return d1.found > d0.found ? d1.c : d0.c;         // the way round they were facing
  }

  void clear() { count = 0; head = 0; memset(tapT, 0, sizeof tapT); }

 private:
  Ev ev[HIST] = {};
  uint8_t head = 0, count = 0;
  uint32_t tapT[2][3] = {};
  uint8_t tapHead[2] = {0, 0};

  int noteTap(uint8_t btn, uint32_t t, const Game &g) {
    int type = (g.punchMask >> btn) & 1 ? 0 : ((g.kickMask >> btn) & 1 ? 1 : -1);
    if (type >= 0) {                                   // rapid-tap tracking per punch / kick
      tapT[type][tapHead[type]] = t;
      tapHead[type] = (tapHead[type] + 1) % 3;
    }
    return type;
  }

  // Relaxed sequence match for the trainer: finds the steps in order within
  // the last 2.5 s, ignoring the timing, then says what went wrong.
  struct Diag { Coach c; int found; };
  Diag seqDiag(const char *m, uint32_t t, bool mir) const {
    int n = strlen(m), need = n - 1;
    uint32_t lastEnter = 0, lastEnd = 0, firstEnd = 0;
    for (int back = 0; back < count && need >= 0; back++) {
      const Ev &e = at(back);
      if (t - e.t > 2500) break;
      uint8_t want = m[need] - '0';
      if (mir) want = mirrorDir(want);
      if (e.dir != want) continue;
      uint32_t endT = back == 0 ? t : at(back - 1).t;
      if (need == n - 1) { lastEnter = e.t; lastEnd = endT; }
      firstEnd = endT;
      need--;
    }
    int found = n - 1 - need;
    if (found == 0) return {{CO_NOMOTION, 0, 0, mir}, 0};
    if (need >= 0) {                                   // a step never happened
      uint8_t d = m[need] - '0';
      return {{CO_MISSED, uint8_t(mir ? mirrorDir(d) : d), 0, mir}, found};
    }
    uint32_t since = lastEnd == t ? t - lastEnter : t - lastEnd;
    if (since > MR_LATE) return {{CO_LATE, 0, uint16_t(mrMin(since, 9999)), mir}, found};
    return {{CO_SLOW, 0, uint16_t(mrMin(t - firstEnd, 9999)), mir}, found};
  }

  // longest continuous hold of `from` (back/down) in the last 3 s
  uint32_t longestHold(uint8_t from, uint32_t t, bool mir) const {
    bool horiz = (from == 4);
    uint32_t best = 0, runStart = 0; bool inRun = false;
    for (int back = count - 1; back >= 0; back--) {    // oldest -> newest
      const Ev &e = at(back);
      uint32_t endT = back == 0 ? t : at(back - 1).t;
      if (t - endT > 3000) continue;
      bool isFrom = horiz ? (mir ? isFwd(e.dir) : isBack(e.dir)) : isDown(e.dir);
      if (isFrom) { if (!inRun) { runStart = e.t; inRun = true; } if (endT - runStart > best) best = endT - runStart; }
      else inRun = false;
    }
    return best;
  }

  const Ev &last() const { return ev[(head + HIST - 1) % HIST]; }
  const Ev &at(int back) const { return ev[(head + HIST - 1 - back) % HIST]; }   // 0 = newest

  // Does pressing button `btn` (with `held` down) complete this button spec?
  // -1 = no, otherwise how exact the spec is (named buttons beat "any punch").
  static int buttonScore(const char *spec, uint8_t btn, uint8_t held, const Game &g) {
    if (!spec[0] || !strncmp(spec, "hold", 4)) return -1;        // shown only
    int score = 0;
    bool pressedUsed = false;
    const char *p = spec;
    while (*p) {
      const char *plus = strchr(p, '+');
      size_t len = plus ? (size_t)(plus - p) : strlen(p);
      uint8_t mask = 0; int need = 1;
      if (len >= 1 && (p[0] == 'P' || p[0] == 'K') && strspn(p, p[0] == 'P' ? "P" : "K") >= len) {
        mask = p[0] == 'P' ? g.punchMask : g.kickMask;            // P, PP, PPP / K, KK, KKK
        need = (int)len;
      } else {
        for (uint8_t i = 0; i < 6; i++)
          if (strlen(g.btnName[i]) == len && !strncmp(g.btnName[i], p, len)) mask = 1u << i;
        if (!mask) return -1;                                       // button not in this game
        score += 2;
      }
      if (__builtin_popcount(held & mask) < need) return -1;
      if (mask & (1u << btn)) pressedUsed = true;
      score += need;
      if (!plus) break;
      p = plus + 1;
    }
    return pressedUsed ? score : -1;
  }

  // score = how specific the match is (longer motions win), -1 = no match
  int matchAny(const char *motion, uint32_t t, bool mir, int type) {
    int best = -1;
    char buf[16];
    const char *p = motion;
    while (true) {
      const char *bar = strchr(p, '|');
      size_t len = bar ? (size_t)(bar - p) : strlen(p);
      if (len >= sizeof buf) len = sizeof buf - 1;
      memcpy(buf, p, len); buf[len] = 0;
      int sc = matchOne(buf, t, mir, type);
      if (sc > best) best = sc;
      if (!bar) break;
      p = bar + 1;
    }
    return best;
  }

  int matchOne(const char *m, uint32_t t, bool mir, int type) {
    if (!*m) return 1;                                         // buttons only
    if (!strcmp(m, "hold")) return -1;                         // shown only
    if (!strcmp(m, "tap")) return type >= 0 && tapped(type, t) ? 2 : -1;
    if (m[1] == 0 && m[0] >= '1' && m[0] <= '9') {             // a direction held while pressing
      uint8_t want = m[0] - '0';
      if (mir) want = mirrorDir(want);
      return count && at(0).dir == want ? 3 : -1;
    }
    if (!strcmp(m, "360")) return circle(t) ? 20 : -1;
    if (m[0] == '[') { int c = charge(m[1] - '0', m[3] - '0', t, mir); return c ? 14 + c : -1; }
    return sequence(m, t, mir) ? 5 + (int)strlen(m) : -1;
  }

  bool tapped(int type, uint32_t t) const {
    for (int i = 0; i < 3; i++)
      if (tapT[type][i] == 0 || t - tapT[type][i] > MR_TAP_WINDOW) return false;
    return true;
  }

  // ordered directions, each must appear after the previous, all inside the window
  bool sequence(const char *m, uint32_t t, bool mir) const {
    int n = strlen(m);
    uint32_t window = n >= 5 ? MR_WINDOW_LONG : MR_WINDOW;
    int need = n - 1;
    for (int back = 0; back < count && need >= 0; back++) {
      const Ev &e = at(back);
      uint8_t want = m[need] - '0';
      if (mir) want = mirrorDir(want);
      uint32_t endT = back == 0 ? t : at(back - 1).t;   // when this direction was left
      if (need == n - 1) {
        // last direction: still held, or let go only just before the button
        // entered recently if still held, or released only just before the button
        uint32_t since = back == 0 ? t - e.t : t - endT;
        if (e.dir == want && since <= MR_LATE) need--;
        else if (t - endT > MR_LATE + window) return false;
        continue;
      }
      if (t - endT > window) return false;
      if (e.dir == want) need--;
    }
    return need < 0;
  }

  // held back (or down) for MR_CHARGE, then forward (or up) shortly before the button.
  // 0 = no, 1 = released roughly that way, 2 = released exactly that way.
  // A diagonal release ([4]3) must be exact.
  int charge(uint8_t from, uint8_t to, uint32_t t, bool mir) const {
    bool horiz = (from == 4);
    uint8_t want = mir ? mirrorDir(to) : to;
    bool diag = (to == 1 || to == 3 || to == 7 || to == 9);
    for (int back = 0; back < count; back++) {
      const Ev &e = at(back);
      if (t - e.t > 3000) return 0;
      bool isTo = diag ? e.dir == want : (horiz ? (mir ? isBack(e.dir) : isFwd(e.dir)) : isUp(e.dir));
      if (!isTo) continue;
      // forward/up must be held now or released just before the button
      uint32_t endT = back == 0 ? t : at(back - 1).t;
      if (back != 0 && t - endT > MR_LATE) continue;
      if (back == 0 && t - e.t > MR_LATE) continue;
      // walk further back: a continuous charge right before it
      uint32_t chargeEnd = e.t, chargeStart = 0;
      bool found = false;
      for (int b2 = back + 1; b2 < count; b2++) {
        const Ev &c = at(b2);
        bool isFrom = horiz ? (mir ? isFwd(c.dir) : isBack(c.dir)) : isDown(c.dir);
        if (!isFrom) { if (found) break; if (chargeEnd - c.t > MR_CHARGE_GAP) break; continue; }
        found = true;
        chargeStart = c.t;
      }
      if (found && chargeEnd - chargeStart >= MR_CHARGE) return e.dir == want ? 2 : 1;
    }
    return 0;
  }

  // a full circle: left, down, right and up all touched within the window
  bool circle(uint32_t t) const {
    bool l = false, d = false, r = false, u = false;
    for (int back = 0; back < count; back++) {
      const Ev &e = at(back);
      if (t - e.t > MR_WINDOW_360) break;
      l |= isBack(e.dir); r |= isFwd(e.dir); d |= isDown(e.dir); u |= isUp(e.dir);
    }
    return l && d && r && u;
  }
};

// Battery: LiPo resting voltage -> percent (typical single-cell curve)
static inline int lipoPercent(float v) {
  static const float V[] = {3.30f, 3.50f, 3.65f, 3.70f, 3.73f, 3.77f, 3.79f, 3.82f, 3.87f, 3.93f, 4.00f, 4.10f, 4.18f};
  static const int   P[] = {0,     5,     10,    15,    20,    30,    40,    50,    60,    70,    80,    90,    100};
  const int n = sizeof(V) / sizeof(V[0]);
  if (v <= V[0]) return 0;
  if (v >= V[n - 1]) return 100;
  for (int i = 1; i < n; i++)
    if (v < V[i]) return P[i - 1] + (int)((v - V[i - 1]) / (V[i] - V[i - 1]) * (P[i] - P[i - 1]) + 0.5f);
  return 100;
}
