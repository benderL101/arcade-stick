# Arcade Stick case v2

A new, cleaner case for the arcade stick. It's 300 × 227 × 55 mm and fits a Sanwa JLF, six 30 mm buttons in a Vewlix layout, START / SELECT / HOME (24 mm), the Freenove 2.8" screen, the I/O board, the speaker and a 1S LiPo.

![Assembly](images/assembly.png)

What's different from the first case:

- **Rounded palm rest:** the front edge rolls into the top with a 20 mm radius, so there's no sharp edge under your wrists or forearms. It's the roundest shape that still prints face down without supports. The other top edges have a 4 mm, 45° chamfer.
- **Room for your hands:** there's 105 mm between the front edge and the stick, and about 60 mm in front of the nearest button.
- **START / SELECT / HOME** sit behind the main buttons, away from the stick hand, so they don't get knocked mid-game.
- **Stays put:** the whole bottom takes one anti-slip mat, instead of four small feet. It grips a desk and sits softly on your lap. Two trays inside take steel weights, because a heavier stick creeps less on your lap.
- **Finger scoops:** shallow scoops under the left and right sides make it easy to pick up and hold on your lap.
- **Two-colour top:** red rings round the stick and every button, a frame round the screen, a pair of racing stripes down each side, and ARCADE STICK in front of the buttons. They're 3 layers deep and print first, because the top prints face down.
- **Hidden seam:** the joint between the two top halves runs between the stick and the buttons. A thin design line on the top makes it look deliberate.
- **Bevelled screen window.** The USB-C port sits in a recess in the right side, so you can charge and flash with the case closed.
- **Speaker grille** in the top behind the stick, with a ring underneath to hold the speaker.

## What to print

Everything prints on the Bambu X1 Carbon's 256 × 256 mm bed with **no supports**.

| File | Qty | How to print |
|---|---|---|
| `3mf/top_left_2colour.3mf` | 1 | **Face down** (as it opens). Two colours in one piece. |
| `3mf/top_right_2colour.3mf` | 1 | Same. |
| `stl/base_left.stl` | 1 | Flat, as exported |
| `stl/base_right.stl` | 1 | Flat, as exported |
| `stl/splice.stl` | 2 | Flat |
| `stl/screen_clip.stl` | 4 | Flat |

Suggested settings: PLA or PETG, 0.2 mm layers, 4 walls, 20 % gyroid infill, textured PEI plate (gives the top a nice finish).

### Two colours in Bambu Studio

Open `top_left_2colour.3mf`. It loads as one object with two parts: **Body** on filament 1 and **Design** (the rings, stripes and name) on filament 2. Set filament 1 to your body colour (for example black) and filament 2 to your stick's colour (for example red), then slice. Do the same for the right half.

The plain STL files are in `stl/` as well. `top_*.stl` is the body, and `accent_*.stl` is the design. Print just the body files for a single-colour top: the rings, stripes and name then show as shallow 0.6 mm grooves, which also looks good.

## Parts you need

| Part | Qty | Notes |
|---|---|---|
| M4 × 12 countersunk screws + M4 nyloc nuts | 4 | Joystick plate (Sanwa JLF-P1, turned so its long side runs front to back) |
| M3 × 10 countersunk screws | 12 | Base plate into the case's posts |
| M3 × 10 pan-head self-tapping screws | 8 | The two splices |
| M3 × 8 pan-head self-tapping screws | 7 | 4 screen clips, 3 I/O board |
| 1.75 mm filament, cut into 12 mm pins | 10 | Lines up the two top halves |
| 2 mm neoprene or EVA foam sheet, about 280 × 210 mm | 1 | Anti-slip mat for the bottom. An old cloth-top mouse mat or yoga mat works. Trace the recess in the base and cut it out. |
| Steel washers (e.g. M10 / M12) or fishing sinkers | up to ~400 g | Optional ballast for the two weight trays |
| Glue: plastic epoxy or thick CA | | For the top seam |
| Double-sided foam tape | | Holds the battery in its bay |

All screw holes are 2.5 mm pilot holes for screws that cut their own thread. If you'd rather use M3 heat-set inserts, change `PILOT = 2.5` to `PILOT = 4.0` in the `.scad` file and run `export.sh`.

## Putting it together

1. **Join the top halves.** Push filament pins into the holes along one seam face, so half of each pin sticks out. Put glue on both seam faces and press the halves together on a flat surface. Screw a **splice** across the seam at the front and the back (4 screws each). Wipe off any glue that squeezes out, then leave it to cure.
2. **Buttons.** Snap the six 30 mm and three 24 mm buttons in from the top. The three small ones sit behind the main buttons. Left to right: **SELECT, HOME, START**.
3. **Joystick.** Take the ball and dust washer off the JLF. From underneath, push the shaft up through the big hole with the mounting plate's long side running front to back and the **wire socket facing left** (towards the speaker), where there's room for the plug. Fit the four M4 countersunk screws from the top, add the nuts underneath, then refit the dust washer and ball.
4. **Screen.** Put the screen face down into its frame under the window, with the **USB-C end against the right-hand wall**. Check that the port lines up with the hole in the side. Screw the four clips over the board's corners. If the picture is upside down, use **Menu → Flip screen**.
5. **Speaker.** Press it into the ring under the grille. A dab of hot glue holds it.
6. **I/O board.** Screw it onto the three posts at the front of the base plate, with the components facing up. The posts only line up one way round.
7. **Battery and weights.** Stick the battery into the bay at the front left of the base plate (under your left palm) with foam tape. If you want the stick heavier, fill the two trays with washers or sinkers and hot-glue them in so they can't rattle. One tray is just behind the battery on the left; the other is under the right-hand side.
8. **Wire it all up.** The firmware manual and `hardware/io-board/README.md` show which terminal each input goes to. Then open **Menu → Input test** and press everything.
9. **Close it.** Lay the base halves into the bottom (the lap joint overlaps in the middle) and fit the 12 countersunk screws. Glue the anti-slip mat into the recess underneath with contact adhesive or spray glue. It covers the screws, so cut small holes over them if you'll want to open the case often.

## Measurements to check before you print

The screen numbers were measured on Ben's board. The rest come from datasheets, so check them with calipers. If any differ, change the number near the top of `arcade_stick_case.scad`, then run `export.sh` again.

| What | Setting | Value used |
|---|---|---|
| Screen board size | `SCR_L`, `SCR_W`, `SCR_D`, `SCR_PARTS` | 85 × 50 mm, 4 mm thick, 8 mm over the connectors (measured) |
| Lit picture size and position | `VIS_L`, `VIS_W`, `VIS_X` | 60 × 45 mm, centred across the board, centre 38.6 mm from the MIC end (measured) |
| USB-C position along the board's short edge | `USB_Y` | About the middle; the slot allows ±3.5 mm |
| Speaker size | `SPK_L`, `SPK_W`, `SPK_H` | 44 × 28 mm, 9 mm deep, rounded (measured) |
| Battery bay | `BAT` | 58 × 64 mm inside, for a 585460 LiPo (5.8 × 54 × 60 mm, measured) |
| Weight trays | `WEIGHTS`, `WEIGHT_H` | 40 × 78 mm and 43 × 80 mm, 8 mm deep |
| Joystick plate | `JLF_SCREWS` | Sanwa JLF-P1: slots 84 × 40 mm, 95 × 53 mm plate (Focus Attack / Akihabara Shop drawing) |

Button holes are 30.3 mm and 24.3 mm. The panel is thinned to 3 mm around each one, so Sanwa snap-in buttons clip in. If your printer makes holes tight, increase `HOLE30` / `HOLE24`.

## Editing the design

`arcade_stick_case.scad` is OpenSCAD and fully parametric. Every size is a named setting at the top. The origin is the joystick's centre. `export.sh` writes all the STL files and the preview images. Then `python make_3mf.py` combines each top half and its design into the two-colour 3MF files. Setting `part="check_0"` … `"check_8"` gives a fit check for each inside part (joystick plate and body, both button sizes, screen, I/O board, battery, speaker, weights): it should come out empty.

## Why it's shaped this way

The comfort changes come from what fight stick reviews praise and complain about:

- **Sharp front edges** are the most common complaint. One reviewer found the Victrix Pro FS's sharp front edge uncomfortable on the lap ([God is a Geek](https://www.godisageek.com/reviews/victrix-fs-pro-fight-stick-review/)). A buyer's guide says the same about the Nacon Daija digging into your forearm ([Wearify](https://thewearify.com/?p=211370)). That's why the front is a rounded palm rest.
- **Room to rest your wrists** is praised on the HORI Fighting Edge ([ProFightStick](https://www.profightstick.com/hori-fighting-edge-review/)). Its lack is a complaint about the 8BitDo stick ([HL Planet](https://www.hlplanet.com/?p=10922)). That's why there's 105 mm of palm room in front of the stick.
- **Weight and anti-slip bases** keep a stick still on your lap. Lighter sticks creep, and full anti-slip bottoms get praised ([Wearify](https://thewearify.com/?p=211370), [Tom's Guide on the Qanba Obsidian 2](https://www.tomsguide.com/gaming/gaming-peripherals/qanba-q7-obsidian-2-review)). That's why the case has the mat and the weight trays.
- **Menu buttons away from the main buttons** avoid accidental presses, as noted on the Fighting Edge ([ProFightStick](https://www.profightstick.com/hori-fighting-edge-review/)).
- **Indented sides** make a stick easier to carry ([ProFightStick](https://www.profightstick.com/hori-fighting-edge-review/)). That's why the case has finger scoops.

The Victrix Pro FS's sloped wrist rest also gets praise ([Tom's Guide](https://www.tomsguide.com/gaming/gaming-peripherals/victrix-pro-fs-review)). A slope that shallow can't print face down without supports, so this case uses the rounded front instead.
