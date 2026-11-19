#!/usr/bin/env bash
# Exports every printable part to stl/ and renders the preview images.
# Needs OpenSCAD (https://openscad.org). Works in Git Bash on Windows.
set -euo pipefail
cd "$(dirname "$0")"
OPENSCAD=${OPENSCAD:-openscad}
command -v "$OPENSCAD" >/dev/null 2>&1 || OPENSCAD="/c/Program Files/OpenSCAD/openscad.com"
mkdir -p stl images
for p in top accent base screen_clip; do
  echo "== $p"
  "$OPENSCAD" -D "part=\"$p\"" -o "stl/$p.stl" arcade_stick_case.scad
done
"$OPENSCAD" -o images/assembly.png --imgsize=1600,1100 --viewall --autocenter \
  --camera=0,0,0,50,0,30,0 --projection=p --colorscheme=Tomorrow arcade_stick_case.scad
"$OPENSCAD" -o images/top.png --imgsize=1600,1100 --viewall --autocenter \
  --camera=0,0,0,0,0,0,0 --projection=o --colorscheme=Tomorrow arcade_stick_case.scad
