#!/usr/bin/env python3
"""Independent checks of the finished board file (not the generator's own asserts).

1. MCP23017 pin functions (from the Microchip datasheet's pinout table) match the nets.
2. Every terminal's signal pad drives the MCP pin the firmware expects for that
   input (parsed from MCP_BIT_STD / PHYS enum in src/main.cpp).
3. Every net contains exactly the expected pads; nothing extra is connected.
4. The silkscreen label at each terminal names the net on that terminal's pad.
5. Copper connectivity: KiCad's own connectivity reports every net fully routed.
"""
import re, sys, pcbnew

import os
HERE = os.path.dirname(os.path.abspath(__file__))
PCB = os.path.join(HERE, "arcade_stick_io.kicad_pcb")
FW = os.path.join(HERE, "..", "..", "src", "main.cpp")
b = pcbnew.LoadBoard(PCB)
fails = []
def check(ok, msg):
    print(("ok   " if ok else "FAIL ") + msg)
    if not ok: fails.append(msg)

pads = {}   # "U1-24" -> net name
for fp in b.GetFootprints():
    for p in fp.Pads():
        if p.GetNumber():
            pads[f"{fp.GetReference()}-{p.GetNumber()}"] = p.GetNetname()

# 1. datasheet pinout (MCP23017 SPDIP-28, Table "Pinout description")
DS = {1: "GPB0", 2: "GPB1", 3: "GPB2", 4: "GPB3", 5: "GPB4", 6: "GPB5", 7: "GPB6", 8: "GPB7",
      9: "VDD", 10: "VSS", 11: "NC", 12: "SCL", 13: "SDA", 14: "NC", 15: "A0", 16: "A1", 17: "A2",
      18: "RESET", 19: "INTB", 20: "INTA", 21: "GPA0", 22: "GPA1", 23: "GPA2", 24: "GPA3",
      25: "GPA4", 26: "GPA5", 27: "GPA6", 28: "GPA7"}
REQUIRED = {"VDD": "3V3", "VSS": "GND", "SCL": "SCL", "SDA": "SDA", "A0": "GND", "A1": "GND",
            "A2": "GND", "RESET": "3V3", "INTA": "INTA", "INTB": "INTB", "NC": "", "GPA7": "", "GPB7": ""}
for pin, fn in DS.items():
    got = pads[f"U1-{pin}"]
    if fn in REQUIRED:
        check(got == REQUIRED[fn], f"U1 pin {pin:2} {fn:5} -> '{got}' (want '{REQUIRED[fn]}')")

# 2. firmware standard wiring
src = open(FW).read()
phys = re.search(r"enum Phys : uint8_t \{(.*?)\};", src, re.S).group(1)
phys = [t.strip() for t in phys.replace("\n", " ").split(",") if t.strip() and t.strip() != "PHYS_COUNT"]
bits = re.search(r"const uint8_t MCP_BIT_STD\[PHYS_COUNT\] = \{(.*?)\};", src, re.S).group(1)
bits = [int(x) for x in re.findall(r"\b\d+\b", re.sub(r"//[^\n]*", "", bits))]
assert len(bits) == len(phys) == 14, (phys, bits)
NAME = {"P_UP": "UP", "P_DOWN": "DOWN", "P_LEFT": "LEFT", "P_RIGHT": "RIGHT", "P_START": "START",
        "P_SELECT": "SELECT", "P_HOME": "HOME", "P_B1": "B1", "P_B2": "B2", "P_B3": "B3",
        "P_B4": "B4", "P_B5": "B5", "P_B6": "B6", "P_EX": "EX"}
for ph, bit in zip(phys, bits):
    gp = ("GPA" if bit < 8 else "GPB") + str(bit % 8)
    pin = [k for k, v in DS.items() if v == gp][0]
    net = pads[f"U1-{pin}"]
    check(net == NAME[ph], f"firmware {ph:8} = MCP bit {bit:2} ({gp}, pin {pin:2}) -> net '{net}'")

# 3. exact net membership
expect = {
    "3V3": {"U1-9", "U1-18", "J1-1", "R1-1", "R2-1", "C1-1"},
    "SCL": {"U1-12", "J1-3", "R2-2"},
    "SDA": {"U1-13", "J1-4", "R1-2"},
    "INTA": {"U1-20", "J2-2"},
    "INTB": {"U1-19", "J2-1"},
}
gnd = {"U1-10", "U1-15", "U1-16", "U1-17", "J1-2", "J2-3", "C1-2"}
for i in range(1, 8):
    gnd |= {f"JT{i}-2", f"JB{i}-2"}
expect["GND"] = gnd
for name, pin in [("SELECT", 27), ("START", 26), ("HOME", 25), ("UP", 24), ("DOWN", 23), ("RIGHT", 22), ("LEFT", 21)]:
    expect[name] = {f"U1-{pin}"}
for name, pin in [("B3", 1), ("B2", 2), ("B1", 3), ("B6", 4), ("B5", 5), ("B4", 6), ("EX", 7)]:
    expect[name] = {f"U1-{pin}"}
term_net = {}
for fp in b.GetFootprints():
    r = fp.GetReference()
    if r.startswith(("JT", "JB")):
        n = pads[f"{r}-1"]; term_net[r] = n
        expect.setdefault(n, set()).add(f"{r}-1")
actual = {}
for k, n in pads.items():
    if n:
        actual.setdefault(n, set()).add(k)
for n in sorted(set(expect) | set(actual)):
    check(expect.get(n) == actual.get(n), f"net {n:6}: {sorted(actual.get(n, []))}")
unconnected = sorted(k for k, n in pads.items() if not n and k.startswith("U1"))
check(unconnected == ["U1-11", "U1-14", "U1-28", "U1-8"], f"unused chip pins: {unconnected}")

# 4. silkscreen label beside each terminal = net on its signal pad
labels = [(pcbnew.ToMM(t.GetPosition().x), pcbnew.ToMM(t.GetPosition().y), t.GetText())
          for t in b.GetDrawings() if isinstance(t, pcbnew.PCB_TEXT) and t.GetLayer() == pcbnew.F_SilkS]
for fp in b.GetFootprints():
    r = fp.GetReference()
    if not r.startswith(("JT", "JB")):
        continue
    for num in ("1", "2"):
        p = [p for p in fp.Pads() if p.GetNumber() == num][0]
        x = pcbnew.ToMM(p.GetPosition().x)
        near = [t for (tx, ty, t) in labels if abs(tx - x) < 0.01 and (ty < 1 or ty > 43)]
        want = pads[f"{r}-{num}"]
        check(near == [want], f"{r} pad {num} label {near} = net '{want}'")

# 5. KiCad connectivity: every net fully routed
b.BuildConnectivity()
conn = b.GetConnectivity()
check(conn.GetUnconnectedCount(True) == 0, f"unrouted connections: {conn.GetUnconnectedCount(True)}")

print("\nALL CHECKS PASSED" if not fails else f"\n{len(fails)} FAILURES")
sys.exit(1 if fails else 0)
