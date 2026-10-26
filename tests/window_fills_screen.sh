#!/bin/sh
# T1: in full-screen mode the main window must cover the whole rM1 screen.
# Device test v0.3.1 found a fixed 702x936 window on the 1404x1872 display.
#   tests/window_fills_screen.sh <app-binary> <screen-config.json> <out-dir>
set -eu
APP=$1
CONFIG=$2
OUT=$3
rm -rf "$OUT" && mkdir -p "$OUT"
# RMSP_SIMULATE_EPAPER: like the epaper platform, no Window.FullScreen is
# applied; the window must size itself to the screen.
LINE=$(RMSP_DATA_DIR="$OUT/data" RMSP_FULLSCREEN=1 RMSP_SIMULATE_EPAPER=1 RMSP_SCREENSHOT="$OUT/fullscreen.png" \
       QT_QPA_PLATFORM="offscreen:configfile=$CONFIG" "$APP" 2>"$OUT/stderr.log" | grep RMSP_GEOMETRY || true)
echo "$LINE"
case "$LINE" in
    *"window=1404x1872 screen=1404x1872 image=1404x1872 saved=1"*) echo "T1 ok" ;;
    *) echo "T1 FEHLER: Fenster füllt den Bildschirm nicht"; cat "$OUT/stderr.log"; exit 1 ;;
esac
