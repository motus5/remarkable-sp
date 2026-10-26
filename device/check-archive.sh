#!/bin/sh
# T6: checks a release archive before it is published.
#   device/check-archive.sh <archive.tar.gz> <expected-top-dir> <max-unpacked-kb>
# Owner 0/0 on every entry (device test v0.3.1: files belonged to uid 1001),
# exactly one top directory, no absolute paths, no "..", no links.
set -eu
ARCHIVE=$1
TOP=$2
MAX_KB=$3
fail() { echo "T6 FEHLER ($ARCHIVE): $*" >&2; exit 1; }

LIST=$(tar -tvzf "$ARCHIVE" --numeric-owner)
NAMES=$(tar -tzf "$ARCHIVE")
[ -n "$NAMES" ] || fail "leeres Archiv"

BAD_OWNER=$(echo "$LIST" | awk '$2 != "0/0"' | head -n 3)
[ -z "$BAD_OWNER" ] || fail "Besitzer nicht 0/0: $BAD_OWNER"
TOPS=$(echo "$NAMES" | cut -d/ -f1 | sort -u)
[ "$TOPS" = "$TOP" ] || fail "Oberordner '$TOPS' statt '$TOP'"
echo "$NAMES" | grep -q '^/' && fail "absoluter Pfad"
echo "$NAMES" | grep -qE '(^|/)\.\.(/|$)' && fail "Pfad mit .."
echo "$LIST" | grep -qE '^[lh]' && fail "Link im Archiv"
KB=$(echo "$LIST" | awk '{ s += $3 } END { printf "%d", s / 1024 }')
[ "$KB" -le "$MAX_KB" ] || fail "entpackt ${KB} KB > ${MAX_KB} KB"
echo "T6 ok: $ARCHIVE ($(echo "$NAMES" | wc -l) Einträge, ${KB} KB, Besitzer 0/0)"
