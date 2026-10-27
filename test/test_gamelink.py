#!/usr/bin/env python3
"""Host test for Game link parsing (RetroArch status replies, labels file).

Pulls the real functions out of src/main.cpp, compiles them with a tiny
Arduino-String stand-in and runs them:  python3 test/test_gamelink.py
"""
import os, subprocess, sys, tempfile
HERE = os.path.dirname(os.path.abspath(__file__))
src = open(os.path.join(HERE, "..", "src", "main.cpp")).read()
def cut(a, b):
    i = src.index(a); return src[i:src.index(b, i)]
out_blk = src[src.index("enum Out : uint8_t {"):src.index("};", src.index("const char *OUT_NAME")) + 2]
lab = cut("struct LabelSet {", "void labelsLoad() {")
lab = lab.replace("std::vector<LabelSet> labelDb;\n", "").replace("bool fsOk = false;\n", "")
lab = lab.replace("struct LabelSet { String key; String lab[OUT_COUNT]; };",
                  "struct LabelSet { String key; String lab[OUT_COUNT]; };\nextern std::vector<LabelSet> labelDb;")
find = cut("int labelsFind(const String &key) {", "String gameKey(")
split = cut("String splitStatus(", "void parseStatus(")
SHIM = r'''
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cctype>
struct String {
  std::string s;
  String() {}
  String(const char *c) : s(c ? c : "") {}
  String(const std::string &x) : s(x) {}
  String(char c) : s(1, c) {}
  unsigned length() const { return s.size(); }
  std::string::const_iterator begin() const { return s.begin(); }
  std::string::const_iterator end() const { return s.end(); }
  char operator[](int i) const { return s[i]; }
  const char *c_str() const { return s.c_str(); }
  int indexOf(char c, int from = 0) const { auto p = s.find(c, from); return p == std::string::npos ? -1 : (int)p; }
  int indexOf(const String &k, int from = 0) const { auto p = s.find(k.s, from); return p == std::string::npos ? -1 : (int)p; }
  int lastIndexOf(const String &k) const { auto p = s.rfind(k.s); return p == std::string::npos ? -1 : (int)p; }
  String substring(int a) const { return a >= (int)s.size() ? String() : String(s.substr(a)); }
  String substring(int a, int b) const { if (b > (int)s.size()) b = s.size(); return a >= b ? String() : String(s.substr(a, b - a)); }
  bool startsWith(const String &p) const { return s.compare(0, p.s.size(), p.s) == 0; }
  void toLowerCase() { for (auto &c : s) c = tolower(c); }
  void trim() { size_t a = s.find_first_not_of(" \t\r\n"); size_t b = s.find_last_not_of(" \t\r\n"); s = a == std::string::npos ? "" : s.substr(a, b - a + 1); }
  void replace(const String &f, const String &t) { size_t p = 0; while ((p = s.find(f.s, p)) != std::string::npos) { s.replace(p, f.s.size(), t.s); p += t.s.size(); } }
  String &operator+=(const String &o) { s += o.s; return *this; }
  String &operator+=(const char *o) { s += o; return *this; }
  String &operator+=(char c) { s += c; return *this; }
  bool operator==(const String &o) const { return s == o.s; }
  bool operator==(const char *o) const { return s == o; }
};
String operator+(const String &a, const String &b) { return a.s + b.s; }
String operator+(const char *a, const String &b) { return std::string(a) + b.s; }
#define LABEL_LEN 16
'''
TEST = r'''std::vector<LabelSet> labelDb;
int fails = 0;
void ex(const char *what, std::string got, std::string want) {
  bool ok = got == want; if (!ok) fails++;
  printf("%-4s %-34s got [%s]%s\n", ok ? "ok" : "FAIL", what, got.c_str(), ok ? "" : (" want [" + want + "]").c_str());
}
std::string st(const char *r) { String sys, t; String s = splitStatus(String(r), sys, t); return s.s + "|" + sys.s + "|" + t.s; }
int main() {
  ex("playing snes", st("GET_STATUS PLAYING snes,Super Mario World (USA),crc32=b19ed489"), "PLAYING|snes|Super Mario World (USA)");
  ex("arcade rom name", st("GET_STATUS PLAYING fbneo,nbajam,crc32=1f2e3d4c"), "PLAYING|fbneo|nbajam");
  ex("title with commas", st("GET_STATUS PAUSED mame,Street Fighter II, The World Warrior,crc32=0000abcd"), "PAUSED|mame|Street Fighter II, The World Warrior");
  ex("no crc", st("GET_STATUS PLAYING genesis,Sonic"), "PLAYING|genesis|Sonic");
  ex("contentless", st("GET_STATUS CONTENTLESS"), "CONTENTLESS||");
  ex("trailing newline", st("GET_STATUS PLAYING snes,Zelda,crc32=1"), "PLAYING|snes|Zelda");
  ex("not a status", st("VERSION 1.19.1"), "||");
  // labels file round trip
  String file = "nbajam\tA=Shoot/Block\tB=Pass/Steal\tX=Turbo\n"
                "sys:snes\tA=Jump\tB=Run\r\n"
                "bad line without tab\n"
                "street fighter ii (usa)\tL1=Too long label text here\tQQ=ignored\tR1=HK\n";
  labelsParse(file, labelDb);
  ex("entries parsed", std::to_string(labelDb.size()), "3");
  ex("nbajam A", labelDb[0].lab[outIndex("A")].s, "Shoot/Block");
  ex("nbajam X", labelDb[0].lab[outIndex("X")].s, "Turbo");
  ex("sys key", labelDb[1].key.s, "sys:snes");
  ex("CRLF trimmed", labelDb[1].lab[outIndex("B")].s, "Run");
  ex("long label cut to 15", labelDb[2].lab[outIndex("L1")].s, "Too long label");
  ex("unknown button ignored", labelDb[2].lab[outIndex("R1")].s, "HK");
  String back = labelsText();
  std::vector<LabelSet> db2; labelsParse(back, db2);
  bool same = db2.size() == labelDb.size();
  for (size_t i = 0; same && i < db2.size(); i++) { same = db2[i].key == labelDb[i].key; for (int k = 0; k < OUT_COUNT; k++) same = same && db2[i].lab[k] == labelDb[i].lab[k]; }
  ex("save -> load round trip", same ? "same" : "different", "same");
  ex("find game", std::to_string(labelsFind(String("nbajam"))), "0");
  ex("find missing", std::to_string(labelsFind(String("tetris"))), "-1");
  printf(fails ? "\n%d FAILURES\n" : "\nALL PASSED\n", fails);
  return fails != 0;
}
'''
d = tempfile.mkdtemp()
cpp = os.path.join(d, "t.cpp"); exe = os.path.join(d, "t")
open(cpp, "w").write(SHIM + out_blk + "\n" + lab + find + split + TEST)
subprocess.check_call(["g++", "-std=c++17", "-Wall", "-Wno-unused-function", cpp, "-o", exe])
sys.exit(subprocess.call([exe]))
