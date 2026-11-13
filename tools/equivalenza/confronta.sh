#!/bin/sh
# Verifica che due versioni del firmware abbiano lo stesso comportamento.
# Esegue entrambe su tutti gli scenari simulati e confronta le tracce
# (uscite DAC, barra LED, display, seriale, ritardi) istante per istante.
#
# Uso: tools/equivalenza/confronta.sh <commit_o_branch_di_riferimento>
#   es. tools/equivalenza/confronta.sh HEAD   (confronta il codice attuale con l'ultimo commit)
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
REPO=$(cd "$HERE/../.." && pwd)
REF=${1:-HEAD}
WORK=$(mktemp -d)
git -C "$REPO" archive "$REF" ANSCOVERY_ESP32 | tar -x -C "$WORK"
"$HERE/registra_tracce.sh" "$WORK/ANSCOVERY_ESP32" "$WORK/out_ref" > /dev/null
"$HERE/registra_tracce.sh" "$REPO/ANSCOVERY_ESP32" "$WORK/out_new" > /dev/null
diversi=0
for f in "$WORK"/out_ref/*.txt; do
    n=$(basename "$f")
    [ "$n" = warnings.txt ] && continue
    if cmp -s "$f" "$WORK/out_new/$n"; then
        echo "uguale   ${n%.txt}"
    else
        echo "DIVERSO  ${n%.txt} (primo secondo diverso: $(diff "$f" "$WORK/out_new/$n" | grep -m1 '^<' | awk '{print $3}') ms)"
        diversi=$((diversi + 1))
    fi
done
rm -rf "$WORK"
[ $diversi -eq 0 ] && echo "Comportamento identico." || { echo "$diversi scenari con comportamento diverso."; exit 1; }
