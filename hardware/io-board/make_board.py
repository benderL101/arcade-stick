#!/usr/bin/env python3
"""Arcade Stick I/O board v1.0: generates arcade_stick_io.kicad_pcb with KiCad 7's pcbnew API.

A through-hole carrier for the MCP23017 (DIP-28, socketed) with a 5.08 mm
screw terminal (signal + GND) for every input, laid out to the firmware's
STANDARD wiring (MCP_BIT_STD in src/main.cpp):

  top row    GPA6..GPA0 : SELECT START HOME UP DOWN RIGHT LEFT
  bottom row GPB0..GPB6 : B3 B2 B1 B6 B5 B4 EX
  J1  3V3 / GND / SCL / SDA  -> Freenove I2C port
  J2  INTB / INTA / GND      (spare, for interrupt-driven reading later)
  A0-A2 to GND (address 0x20), RESET to 3V3, C1 100 nF, R1/R2 4.7k optional I2C pull-ups.
"""
import pcbnew
from pcbnew import VECTOR2I_MM as V, FromMM

FP = "/usr/share/kicad/footprints"
import os
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "arcade_stick_io.kicad_pcb")
W = 97.0
Y_MIN, Y_MAX = -2.5, 47.0   # board top/bottom edge (labels sit outside the terminal rows)

board = pcbnew.BOARD()
ds = board.GetDesignSettings()
nc = ds.m_NetSettings.m_DefaultNetClass
nc.SetClearance(FromMM(0.25))
nc.SetTrackWidth(FromMM(0.4))
nc.SetViaDrill(FromMM(0.4))
nc.SetViaDiameter(FromMM(0.8))
ds.m_TrackMinWidth = FromMM(0.2)
ds.m_MinClearance = FromMM(0.2)
ds.m_HoleToHoleMin = FromMM(0.5)
ds.m_CopperEdgeClearance = FromMM(0.5)
ds.m_MinThroughDrill = FromMM(0.3)

nets = {}
def net(name):
    if name not in nets:
        n = pcbnew.NETINFO_ITEM(board, name)
        board.Add(n)
        nets[name] = n
    return nets[name]

fps = {}
def place(lib, name, ref, x, y, rot=0, value=""):
    fp = pcbnew.FootprintLoad(f"{FP}/{lib}.pretty", name)
    assert fp, name
    fp.SetFPID(pcbnew.LIB_ID(lib, name))
    fp.SetReference(ref)
    fp.SetValue(value or name)
    fp.SetPosition(V(x, y))
    fp.SetOrientationDegrees(rot)
    fp.Value().SetVisible(False)
    board.Add(fp)
    fps[ref] = fp
    return fp

def pad(ref, num):
    for p in fps[ref].Pads():
        if p.GetNumber() == str(num):
            return p
    raise KeyError((ref, num))

def P(ref, num):
    p = pad(ref, num).GetPosition()
    return (round(pcbnew.ToMM(p.x), 3), round(pcbnew.ToMM(p.y), 3))

def setnet(ref, num, name):
    pad(ref, num).SetNet(net(name))

def track(points, name, w=0.4, layer=pcbnew.F_Cu):
    for a, b in zip(points, points[1:]):
        if a == b:
            continue
        t = pcbnew.PCB_TRACK(board)
        t.SetStart(V(*a)); t.SetEnd(V(*b))
        t.SetWidth(FromMM(w)); t.SetLayer(layer); t.SetNet(net(name))
        board.Add(t)

def text(s, x, y, size=1.0, layer=pcbnew.F_SilkS, thick=0.16, just=None):
    t = pcbnew.PCB_TEXT(board)
    t.SetText(s); t.SetPosition(V(x, y)); t.SetLayer(layer)
    t.SetTextSize(V(size, size)); t.SetTextThickness(FromMM(thick))
    if layer == pcbnew.B_SilkS:
        t.SetMirrored(True)
    if just == "left":
        t.SetHorizJustify(pcbnew.GR_TEXT_H_ALIGN_LEFT if layer != pcbnew.B_SilkS else pcbnew.GR_TEXT_H_ALIGN_RIGHT)
    board.Add(t)
    return t

def edge(pts):
    for a, b in zip(pts, pts[1:] + pts[:1]):
        s = pcbnew.PCB_SHAPE(board)
        s.SetShape(pcbnew.SHAPE_T_SEGMENT)
        s.SetStart(V(*a)); s.SetEnd(V(*b))
        s.SetLayer(pcbnew.Edge_Cuts); s.SetWidth(FromMM(0.1))
        board.Add(s)

# ---------------- outline ----------------
edge([(0, Y_MIN), (W, Y_MIN), (W, Y_MAX), (0, Y_MAX)])

# ---------------- U1: MCP23017 in a 28-pin socket ----------------
# rotated so pins 1-14 run left->right along the bottom row (y = 26.62)
# and pins 28..15 run left->right along the top row (y = 19.00)
X0, YTOP, YBOT = 34.0, 19.0, 26.62
place("Package_DIP", "DIP-28_W7.62mm_Socket_LongPads", "U1", X0, YBOT, 90, "MCP23017-E/SP")
for k in range(1, 15):
    assert P("U1", k) == (round(X0 + 2.54 * (k - 1), 3), YBOT), (k, P("U1", k))
for k in range(15, 29):
    assert P("U1", k) == (round(X0 + 2.54 * (28 - k), 3), YTOP), (k, P("U1", k))

# ---------------- screw terminals ----------------
TOP = [(27, "SELECT"), (26, "START"), (25, "HOME"), (24, "UP"), (23, "DOWN"), (22, "RIGHT"), (21, "LEFT")]
BOT = [(1, "B3"), (2, "B2"), (3, "B1"), (4, "B6"), (5, "B5"), (6, "B4"), (7, "EX")]
YT, YB = 6.0, 39.0                         # signal pad rows
TOP_LEVEL = {0: 16.0, 1: 14.5, 2: 13.0, 4: 13.0, 5: 14.5, 6: 16.0}
BOT_LEVEL = {0: 30.0, 1: 31.5, 2: 33.0, 4: 33.0, 5: 31.5, 6: 30.0}

for i, (pin, name) in enumerate(TOP):
    xs = round(X0 + 10.16 + 11 * (i - 3), 3)
    ref = f"JT{i + 1}"
    place("TerminalBlock", "TerminalBlock_bornier-2_P5.08mm", ref, xs, YT, 180, "Screw terminal 2P 5.08")
    assert P(ref, 1) == (xs, YT) and P(ref, 2) == (round(xs - 5.08, 3), YT), (ref, P(ref, 1), P(ref, 2))
    setnet(ref, 1, name); setnet(ref, 2, "GND"); setnet("U1", pin, name)
    px, py = P("U1", pin)
    if i == 3:
        assert px == xs
        track([(px, py), (xs, YT)], name)
    else:
        lv = TOP_LEVEL[i]
        track([(px, py), (px, lv), (xs, lv), (xs, YT)], name)
    fps[ref].Reference().SetVisible(False)
    text(name, xs, 0.25)                         # by the wire entry, clear of traces
    text("GND", round(xs - 5.08, 3), 0.25)

for i, (pin, name) in enumerate(BOT):
    xs = round(X0 + 7.62 + 11 * (i - 3), 3)
    ref = f"JB{i + 1}"
    place("TerminalBlock", "TerminalBlock_bornier-2_P5.08mm", ref, xs, YB, 0, "Screw terminal 2P 5.08")
    assert P(ref, 1) == (xs, YB) and P(ref, 2) == (round(xs + 5.08, 3), YB), (ref, P(ref, 1), P(ref, 2))
    setnet(ref, 1, name); setnet(ref, 2, "GND"); setnet("U1", pin, name)
    px, py = P("U1", pin)
    if i == 3:
        assert px == xs
        track([(px, py), (xs, YB)], name)
    else:
        lv = BOT_LEVEL[i]
        track([(px, py), (px, lv), (xs, lv), (xs, YB)], name)
    fps[ref].Reference().SetVisible(False)
    text(name, xs, 44.25)
    text("GND", round(xs + 5.08, 3), 44.25)

# ---------------- power, I2C, reset, address ----------------
for k in (10, 15, 16, 17):                 # VSS, A0, A1, A2 -> GND (address 0x20)
    setnet("U1", k, "GND")
setnet("U1", 9, "3V3"); setnet("U1", 18, "3V3")   # RESET tied high
setnet("U1", 12, "SCL"); setnet("U1", 13, "SDA")
setnet("U1", 19, "INTB"); setnet("U1", 20, "INTA")

JX = 91.0
place("Connector_PinHeader_2.54mm", "PinHeader_1x04_P2.54mm_Vertical", "J1", JX, 12.46, 0, "I2C to Freenove")
for k, n in enumerate(["3V3", "GND", "SCL", "SDA"], 1):
    setnet("J1", k, n)
place("Connector_PinHeader_2.54mm", "PinHeader_1x03_P2.54mm_Vertical", "J2", JX, 26.0, 0, "INT (spare)")
for k, n in enumerate(["INTB", "INTA", "GND"], 1):
    setnet("J2", k, n)
place("Resistor_THT", "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal", "R1", 84.5, 12.46, -90, "4.7k")
place("Resistor_THT", "R_Axial_DIN0207_L6.3mm_D2.5mm_P5.08mm_Vertical", "R2", 87.5, 12.46, -90, "4.7k")
place("Capacitor_THT", "C_Disc_D3.0mm_W1.6mm_P2.50mm", "C1", 74.5, 18.3, 180, "100nF")
assert P("R1", 2) == (84.5, 20.08), P("R1", 2)
assert P("R2", 2) == (87.5, 17.54), P("R2", 2)
assert P("C1", 2) == (72.0, 18.3), P("C1", 2)
setnet("R1", 1, "3V3"); setnet("R1", 2, "SDA")
setnet("R2", 1, "3V3"); setnet("R2", 2, "SCL")
setnet("C1", 1, "3V3"); setnet("C1", 2, "GND")

Y_VDD, Y_SCL, Y_SDA = 21.6, 23.0, 24.4    # levels in the gap between the chip's rows
x9, y9 = P("U1", 9); x18, y18 = P("U1", 18); x12, _ = P("U1", 12); x13, _ = P("U1", 13)
# 3V3: pin 9 up into the gap between the rows, right, up to the header (split at every tee)
track([(x9, y9), (x9, Y_VDD), (x18, Y_VDD), (74.5, Y_VDD), (79.0, Y_VDD), (79.0, 12.46),
       (84.5, 12.46), (87.5, 12.46), P("J1", 1)], "3V3", 0.5)
track([(x18, y18), (x18, Y_VDD)], "3V3", 0.5)            # RESET
track([P("U1", 17), P("U1", 16), P("U1", 15)], "GND", 0.4)   # A2-A1-A0 tied together (and to GND)
track([P("C1", 1), (74.5, Y_VDD)], "3V3", 0.5)           # decoupling cap
track([(x12, YBOT), (x12, Y_SCL), (80.5, Y_SCL), (80.5, 17.54), (87.5, 17.54), P("J1", 3)], "SCL")
track([(x13, YBOT), (x13, Y_SDA), (82.0, Y_SDA), (82.0, 20.08), (84.5, 20.08), P("J1", 4)], "SDA")
# spare interrupt lines on the back
x19, _ = P("U1", 19); x20, _ = P("U1", 20)
track([(x19, YTOP), (x19, 21.7), (87.0, 21.7), (87.0, 26.0), P("J2", 1)], "INTB", 0.4, pcbnew.B_Cu)
track([(x20, YTOP), (x20, 23.4), (85.5, 23.4), (85.5, 28.54), P("J2", 2)], "INTA", 0.4, pcbnew.B_Cu)

for r in ("J1", "J2", "C1", "R1", "R2"):
    fps[r].Reference().SetVisible(False)
text("R1", 84.5, 16.27, 1.0).SetTextAngleDegrees(90)
text("R2", 88.65, 15.4, 1.0)
text("C1", 69.8, 18.3, 1.0)
text("J2", JX - 2.4, 31.08, 1.0)
fps["U1"].Reference().SetVisible(False)
text("U1  MCP23017", 44.0, 22.81, 1.0)                 # inside the socket outline; notch = pin 1 end

# ---------------- mounting holes (M3) ----------------
place("MountingHole", "MountingHole_3.2mm_M3", "H1", 5.0, 23.0)
place("MountingHole", "MountingHole_3.2mm_M3", "H2", 91.5, 5.0)
place("MountingHole", "MountingHole_3.2mm_M3", "H3", 91.5, 40.0)
for h in ("H1", "H2", "H3"):
    fps[h].Reference().SetVisible(False)

# ---------------- silkscreen ----------------
text("ARCADE STICK I/O", 20.0, 19.8, 1.5, thick=0.22)
text("v1.0  MCP23017 @ 0x20", 20.0, 22.2, 1.0)
text("buttons:  B1 B2 B3", 20.0, 24.4, 1.0)
text("B4 B5 B6", 23.3, 26.3, 1.0)
for y, s in zip([12.46, 15.0, 17.54, 20.08], ["3V3", "GND", "SCL", "SDA"]):
    text(s, 92.6, y, 1.0, just="left")
for y, s in zip([26.0, 28.54, 31.08], ["INTB", "INTA", "GND"]):
    text(s, 92.6, y, 1.0, just="left")
text("FREENOVE", JX - 0.8, 8.6, 1.0)
text("I2C", JX - 0.8, 10.1, 1.0)

back = [  # (text, y, size): pad-free bands above and below the chip
    ("ARCADE STICK I/O BOARD v1.0", 10.0, 1.2),
    ("github.com/benderL101/arcade-stick", 12.3, 1.0),
    ("Firmware: Menu > Wiring > Standard", 14.3, 1.0),
    ("J1 -> Freenove I2C port: 3V3 GND SCL SDA", 29.4, 1.0),
    ("U1 MCP23017-E/SP in 28-pin socket", 31.2, 1.0),
    ("C1 100nF   R1 R2 4.7k (optional I2C pull-ups)", 33.0, 1.0),
    ("Each terminal: signal + GND", 34.8, 1.0),
]
for s_, y, sz in back:
    text(s_, 46.0, y, sz, pcbnew.B_SilkS)

# ---------------- ground pours (both layers) ----------------
for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
    z = pcbnew.ZONE(board)
    z.SetLayer(layer)
    z.SetNet(net("GND"))
    o = z.Outline(); o.NewOutline()
    for x, y in [(0.3, Y_MIN + 0.3), (W - 0.3, Y_MIN + 0.3), (W - 0.3, Y_MAX - 0.3), (0.3, Y_MAX - 0.3)]:
        o.Append(FromMM(x), FromMM(y))
    z.SetLocalClearance(FromMM(0.3))
    z.SetMinThickness(FromMM(0.25))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
    z.SetThermalReliefGap(FromMM(0.4))
    z.SetThermalReliefSpokeWidth(FromMM(0.5))
    z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
    board.Add(z)

board.Save(OUT)
print("saved", OUT)
