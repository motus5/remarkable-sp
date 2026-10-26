#!/bin/sh
# T3: a fast synthetic pen stroke (spiral at 200 Hz, PenInput::startReplay)
# must reach the ink canvas with >= 95 % of its samples. Prints the
# latency probe lines; times are desktop values, not device values.
#   tests/pen_point_density.sh <app-binary> <screen-config.json> <out-dir>
set -eu
APP=$1
CONFIG=$2
OUT=$3
rm -rf "$OUT" && mkdir -p "$OUT"
RMSP_DATA_DIR="$OUT/data" RMSP_FULLSCREEN=1 RMSP_SIMULATE_EPAPER=1 RMSP_LATENCY_LOG=1 \
    RMSP_OPEN_TASK=new RMSP_PEN_REPLAY=spiral QT_QPA_PLATFORM="offscreen:configfile=$CONFIG" \
    "$APP" > "$OUT/stdout.log" 2> "$OUT/stderr.log" || true
grep LATENZ "$OUT/stderr.log" || true
SUM=$(grep LATENZ-SUMME "$OUT/stderr.log" || true)
DENSITY=$(echo "$SUM" | sed -n 's/.* dichte=\([0-9.]*\)%.*/\1/p')
RAW=$(echo "$SUM" | sed -n 's/.* roh=\([0-9]*\) .*/\1/p')
if [ -z "$DENSITY" ] || [ "${RAW:-0}" -lt 400 ]; then
    echo "T3 FEHLER: keine Messung (roh=${RAW:-?})"; cat "$OUT/stderr.log"; exit 1
fi
if awk "BEGIN { exit !($DENSITY >= 95) }"; then
    echo "T3 ok: dichte=$DENSITY% bei roh=$RAW"
else
    echo "T3 FEHLER: nur $DENSITY% der Stiftpunkte kommen an"; exit 1
fi
