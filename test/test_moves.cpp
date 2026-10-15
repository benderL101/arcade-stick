#include <cstdio>
#include <vector>
#include <string>
#include "moves.h"
struct Step { int dir; int ms; };
const Game *G(const char *n) { for (auto &g : GAMES) if (!strcmp(g.name, n)) return &g; return nullptr; }
const Fighter *F(const Game *g, const char *n) { for (int i = 0; i < g->n; i++) if (!strcmp(g->fighters[i].name, n)) return &g->fighters[i]; return nullptr; }
int idx(const Game *g, const char *b) { for (int i = 0; i < 6; i++) if (!strcmp(g->btnName[i], b)) return i; return -1; }
// buttons: names pressed together (last one is "the press"), taps = extra presses of same before
std::string run(const char *game, const char *who, std::vector<Step> steps, std::vector<const char*> btns, int extraTaps = 0) {
  const Game *g = G(game); const Fighter *f = F(g, who);
  if (!g || !f) return "NO DATA";
  MotionReader mr; uint32_t t = 1000;
  for (auto s : steps) { mr.dir(s.dir, t); t += s.ms; }
  uint8_t held = 0; int last = 0;
  for (auto b : btns) { last = idx(g, b); held |= 1u << last; }
  bool mir = false;
  for (int i = 0; i < extraTaps; i++) { mr.press(last, held, t, *g, *f, mir); t += 120; }
  int r = mr.press(last, held, t, *g, *f, mir);
  if (r < 0) return "-";
  return std::string(f->moves[r].name) + (mir ? " (L)" : "");
}
int fails = 0;
void expect(const char *label, std::string got, const char *want) {
  bool ok = got == want; if (!ok) fails++;
  printf("%-34s %-26s %s\n", label, got.c_str(), ok ? "ok" : (std::string("FAIL want ") + want).c_str());
}
int main() {
  const char *SF = "Street Fighter II", *A3 = "Street Fighter Alpha 3", *MK2 = "Mortal Kombat II", *MK3 = "Mortal Kombat 3", *KI = "Killer Instinct", *MK1 = "Mortal Kombat";
  expect("SF2 Ryu QCF+LP",        run(SF, "Ryu", {{5,200},{2,40},{3,40},{6,30}}, {"LP"}), "Hadoken");
  expect("SF2 Ryu DP+HP",         run(SF, "Ryu", {{6,40},{2,40},{3,30}}, {"HP"}), "Shoryuken");
  expect("SF2 Ryu QCB+MK",        run(SF, "Ryu", {{2,40},{1,40},{4,30}}, {"MK"}), "Tatsumaki");
  expect("SF2 Ryu QCF facing L",  run(SF, "Ryu", {{2,40},{1,40},{4,30}}, {"LP"}), "Hadoken (L)");
  expect("SF2 Ryu too slow",      run(SF, "Ryu", {{2,500},{3,300},{6,300}}, {"LP"}), "-");
  expect("SF2 Guile boom",        run(SF, "Guile", {{4,1000},{6,40}}, {"MP"}), "Sonic Boom");
  expect("SF2 Guile short charge",run(SF, "Guile", {{4,400},{6,40}}, {"MP"}), "-");
  expect("SF2 Guile flash",       run(SF, "Guile", {{1,1000},{9,40}}, {"HK"}), "Flash Kick");
  expect("SF2 Zangief 360",       run(SF, "Zangief", {{6,50},{3,50},{2,50},{1,50},{4,50},{7,50},{8,40}}, {"HP"}), "Spinning Piledriver");
  expect("SF2 Zangief PPP",       run(SF, "Zangief", {{5,300}}, {"LP","MP","HP"}), "Double Lariat");
  expect("SF2 Zangief PP only",   run(SF, "Zangief", {{5,300}}, {"LP","MP"}), "-");
  expect("SF2 Dhalsim HCF",       run(SF, "Dhalsim", {{4,40},{1,40},{2,40},{3,40},{6,30}}, {"LP"}), "Yoga Flame");
  expect("SF2 Sagat 2369+K",      run(SF, "Sagat", {{2,40},{3,40},{6,40},{9,30}}, {"LK"}), "Tiger Knee");
  expect("SF2 Chun rapid K",      run(SF, "Chun-Li", {{5,500}}, {"MK"}, 2), "Lightning Legs");
  expect("SFA3 Akuma HCB+P",      run(A3, "Akuma", {{6,40},{3,40},{2,40},{1,40},{4,30}}, {"HP"}), "Shakunetsu Hadouken");
  expect("SFA3 Blanka F+KKK",     run(A3, "Blanka", {{6,400}}, {"LK","MK","HK"}), "Surprise Forward");
  expect("SFA3 Balrog [4]3+P",    run(A3, "Balrog", {{4,1000},{3,40}}, {"MP"}), "Dash Ground Straight");
  expect("SFA3 Cammy hooligan",   run(A3, "Cammy", {{4,40},{1,40},{2,40},{3,40},{6,40},{9,30}}, {"LP"}), "Hooligan Combination");
  expect("MK2 Liu F,F+HP",        run(MK2, "Liu Kang", {{5,100},{6,60},{5,60},{6,40}}, {"HP"}), "Fireball");
  expect("MK2 Liu F,F+LP",        run(MK2, "Liu Kang", {{5,100},{6,60},{5,60},{6,40}}, {"LP"}), "Low Fireball");
  expect("MK2 Liu hold F +HP",    run(MK2, "Liu Kang", {{5,100},{6,300}}, {"HP"}), "-");
  expect("MK2 Scorpion B,B+LP",   run(MK2, "Scorpion", {{4,60},{5,60},{4,40}}, {"LP"}), "Spear");
  expect("MK2 Scorpion spear L",  run(MK2, "Scorpion", {{6,60},{5,60},{6,40}}, {"LP"}), "Spear (L)");
  expect("MK2 SubZero D,F+LP",    run(MK2, "Sub-Zero", {{2,60},{3,30},{6,40}}, {"LP"}), "Iceball");
  expect("MK2 SubZero slide",     run(MK2, "Sub-Zero", {{4,400}}, {"LP","BL","LK"}), "Slide");
  expect("MK2 Kitana fan throw",  run(MK2, "Kitana", {{6,60},{5,60},{6,40}}, {"HP","LP"}), "Fan Throw");
  expect("MK2 Kitana fan lift",   run(MK2, "Kitana", {{4,60},{5,60},{4,60},{5,60},{4,40}}, {"HP"}), "Fan Lift");
  expect("MK2 Cage nut punch",    run(MK2, "Johnny Cage", {{5,300}}, {"BL","LP"}), "Nut Punch");
  expect("MK2 Reptile invisible", run(MK2, "Reptile", {{8,60},{5,60},{8,60},{2,40}}, {"BL","HP"}), "Invisible");
  expect("MK2 Kung Lao whirlwind",run(MK2, "Kung Lao", {{8,60},{5,60},{8,40}}, {"BL","LK"}), "Whirlwind");
  expect("MK3 SubZero front shwr",run(MK3, "Sub-Zero", {{2,60},{6,60},{4,40}}, {"HP"}), "Ice Shower (front)");
  expect("MK3 Kabal B,B,B+RUN",   run(MK3, "Kabal", {{4,50},{5,50},{4,50},{5,50},{4,40}}, {"RUN"}), "Ground Saw");
  expect("MK3 Cyrax far grenade", run(MK3, "Cyrax", {{6,50},{5,50},{6,40}}, {"LK","HK"}), "Far Grenade");
  expect("MK3 Cyrax net",         run(MK3, "Cyrax", {{4,50},{5,50},{4,40}}, {"LK"}), "Net");
  expect("MK1 Cage B,F+LP",       run(MK1, "Johnny Cage", {{4,60},{6,40}}, {"LP"}), "Green Bolt");
  expect("KI Riptor HCB+QP",      run(KI, "Riptor", {{6,40},{3,40},{2,40},{1,40},{4,30}}, {"QP"}), "Uppercut Slash");
  expect("KI Riptor HCB+MP",      run(KI, "Riptor", {{6,40},{3,40},{2,40},{1,40},{4,30}}, {"MP"}), "Fireball");
  expect("KI TJ B,F+FK",          run(KI, "T.J. Combo", {{4,60},{6,40}}, {"FK"}), "Flying Tiger Knee");
  expect("KI Spinal back+QP",     run(KI, "Spinal", {{4,500}}, {"QP"}), "Absorption Shield");
  printf("\nbattery: 4.20=%d 4.00=%d 3.85=%d 3.60=%d\n", lipoPercent(4.2f), lipoPercent(4.0f), lipoPercent(3.85f), lipoPercent(3.6f));
  printf("%s (%d failures)\n", fails ? "FAILED" : "ALL PASSED", fails);
  return fails != 0;
}
