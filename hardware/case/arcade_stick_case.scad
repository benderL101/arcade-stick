// =====================================================================
//  Arcade Stick case v2: Sanwa JLF + 6 x 30 mm (Vewlix) + 3 x 24 mm,
//  Freenove FNK0104 2.8" screen, I/O board v1.0, 1S LiPo.
//
//  300 x 227 x 55 mm. The top prints in two halves, upside down (top
//  face on the bed), so no surface is shallower than 45 degrees and nothing
//  needs supports. The front edge is a rounded palm rest. The base plate
//  prints in two halves too; its seam is offset from the top's, so the
//  screwed-on base ties the halves together.
//
//  Export one part at a time:  openscad -D 'part="top_left"' -o top_left.stl arcade_stick_case.scad
//  Parts: top_left, top_right, accent_left, accent_right, base_left, base_right,
//         splice, screen_clip, assembly (preview only)
//  Origin: 10 mm behind the joystick's centre (STICK), z = 0 at the bottom of the case. +y is away from you.
// =====================================================================

part = "assembly";
$fn = 72;

// ---------- overall shape ----------
X0 = -85;  X1 = 215;     // left / right outside edges
Y0 = -115; Y1 = 112;     // front (wrist side) / back: 105 mm of palm room in front of the stick
H  = 55;                 // total height
R  = 16;                 // corner radius, seen from above
W  = 3;                  // wall thickness
T  = 4;                  // top panel thickness
C  = 4;                  // 45-degree chamfer on the top edge (back, left, right)
// Front palm rest: the front wall rolls into the top with a big radius,
// then a 45-degree run, then a small radius onto the flat top. It's the
// roundest shape that still prints face down without supports, and keeps
// any sharp edge away from your wrists and forearms.
PALM = 18;               // the flat top starts this far back from the front
PALM_R = 20;             // radius where the front wall meets the slope
PALM_RT = 2.5;           // radius where the slope meets the flat top
CB = 1.5;                // small chamfer on the bottom edge
BASE = 4;                // base plate thickness (3 mm under the anti-slip mat)
FIT = 0.3;               // clearance for parts that slot together

SPLIT_X = 34;            // where the top is cut into two printable halves
BASE_SPLIT_X = 120;      // where the base plate is cut (offset on purpose)
LAP = 10;                // half-lap overlap at the base seam

// ---------- controls ----------
// Sanwa JLF on a JLF-P1 plate (95 x 53 mm, mounting slots 84 x 40 mm),
// plate turned so its long side runs front to back, clear of B4.
// The stick and the six main buttons sit 10 mm forward of the old
// origin, leaving a clear gap below the screen and the 24 mm row.
CY = -10;
STICK = [0, CY];
JLF_HOLE = 24;
JLF_SCREWS = [for (p = [[20, 42], [-20, 42], [20, -42], [-20, -42]]) [p[0], p[1] + CY]];   // M4 countersunk
JLF_PLATE = [53, 95];
// 30 mm buttons, Vewlix, relative to the stick (same as the firmware's screen layout)
B30 = [for (p = [[66, 10], [99, 24], [135, 24], [59, -28], [92, -15], [128, -15]]) [p[0], p[1] + CY]];
// 24 mm buttons along the back, behind the main buttons and well away from
// the stick hand, so they don't get knocked mid-game: SELECT, HOME, START
B24 = [[49, 88], [78.5, 88], [108, 88]];
HOLE30 = 30.3;  CBORE30 = 35;     // Sanwa OBSF-30 snaps into 2-3.9 mm panels
HOLE24 = 24.3;  CBORE24 = 29;
SNAP = 3.0;                       // panel thickness left at the button clips

// ---------- screen: Freenove FNK0104, measured by Ben ----------
// Board 85 x 50, 4 mm from the glass to the back of the board, 8 mm over the
// connectors. Lit picture 60 x 45, centred across the board, its centre
// 38.6 mm from the MIC end. USB-C is on the back at the other (ribbon) end.
SCR_L = 85; SCR_W = 50; SCR_D = 4; SCR_PARTS = 8;
VIS_L = 59; VIS_W = 44;          // window = lit area (59 + 2 x 0.5 margin = 60)
VIS_X = 38.6;                    // picture centre, from the board's MIC end
SCR_X0 = X1 - W - FIT - SCR_L;   // MIC end on the left, USB end against the right wall
SCR_Y0 = 46;                     // clear of the rounded back-right corner
USB_Y = SCR_Y0 + SCR_W / 2;      // USB-C sits about mid-edge (the slot allows +/- 3.5 mm)
USB_Z = H - T - SCR_D - 1.6;
// clips press on the board's corners, clear of the connectors along its long edges
CLIP_BOSSES = [[SCR_X0 + 3, SCR_Y0 - 4.5], [X1 - W - 8, SCR_Y0 - 4.5],
               [SCR_X0 + 3, SCR_Y0 + SCR_W + 4.5], [X1 - W - 8, SCR_Y0 + SCR_W + 4.5]];

// ---------- speaker (the one that came with the board) ----------
SPK = [-45, 82];                 // grille centre, behind the stick
SPK_L = 44; SPK_W = 28; SPK_H = 9;   // rounded-rectangle speaker, measured by Ben
module spk_shape(grow) { rr(-SPK_L / 2 - grow, -SPK_W / 2 - grow, SPK_L / 2 + grow, SPK_W / 2 + grow, 8 + grow); }

// ---------- I/O board v1.0 (97 x 49.5 mm, holes from the KiCad file) ----------
IO_X = 48; IO_Y = -61;           // board origin; KiCad y runs the other way
IO_HOLES = [[5, 23], [91.5, 5], [91.5, 40]];
IO_STANDOFF = 5;

// ---------- battery bay (1S LiPo) ----------
// 585460 LiPo (5.8 x 54 x 60 mm, 2000 mAh), front left under your palm
BAT = [-66, -108, -8, -44];      // x0, y0, x1, y1 inside the low walls

// ---------- weight trays ----------
// A heavier stick stays put on your lap. Fill these with steel washers or
// fishing sinkers (glue or hot-glue them in).
WEIGHTS = [[-70, -38, -30, 40], [157, -40, 200, 40]];     // x0, y0, x1, y1 inside
WEIGHT_H = 8;

// ---------- anti-slip mat ----------
// The base has a 1 mm recess for one big pad of 2 mm neoprene / EVA foam
// (an old mouse mat works), instead of four small feet: it grips a desk and
// sits softly on your lap.
MAT_RIM = 6;                      // plastic rim left round the mat
MAT_DEPTH = 1;

// ---------- screws ----------
// Base plate: M3 countersunk into 2.5 mm pilot holes (or M3 heat-set inserts: make them 4.0).
BASE_BOSSES = [[-73, -103], [-79, 0], [-75, 100], [26, -106], [42, -106], [26, 106],
               [42, 106], [120, 106], [175, -106], [203, -103], [206, 10], [180, 106]];
PILOT = 2.5;
// Splices under the top panel, across the seam (M3 x 10 pan head)
SPLICES = [[SPLIT_X, -75], [SPLIT_X, 62]];
SPLICE = [40, 18, 4];
SPLICE_HOLES = [[-12, -4], [-12, 4], [12, -4], [12, 4]];
PAD = 6;                          // splice pads below the panel
// 1.75 mm filament as alignment pins across the seam
PIN_D = 2.0; PIN_L = 6;
PANEL_PINS = [-90, -60, -20, 20, 50, 104];
WALL_PINS = [[Y0 + W / 2, 20], [Y0 + W / 2, 32], [Y1 - W / 2, 15], [Y1 - W / 2, 35]];

// ---------- looks ----------
ACCENT = 0.6;                     // depth of the second-colour inlays (3 layers at 0.2 mm)
// Second-colour design: rings round the stick and every button, a frame
// round the screen, a pair of racing stripes down each side and the name
// in front of the buttons.
LINE = 2;                         // width of the outlines and rings
RING30 = 17.5; RING24 = 14; RING_STICK = 21;   // inner radius of each ring
SCR_FRAME = 3;                    // gap between the window's bevel and its frame
STRIPES = [[8, 0], [3, 11]];      // racing stripes: width, offset in from the side
STRIPE_X = 7;                     // stripes start this far in from the top's flat edge
STRIPE_END = [55, 30];            // where the left / right pair stop (y), cut at 45 degrees
LOGO = "ARCADE STICK";
LOGO_AT = [100, -88]; LOGO_SIZE = 6;
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

// Front profile as [inset from the front, height] points. ZA is where the
// front wall starts to roll over; it makes the run between the two radii 45 degrees.
ZA = H - PALM - (S2 - 1) * (PALM_R - PALM_RT);
function front_profile(off) = concat(
  [[off, -1], [off, ZA]],
  [for (a = [7.5:7.5:45]) [PALM_R - (PALM_R - off) * cos(a), ZA + (PALM_R - off) * sin(a)]],
  [for (a = [45:7.5:90]) [PALM - (PALM_RT - off) * cos(a), H - PALM_RT + (PALM_RT - off) * sin(a)]]);

// The outside is the overlap of two shapes: one carries the front's palm
// rest, the other the chamfer on the back and sides.
module outer() {
  intersection() {
    hull() {
      slab(0, CB, CB);
      for (p = front_profile(0)) if (p[1] >= CB) slab(p[1] - (p[1] >= H ? 0.01 : 0), 0, p[0]);
    }
    hull() {
      slab(0, CB, CB);
      slab(CB, 0, 0);
      slab(H - C, 0, 0);
      slab(H - 0.01, C, 0);
    }
  }
}
function side_in(z) = max(W, z - (H - C) + W * S2);
module cavity() {
  intersection() {
    hull() for (p = front_profile(W)) slab(p[1], W, p[0]);
    hull() {
      slab(-1, W, W);
      slab(H - C + W - W * S2, W, W);
      slab(H - T, side_in(H - T), W);
    }
    translate([X0, Y0, -2]) cube([X1 - X0, Y1 - Y0, H - T + 2]);
  }
}

// ---------- top shell ----------
module panel_cuts() {
  ztop = H - T;
  // joystick: shaft hole, chamfered on top, and countersunk M4 screws
  translate([STICK[0], STICK[1], ztop - 1]) cylinder(d = JLF_HOLE, h = T + 2);
  translate([STICK[0], STICK[1], H - 1]) cylinder(d1 = JLF_HOLE, d2 = JLF_HOLE + 4, h = 2.01);
  for (p = JLF_SCREWS) translate([p[0], p[1], 0]) {
    translate([0, 0, ztop - 1]) cylinder(d = 4.5, h = T + 2);
    translate([0, 0, H - 2.2]) cylinder(d1 = 4.5, d2 = 9, h = 2.21);
  }
  for (p = B30) button_hole(p, HOLE30, CBORE30);
  for (p = B24) button_hole(p, HOLE24, CBORE24);
  // screen window with a 45-degree bevel
  vx = SCR_X0 + VIS_X; vy = SCR_Y0 + SCR_W / 2;
  translate([vx, vy, ztop - 1]) linear_extrude(T + 2) rr(-VIS_L / 2 - 0.5, -VIS_W / 2 - 0.5, VIS_L / 2 + 0.5, VIS_W / 2 + 0.5, 1.5);
  translate([vx, vy, H - 2]) hull() {
    linear_extrude(0.01) rr(-VIS_L / 2 - 0.5, -VIS_W / 2 - 0.5, VIS_L / 2 + 0.5, VIS_W / 2 + 0.5, 1.5);
    translate([0, 0, 2.01]) linear_extrude(0.01) rr(-VIS_L / 2 - 2.5, -VIS_W / 2 - 2.5, VIS_L / 2 + 2.5, VIS_W / 2 + 2.5, 3.5);
  }
  // speaker grille: slots clipped to the speaker's shape
  translate([SPK[0], SPK[1], ztop - 1]) linear_extrude(T + 2) intersection() {
    spk_shape(-4);
    for (i = [-5:5]) translate([i * 3.6 - 1.1, -20]) square([2.2, 40]);
  }
  // notch so the board's end can sit right against the wall (under the chamfer)
  translate([SCR_X0, SCR_Y0 - FIT, ztop - SCR_D - 0.5]) cube([X1 - W - SCR_X0, SCR_W + 2 * FIT, SCR_D + 0.5 + 0.01]);
  // USB-C through the right wall, with a recess for the plug's moulding
  translate([X1 - W - 1, USB_Y, USB_Z]) rotate([0, 90, 0]) linear_extrude(W + 2) rr(-3.75, -10, 3.75, 10, 2);
  translate([X1 - 1.8, USB_Y, USB_Z]) rotate([0, 90, 0]) linear_extrude(3) rr(-5.5, -12, 5.5, 12, 3);
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
    linear_extrude(5.1) spk_shape(2.8);
    translate([0, 0, -1]) linear_extrude(7.2) spk_shape(0.3);
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

module ring(r) { difference() { circle(r = r + LINE); circle(r = r); } }
module accent_shape() {
  vx = SCR_X0 + VIS_X; vy = SCR_Y0 + SCR_W / 2;
  // rings round the stick and the buttons
  translate(STICK) ring(RING_STICK);
  for (p = B30) translate(p) ring(RING30);
  for (p = B24) translate(p) ring(RING24);
  // frame round the screen window (outside its bevel)
  fx = VIS_L / 2 + 0.5 + 2 + SCR_FRAME; fy = VIS_W / 2 + 0.5 + 2 + SCR_FRAME;
  translate([vx, vy]) difference() {
    rr(-fx - LINE, -fy - LINE, fx + LINE, fy + LINE, 4 + LINE);
    rr(-fx, -fy, fx, fy, 4);
  }
  // racing stripes down each side, from the palm rest towards the back
  y0 = Y0 + PALM + 4;
  for (st = STRIPES) {
    xl = X0 + C + STRIPE_X + st[1];
    polygon([[xl, y0], [xl + st[0], y0], [xl + st[0], STRIPE_END[0] + st[0]], [xl, STRIPE_END[0]]]);
    xr = X1 - C - STRIPE_X - st[1];
    polygon([[xr - st[0], y0], [xr, y0], [xr, STRIPE_END[1]], [xr - st[0], STRIPE_END[1] + st[0]]]);
  }
  // the name, in front of the buttons
  translate(LOGO_AT) text(LOGO, size = LOGO_SIZE, font = "Liberation Sans:style=Bold", halign = "center", valign = "center", spacing = 1.2);
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
      // weight trays
      for (t = WEIGHTS) translate([0, 0, BASE - 0.1]) linear_extrude(WEIGHT_H + 0.1) difference() {
        rr(t[0] - 2, t[1] - 2, t[2] + 2, t[3] + 2, 4);
        rr(t[0], t[1], t[2], t[3], 2);
      }
    }
    // anti-slip mat recess underneath, with the screws countersunk into its floor
    translate([0, 0, -1]) linear_extrude(MAT_DEPTH + 1) outline(W + FIT + MAT_RIM, W + FIT + MAT_RIM);
    for (p = BASE_BOSSES) translate([p[0], p[1], 0]) {
      translate([0, 0, -1]) cylinder(d = 3.4, h = BASE + 2);
      translate([0, 0, MAT_DEPTH - 0.01]) cylinder(d1 = 6.6, d2 = 3.4, h = 1.6);
    }
    for (h = IO_HOLES) translate([IO_X + h[0], IO_Y - h[1], MAT_DEPTH + 0.6]) cylinder(d = PILOT, h = BASE + IO_STANDOFF);
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
  color([0.07,0.07,0.07]) translate([STICK[0], STICK[1], H]) cylinder(d = 30, h = 1.5);
  color([0.13,0.13,0.13]) translate([STICK[0], STICK[1], H]) cylinder(d = 8, h = 32);
  color([0.85,0.2,0.23]) translate([STICK[0], STICK[1], H + 32 + 15]) sphere(d = 35);
  for (p = B30) translate([p[0], p[1], H]) { color([0.13,0.13,0.13]) cylinder(d = 33, h = 1.2); color([0.95,0.95,0.95]) cylinder(d = 24, h = 7); }
  for (p = B24) translate([p[0], p[1], H]) { color([0.13,0.13,0.13]) cylinder(d = 27, h = 1.2); color([0.95,0.95,0.95]) cylinder(d = 19, h = 6); }
  color([0.05,0.17,0.3]) translate([SCR_X0 + VIS_X - VIS_L / 2, SCR_Y0 + SCR_W / 2 - VIS_W / 2, H - T]) cube([VIS_L, VIS_W, 0.5]);
}

module face_down() { translate([0, 0, H]) rotate([0, 180, 0]) children(); }

// ---------- fit check ----------
// Rough keep-out volumes for the parts inside. export.sh renders
// part="check_N"; each must come out empty (nothing touching the case).
module keepout(i) {
  e = 0.05; zt = H - T - e;
  if (i == 0) translate([-JLF_PLATE[0] / 2, CY - JLF_PLATE[1] / 2, zt - 1.6]) cube([JLF_PLATE[0], JLF_PLATE[1], 1.6]);  // JLF plate
  if (i == 1) {                                                                                                   // JLF body
    translate([-26, CY - 26, zt - 1.6 - 37]) cube([52, 52, 37]);
    translate([-46, CY - 12, zt - 1.6 - 37]) cube([20.1, 24, 30]);                     // wire socket + plug, facing left
  }
  if (i == 2) for (p = B30) translate([p[0], p[1], zt - 45]) cylinder(d = 33, h = 45);                              // 30 mm: 34.8 mm measured + wire connectors
  if (i == 3) for (p = B24) translate([p[0], p[1], zt - 45]) cylinder(d = 26, h = 45);                              // 24 mm, same allowance
  if (i == 4) translate([SCR_X0 + e, SCR_Y0 + e, zt - SCR_PARTS]) cube([SCR_L, SCR_W - 2 * e, SCR_PARTS]);         // screen + its connectors
  if (i == 5) translate([IO_X, IO_Y - 47, BASE + IO_STANDOFF + e]) cube([97, 49.5, 14]);                          // I/O board
  if (i == 6) translate([BAT[0] + e, BAT[1] + e, BASE + e]) cube([BAT[2] - BAT[0] - 2 * e, BAT[3] - BAT[1] - 2 * e, 6.5]); // battery
  if (i == 7) translate([SPK[0], SPK[1], zt - SPK_H]) linear_extrude(SPK_H) spk_shape(0);                                    // speaker
  if (i == 8) for (t = WEIGHTS) translate([t[0] + e, t[1] + e, BASE + e]) cube([t[2] - t[0] - 2 * e, t[3] - t[1] - 2 * e, WEIGHT_H]); // weights
}
module check(i) {
  // the I/O board, battery and weights sit on the base, so only the top is checked for them
  intersection() {
    union() { top_shell(); if (i != 5 && i != 6 && i != 8) base_plate(); }
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
else if (part == "check_8") check(8);
// the top halves and their accents are exported face down, ready to print
else if (part == "top_left")     face_down() difference() { half(true)  top_shell(); accent_volume(); }
else if (part == "top_right")    face_down() difference() { half(false) top_shell(); accent_volume(); }
else if (part == "accent_left")  face_down() intersection() { half(true)  top_shell(); accent_volume(); }
else if (part == "accent_right") face_down() intersection() { half(false) top_shell(); accent_volume(); }
else if (part == "base_left")    base_half(true);
else if (part == "base_right")   base_half(false);
else if (part == "splice")       splice();
else if (part == "screen_clip")  screen_clip();
