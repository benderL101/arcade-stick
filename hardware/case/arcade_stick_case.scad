// =====================================================================
//  Arcade Stick case v2: Sanwa JLF + 6 x 30 mm (Vewlix) + 3 x 24 mm,
//  Freenove FNK0104 2.8" screen, I/O board v1.0, 1S LiPo.
//
//  310 x 220 x 55 mm. The top prints in two halves, upside down (top
//  face on the bed), so every edge is a 45-degree chamfer and nothing
//  needs supports. The base plate prints in two halves too; its seam is
//  offset from the top's, so the screwed-on base ties the halves together.
//
//  Export one part at a time:  openscad -D 'part="top_left"' -o top_left.stl arcade_stick_case.scad
//  Parts: top_left, top_right, accent_left, accent_right, base_left, base_right,
//         splice, screen_clip, assembly (preview only)
//  Origin: joystick centre, z = 0 at the bottom of the case. +y is away from you.
// =====================================================================

part = "assembly";
$fn = 72;

// ---------- overall shape ----------
X0 = -95;  X1 = 215;     // left / right outside edges
Y0 = -105; Y1 = 115;     // front (wrist side) / back
H  = 55;                 // total height
R  = 16;                 // corner radius, seen from above
W  = 3;                  // wall thickness
T  = 4;                  // top panel thickness
C  = 4;                  // 45-degree chamfer on the top edge (back, left, right)
BEV = 12;                // bigger 45-degree palm bevel on the front edge
CB = 1.5;                // small chamfer on the bottom edge
BASE = 3;                // base plate thickness
FIT = 0.3;               // clearance for parts that slot together

SPLIT_X = 34;            // where the top is cut into two printable halves
BASE_SPLIT_X = 120;      // where the base plate is cut (offset on purpose)
LAP = 10;                // half-lap overlap at the base seam

// ---------- controls ----------
// Sanwa JLF on a JLF-P1 plate (95 x 53 mm, mounting slots 84 x 40 mm),
// plate turned so its long side runs front to back, clear of B4.
JLF_HOLE = 24;
JLF_SCREWS = [[20, 42], [-20, 42], [20, -42], [-20, -42]];   // M4 countersunk
JLF_PLATE = [53, 95];
// 30 mm buttons, Vewlix, relative to the stick (same as the firmware's screen layout)
B30 = [[66, 10], [99, 24], [135, 24], [59, -28], [92, -15], [128, -15]];
// 24 mm buttons along the back: SELECT, HOME, START
B24 = [[-54, 88], [-22, 88], [10, 88]];
HOLE30 = 30.3;  CBORE30 = 35;     // Sanwa OBSF-30 snaps into 2-3.9 mm panels
HOLE24 = 24.3;  CBORE24 = 30;
SNAP = 3.0;                       // panel thickness left at the button clips

// ---------- screen: Freenove FNK0104 (ES3N28P) 50 x 86 x 4.4 mm ----------
// Glass 50 x 69.2 at one end of the board, USB-C on the other end.
// Visible area 57.6 x 43.2, assumed centred on the glass.
SCR_L = 86; SCR_W = 50; SCR_D = 4.4; GLASS_L = 69.2;
VIS_L = 57.6; VIS_W = 43.2;
SCR_X0 = X1 - W - FIT - SCR_L;   // USB end against the right wall
SCR_Y0 = 50;
SCR_VIS_SHIFT = 0;               // move the window along the board if your glass sits differently
USB_Y = SCR_Y0 + SCR_W / 2;      // USB-C position along the board's short edge
USB_Z = H - T - SCR_D - 1.6;
// clips press on the board's corners, clear of the connectors along its long edges
CLIP_BOSSES = [[SCR_X0 + 3, SCR_Y0 - 4.5], [X1 - W - 8, SCR_Y0 - 4.5],
               [SCR_X0 + 3, SCR_Y0 + SCR_W + 4.5], [X1 - W - 8, SCR_Y0 + SCR_W + 4.5]];

// ---------- speaker (the one that came with the board) ----------
SPK = [75, 83];  SPK_D = 28;     // grille centre and speaker diameter

// ---------- I/O board v1.0 (97 x 49.5 mm, holes from the KiCad file) ----------
IO_X = 48; IO_Y = -51;           // board origin; KiCad y runs the other way
IO_HOLES = [[5, 23], [91.5, 5], [91.5, 40]];
IO_STANDOFF = 5;

// ---------- battery bay (1S LiPo) ----------
BAT = [-82, -40, -30, 40];       // x0, y0, x1, y1 inside the low walls

// ---------- screws ----------
// Base plate: M3 countersunk into 2.5 mm pilot holes (or M3 heat-set inserts: make them 4.0).
BASE_BOSSES = [[-83, -93], [-89, 0], [-83, 103], [26, -96], [42, -96], [26, 106],
               [42, 106], [120, 106], [175, -96], [203, -93], [206, 10], [180, 106]];
PILOT = 2.5;
FEET = [[-68, -80], [-68, 90], [190, -80], [190, 90]];
// Splices under the top panel, across the seam (M3 x 10 pan head)
SPLICES = [[SPLIT_X, -75], [SPLIT_X, 62]];
SPLICE = [40, 18, 4];
SPLICE_HOLES = [[-12, -4], [-12, 4], [12, -4], [12, 4]];
PAD = 6;                          // splice pads below the panel
// 1.75 mm filament as alignment pins across the seam
PIN_D = 2.0; PIN_L = 6;
PANEL_PINS = [-85, -60, -20, 20, 50, 100];
WALL_PINS = [[Y0 + W / 2, 20], [Y0 + W / 2, 32], [Y1 - W / 2, 15], [Y1 - W / 2, 35]];

// ---------- looks ----------
ACCENT = 0.6;                     // depth of the second-colour inlays (3 layers at 0.2 mm)
FIELD = [-42, -60, 162, 52, 24];  // play-area panel: x0, y0, x1, y1, radius
LOGO = "ARCADE STICK";
GRIP = [90, 9];                   // finger scoops under the left and right sides: length, height

// =====================================================================
S2 = sqrt(2);
module rr(x0, y0, x1, y1, r) {
  rr_ = max(0.5, min(r, (x1 - x0) / 2 - 0.01, (y1 - y0) / 2 - 0.01));
  translate([x0 + rr_, y0 + rr_]) offset(r = rr_) square([x1 - x0 - 2 * rr_, y1 - y0 - 2 * rr_]);
}
// footprint inset by s on the back/left/right and f on the front
module outline(s, f) { rr(X0 + s, Y0 + f, X1 - s, Y1 - s, R - s); }
module slab(z, s, f) { translate([0, 0, z]) linear_extrude(0.01) outline(s, f); }

module outer() {
  hull() {
    slab(0, CB, CB);
    slab(CB, 0, 0);
    slab(H - BEV, 0, 0);
    slab(H - C, 0, BEV - C);
    slab(H - 0.01, C, BEV);
  }
}
function side_in(z)  = max(W, z - (H - C) + W * S2);
function front_in(z) = max(W, z - (H - BEV) + W * S2);
module cavity() {
  hull() {
    slab(-1, W, W);
    slab(H - BEV + W - W * S2, W, W);
    slab(H - C + W - W * S2, W, front_in(H - C + W - W * S2));
    slab(H - T - 0.01, side_in(H - T), front_in(H - T));
  }
}

// ---------- top shell ----------
module panel_cuts() {
  ztop = H - T;
  // joystick: shaft hole, chamfered on top, and countersunk M4 screws
  translate([0, 0, ztop - 1]) cylinder(d = JLF_HOLE, h = T + 2);
  translate([0, 0, H - 1]) cylinder(d1 = JLF_HOLE, d2 = JLF_HOLE + 4, h = 2.01);
  for (p = JLF_SCREWS) translate([p[0], p[1], 0]) {
    translate([0, 0, ztop - 1]) cylinder(d = 4.5, h = T + 2);
    translate([0, 0, H - 2.2]) cylinder(d1 = 4.5, d2 = 9, h = 2.21);
  }
  for (p = B30) button_hole(p, HOLE30, CBORE30);
  for (p = B24) button_hole(p, HOLE24, CBORE24);
  // screen window with a 45-degree bevel
  vx = SCR_X0 + GLASS_L / 2 + SCR_VIS_SHIFT; vy = SCR_Y0 + SCR_W / 2;
  translate([vx, vy, ztop - 1]) linear_extrude(T + 2) rr(-VIS_L / 2 - 0.5, -VIS_W / 2 - 0.5, VIS_L / 2 + 0.5, VIS_W / 2 + 0.5, 1.5);
  translate([vx, vy, H - 2]) hull() {
    linear_extrude(0.01) rr(-VIS_L / 2 - 0.5, -VIS_W / 2 - 0.5, VIS_L / 2 + 0.5, VIS_W / 2 + 0.5, 1.5);
    translate([0, 0, 2.01]) linear_extrude(0.01) rr(-VIS_L / 2 - 2.5, -VIS_W / 2 - 2.5, VIS_L / 2 + 2.5, VIS_W / 2 + 2.5, 3.5);
  }
  // speaker grille: slots clipped to a circle
  translate([SPK[0], SPK[1], ztop - 1]) linear_extrude(T + 2) intersection() {
    circle(d = SPK_D - 6);
    for (i = [-3:3]) translate([i * 3.6 - 1.1, -20]) square([2.2, 40]);
  }
  // notch so the board's end can sit right against the wall (under the chamfer)
  translate([SCR_X0, SCR_Y0 - FIT, ztop - SCR_D - 0.5]) cube([X1 - W - SCR_X0, SCR_W + 2 * FIT, SCR_D + 0.5 + 0.01]);
  // USB-C through the right wall, with a recess for the plug's moulding
  translate([X1 - W - 1, USB_Y, USB_Z]) rotate([0, 90, 0]) linear_extrude(W + 2) rr(-3.75, -6.5, 3.75, 6.5, 2);
  translate([X1 - 1.8, USB_Y, USB_Z]) rotate([0, 90, 0]) linear_extrude(3) rr(-5.5, -8.5, 5.5, 8.5, 3);
}
module button_hole(p, d, cb) {
  translate([p[0], p[1], 0]) {
    translate([0, 0, H - T - 1]) cylinder(d = d, h = T + 2);
    translate([0, 0, H - 0.6]) cylinder(d1 = d, d2 = d + 1.2, h = 0.61);
    translate([0, 0, H - T - 10]) cylinder(d = cb, h = 10 + T - SNAP);
  }
}

module inside_parts() {
  ztop = H - T;
  // base screw bosses
  for (p = BASE_BOSSES) translate([p[0], p[1], BASE]) cylinder(d = 8, h = ztop - BASE + 0.1);
  // splice pads under the panel
  for (s = SPLICES) translate([s[0] - SPLICE[0] / 2 - 2, s[1] - SPLICE[1] / 2 - 2, ztop - PAD])
    cube([SPLICE[0] + 4, SPLICE[1] + 4, PAD + 0.1]);
  // screen: locating ribs round the board (open at the USB end) and clip bosses
  translate([0, 0, ztop - 3]) linear_extrude(3.1) difference() {
    translate([SCR_X0 - FIT - 1.2, SCR_Y0 - FIT - 1.2]) square([SCR_L + 2 * FIT + 1.2 + 4, SCR_W + 2 * FIT + 2.4]);
    translate([SCR_X0 - FIT, SCR_Y0 - FIT]) square([SCR_L + 2 * FIT + 10, SCR_W + 2 * FIT]);
  }
  for (p = CLIP_BOSSES) translate([p[0], p[1], ztop - SCR_D]) cylinder(d = 7, h = SCR_D + 0.1);
  // speaker ring
  translate([SPK[0], SPK[1], ztop - 5]) difference() {
    cylinder(d = SPK_D + 5, h = 5.1);
    translate([0, 0, -1]) cylinder(d = SPK_D + 0.6, h = 7.2);
  }
  // pin bosses at the wall seam
  for (w = WALL_PINS) translate([SPLIT_X - 5, w[0] < 0 ? Y0 + W - 0.1 : Y1 - W - 5, w[1] - 5]) cube([10, 5.1, 10]);
  // thicker wall behind the finger scoops
  for (x = [X0 + W - 0.1, X1 - W - 8]) translate([x, (Y0 + Y1) / 2 - GRIP[0] / 2 - 2, BASE]) cube([8.1, GRIP[0] + 4, GRIP[1] + 3]);
}
module inside_holes() {
  ztop = H - T;
  for (p = BASE_BOSSES) translate([p[0], p[1], BASE - 1]) cylinder(d = PILOT, h = 13);
  for (s = SPLICES, h = SPLICE_HOLES) translate([s[0] + h[0], s[1] + h[1], ztop - PAD - 1]) cylinder(d = PILOT, h = PAD + 2);
  for (p = CLIP_BOSSES) translate([p[0], p[1], ztop - SCR_D - 1]) cylinder(d = PILOT, h = SCR_D + 3);
}

module seam_pins() {
  for (y = PANEL_PINS) translate([SPLIT_X - PIN_L, y, H - T / 2]) rotate([0, 90, 0]) cylinder(d = PIN_D, h = 2 * PIN_L, $fn = 16);
  for (w = WALL_PINS) translate([SPLIT_X - PIN_L, w[0] < 0 ? Y0 + W + 2.5 : Y1 - W - 2.5, w[1]])
    rotate([0, 90, 0]) cylinder(d = PIN_D, h = 2 * PIN_L, $fn = 16);
}

module grips() {   // finger scoops under the left and right sides, 6 mm deep
  for (x = [X0, X1]) translate([x, (Y0 + Y1) / 2, 0]) scale([6, GRIP[0] / 2, GRIP[1]]) sphere(r = 1, $fn = 48);
}

module accent_shape() {
  vx = SCR_X0 + GLASS_L / 2 + SCR_VIS_SHIFT;
  rr(FIELD[0], FIELD[1], FIELD[2], FIELD[3], FIELD[4]);
  translate([vx, Y1 - 9]) text(LOGO, size = 4.6, font = "Liberation Sans:style=Bold", halign = "center", valign = "center", spacing = 1.15);
}
module accent_volume() { translate([0, 0, H - ACCENT]) linear_extrude(ACCENT + 1) accent_shape(); }

module top_shell() {
  difference() {
    union() {
      difference() { outer(); cavity(); }
      intersection() { cavity(); inside_parts(); }
    }
    panel_cuts();
    inside_holes();
    seam_pins();
    grips();
    // design line along the seam, so the joint reads as deliberate
    translate([SPLIT_X - 0.5, Y0 - 1, H - 0.5]) cube([1, Y1 - Y0 + 2, 1]);
  }
}
module half(left) {
  intersection() {
    children();
    translate([left ? X0 - 10 : SPLIT_X, Y0 - 10, -10]) cube([left ? SPLIT_X - X0 + 10 : X1 - SPLIT_X + 10, Y1 - Y0 + 20, H + 20]);
  }
}

// ---------- base plate ----------
module base_plate() {
  difference() {
    union() {
      translate([0, 0, 0]) linear_extrude(BASE) outline(W + FIT, W + FIT);
      // I/O board standoffs
      for (h = IO_HOLES) translate([IO_X + h[0], IO_Y - h[1], 0]) cylinder(d = 7, h = BASE + IO_STANDOFF);
      // battery bay walls
      translate([0, 0, BASE - 0.1]) linear_extrude(6.1) difference() {
        rr(BAT[0] - 2, BAT[1] - 2, BAT[2] + 2, BAT[3] + 2, 4);
        rr(BAT[0], BAT[1], BAT[2], BAT[3], 2);
        translate([BAT[0] + 10, BAT[1] - 5]) square([BAT[2] - BAT[0] - 20, BAT[3] - BAT[1] + 10]);   // wire / strap gaps
      }
    }
    for (p = BASE_BOSSES) translate([p[0], p[1], -1]) {
      cylinder(d = 3.4, h = BASE + 2);
      cylinder(d1 = 7, d2 = 3.4, h = 1.8 + 1);
    }
    for (h = IO_HOLES) translate([IO_X + h[0], IO_Y - h[1], -1]) cylinder(d = PILOT, h = BASE + IO_STANDOFF + 2);
    for (f = FEET) translate([f[0], f[1], -1]) cylinder(d = 14, h = 1 + 1.2);
    // vents under the battery and the I/O board
    for (i = [0:5]) translate([BAT[0] + 8 + i * 7, -25, -1]) linear_extrude(BASE + 2) rr(0, 0, 3, 50, 1.5);
    translate([40, 0, -1]) linear_extrude(0.6 + 1) mirror([1, 0]) text("ARCADE STICK", size = 9, font = "Liberation Sans:style=Bold", halign = "center", valign = "center");
  }
}
module base_half(left) {
  // half-lap: the left piece keeps the lower half of the overlap, the right the upper
  difference() {
    intersection() {
      base_plate();
      union() {
        translate([left ? X0 - 10 : BASE_SPLIT_X - LAP / 2, Y0 - 10, -10])
          cube([left ? BASE_SPLIT_X + LAP / 2 - X0 + 10 : X1 - BASE_SPLIT_X + LAP / 2 + 10, Y1 - Y0 + 20, 40]);
      }
    }
    if (left)  translate([BASE_SPLIT_X - LAP / 2, Y0 - 10, BASE / 2]) cube([LAP + 1, Y1 - Y0 + 20, BASE]);
    else       translate([BASE_SPLIT_X - LAP / 2 - 1, Y0 - 10, -1]) cube([LAP + 1, Y1 - Y0 + 20, BASE / 2 + 1]);
  }
}

// ---------- small parts ----------
module splice() {
  difference() {
    translate([-SPLICE[0] / 2, -SPLICE[1] / 2, 0]) linear_extrude(SPLICE[2]) rr(0, 0, SPLICE[0], SPLICE[1], 3);
    for (h = SPLICE_HOLES) translate([h[0], h[1], -1]) cylinder(d = 3.4, h = SPLICE[2] + 2);
  }
}
module screen_clip() {
  difference() {
    linear_extrude(3) rr(-4, -4, 4, 8, 3);   // reaches 3 mm over the board's corner
    translate([0, 0, -1]) cylinder(d = 3.4, h = 5);
  }
}

// ---------- preview ----------
module assembly() {
  color([0.17,0.18,0.22]) render() difference() { top_shell(); accent_volume(); }
  color([0.85,0.2,0.23]) render() intersection() { top_shell(); accent_volume(); }
  color([0.11,0.12,0.15]) render() translate([0, 0, -0.01]) base_plate();
  // stand-ins for the parts
  color([0.07,0.07,0.07]) translate([0, 0, H]) cylinder(d = 30, h = 1.5);
  color([0.13,0.13,0.13]) translate([0, 0, H]) cylinder(d = 8, h = 32);
  color([0.85,0.2,0.23]) translate([0, 0, H + 32 + 15]) sphere(d = 35);
  for (p = B30) translate([p[0], p[1], H]) { color([0.13,0.13,0.13]) cylinder(d = 33, h = 1.2); color([0.95,0.95,0.95]) cylinder(d = 24, h = 7); }
  for (p = B24) translate([p[0], p[1], H]) { color([0.13,0.13,0.13]) cylinder(d = 27, h = 1.2); color([0.95,0.95,0.95]) cylinder(d = 19, h = 6); }
  color([0.05,0.17,0.3]) translate([SCR_X0 + GLASS_L / 2 - VIS_L / 2, SCR_Y0 + SCR_W / 2 - VIS_W / 2, H - T]) cube([VIS_L, VIS_W, 0.5]);
}

module face_down() { translate([0, 0, H]) rotate([0, 180, 0]) children(); }

// ---------- fit check ----------
// Rough keep-out volumes for the parts inside. export.sh renders
// part="check_N"; each must come out empty (nothing touching the case).
module keepout(i) {
  e = 0.05; zt = H - T - e;
  if (i == 0) translate([-JLF_PLATE[0] / 2, -JLF_PLATE[1] / 2, zt - 1.6]) cube([JLF_PLATE[0], JLF_PLATE[1], 1.6]);  // JLF plate
  if (i == 1) translate([-26, -26, zt - 1.6 - 37]) cube([52, 52, 37]);                                               // JLF body
  if (i == 2) for (p = B30) translate([p[0], p[1], zt - 38]) cylinder(d = 33, h = 38);                              // OBSF-30
  if (i == 3) for (p = B24) translate([p[0], p[1], zt - 32]) cylinder(d = 26, h = 32);                              // OBSF-24
  if (i == 4) translate([SCR_X0 + e, SCR_Y0 + e, zt - SCR_D]) cube([SCR_L, SCR_W - 2 * e, SCR_D]);                 // screen
  if (i == 5) translate([IO_X, IO_Y - 47, BASE + IO_STANDOFF + e]) cube([97, 49.5, 14]);                          // I/O board
  if (i == 6) translate([BAT[0] + e, BAT[1] + e, BASE + e]) cube([BAT[2] - BAT[0] - 2 * e, BAT[3] - BAT[1] - 2 * e, 12]); // battery
  if (i == 7) translate([SPK[0], SPK[1], zt - 10]) cylinder(d = SPK_D, h = 10);                                    // speaker
}
module check(i) {
  // the I/O board and battery sit on the base, so only the top is checked for them
  intersection() {
    union() { top_shell(); if (i != 5 && i != 6) base_plate(); }
    keepout(i);
  }
}

if (part == "assembly") assembly();
else if (part == "check_0") check(0);
else if (part == "check_1") check(1);
else if (part == "check_2") check(2);
else if (part == "check_3") check(3);
else if (part == "check_4") check(4);
else if (part == "check_5") check(5);
else if (part == "check_6") check(6);
else if (part == "check_7") check(7);
// the top halves and their accents are exported face down, ready to print
else if (part == "top_left")     face_down() difference() { half(true)  top_shell(); accent_volume(); }
else if (part == "top_right")    face_down() difference() { half(false) top_shell(); accent_volume(); }
else if (part == "accent_left")  face_down() intersection() { half(true)  top_shell(); accent_volume(); }
else if (part == "accent_right") face_down() intersection() { half(false) top_shell(); accent_volume(); }
else if (part == "base_left")    base_half(true);
else if (part == "base_right")   base_half(false);
else if (part == "splice")       splice();
else if (part == "screen_clip")  screen_clip();
