#!/bin/sh
# T8 (#14): SIGINT/SIGTERM/SIGHUP end the app in order: exit code 0, the
# LATENZ-SUMME line is written and ink drawn just before is saved although
# the 1.5 s autosave has not run yet. Background jobs of a shell ignore
# SIGINT, so the app is started through python3, which restores it.
#   tests/graceful_exit.sh <app-binary> <screen-config.json> <out-dir>
set -eu
APP=$1
CONFIG=$2
OUT=$3
rm -rf "$OUT" && mkdir -p "$OUT"
FAIL=0
for SIG in INT TERM HUP; do
    D="$OUT/$SIG"
    mkdir -p "$D"
    # Replay starts after 2.5 s and the spiral ends ~2 s later; the signal at
    # 5 s comes after the stroke and before the autosave (6 s).
    RMSP_DATA_DIR="$D/data" RMSP_FULLSCREEN=1 RMSP_SIMULATE_EPAPER=1 RMSP_LATENCY_LOG=1 \
        RMSP_OPEN_TASK=new RMSP_PEN_REPLAY=spiral QT_QPA_PLATFORM="offscreen:configfile=$CONFIG" \
        python3 -c 'import os, signal, sys; signal.signal(signal.SIGINT, signal.SIG_DFL); os.execv(sys.argv[1], sys.argv[1:])' \
        "$APP" > "$D/stdout.log" 2> "$D/stderr.log" &
    PID=$!
    sleep 5
    kill -"$SIG" "$PID"
    RC=0
    wait "$PID" || RC=$?
    INK=$(find "$D/data/ink" -type f | wc -l)
    if [ "$RC" -eq 0 ] && grep -q "beende geordnet" "$D/stderr.log" && grep -q LATENZ-SUMME "$D/stderr.log" \
        && [ "$INK" -ge 1 ]; then
        echo "T8 ok: SIG$SIG exit=0, LATENZ-SUMME da, Tinte gespeichert ($INK Datei)"
    else
        echo "T8 FEHLER: SIG$SIG exit=$RC, Summe=$(grep -c LATENZ-SUMME "$D/stderr.log"), Tinte=$INK"
        tail -5 "$D/stderr.log"
        FAIL=1
    fi
done
exit $FAIL
