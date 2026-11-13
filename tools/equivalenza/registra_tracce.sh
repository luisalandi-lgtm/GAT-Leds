#!/bin/sh
# Uso: registra_tracce.sh <cartella_sorgenti> <cartella_output> [--coverage]
# Compila il firmware con gli stub e registra le tracce di tutti gli scenari.
set -e
EQ=$(cd "$(dirname "$0")" && pwd)
SRC=$(cd "$1" && pwd)
OUT=$2
COV=$3
mkdir -p "$OUT/build"
cp "$SRC"/*.cpp "$SRC"/*.h "$OUT/build/" 2>/dev/null || true
cp "$SRC"/*.ino "$OUT/build/main_ino.cpp"
FLAGS="-std=gnu++17 -O1 -Wall -Wextra -Wno-unused-parameter -I$EQ/stubs -I$OUT/build"
[ "$COV" = "--coverage" ] && FLAGS="$FLAGS --coverage -O0"
g++ $FLAGS -o "$OUT/sim" "$EQ/sim.cpp" "$OUT"/build/*.cpp 2> "$OUT/warnings.txt"
for s in idle tilt tilt_static35 overrange tilt_slow_noisy tilt_stuck_valsalva20 valsalva handgrip handgrip_partial deep_breathing buttons fuzz1 fuzz2 fuzz3 fuzz4 fuzz5 fuzz6 fuzz7 fuzz8; do
    "$OUT/sim" $s > "$OUT/$s.txt"
done
tail -qn1 "$OUT"/*.txt
