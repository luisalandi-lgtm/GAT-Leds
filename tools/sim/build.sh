#!/bin/sh
# Compila il firmware sul PC con stub minimali di Arduino ed esegue uno scenario.
# Uso: tools/sim/build.sh [tilt|valsalva20|handgrip|db|cold|t1]
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
SRC="$HERE/../../ANSCOVERY_ESP32"
OUT="${TMPDIR:-/tmp}/anscovery_sim"
mkdir -p "$OUT"
cp "$SRC/ANSCOVERY_ESP32.ino" "$OUT/main_ino.cpp"
g++ -std=gnu++17 -Wall -Wextra -I"$HERE/stubs" -I"$SRC" -o "$OUT/sim" \
    "$HERE/sim.cpp" "$OUT/main_ino.cpp" "$SRC"/*.cpp
"$OUT/sim" "${1:-tilt}"
