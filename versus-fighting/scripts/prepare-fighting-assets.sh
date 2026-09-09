#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/assets/generated"
NGDEVKIT="$ROOT/../external/ngdevkit"
CONVERT="${CONVERT:-magick}"
PYTHON="${PYTHON:-python3}"

mkdir -p "$OUT"

for player in p1 p2; do
    for i in $(seq -w 0 28); do
        test -f "$ROOT/assets/source/$player/$i.png" || {
            echo "missing assets/source/$player/$i.png" >&2
            exit 1
        }
    done
done

frames=()
for player in p1 p2; do
    for i in $(seq -w 0 28); do
        src="$ROOT/assets/source/$player/$i.png"
        dst="$OUT/${player}_${i}.png"
        "$CONVERT" "$src" -background '#ff00ff' -gravity south -extent 64x64 "$dst"
        frames+=("$dst")
    done
done

"$CONVERT" "${frames[@]}" -append "$OUT/fighters.gif"
"$PYTHON" "$NGDEVKIT/tools/tiletool.py" --sprite -c "$OUT/fighters.gif" -o "$OUT/fighters.c1" "$OUT/fighters.c2"
"$PYTHON" "$NGDEVKIT/tools/paltool.py" "$OUT/fighters.gif" -o "$OUT/fighters.pal"

echo "generated $OUT/fighters.c1, fighters.c2 and fighters.pal"
