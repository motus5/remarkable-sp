#!/bin/sh
# Removes an app from /home/root/apps on the reMarkable.
#   deploy/uninstall.sh <remarkable-sp|hello-remarkable> [ssh-host] [--purge]
# Without --purge the app's data/ folder (tasks, handwriting, settings) is kept.
set -eu
APP=${1:?App angeben: remarkable-sp oder hello-remarkable}
HOST=${2:-root@10.11.99.1}
PURGE=${3:-}
case "$APP" in
    remarkable-sp|hello-remarkable) ;;
    *) echo "Unbekannte App '$APP'" >&2; exit 1 ;;
esac
ssh "$HOST" "set -e
    DIR=/home/root/apps/$APP
    [ -d \"\$DIR\" ] || { echo \"\$DIR existiert nicht – nichts zu tun\"; exit 0; }
    if pidof remarkable-sp hello_remarkable >/dev/null 2>&1; then
        echo 'App läuft noch – bitte zuerst beenden' >&2; exit 1
    fi
    if [ '$PURGE' = '--purge' ] || [ ! -d \"\$DIR/data\" ]; then
        rm -rf \"\$DIR\"
        echo \"Entfernt: \$DIR\"
    else
        for f in \"\$DIR\"/* \"\$DIR\"/.[!.]*; do
            [ -e \"\$f\" ] || continue
            [ \"\${f##*/}\" = data ] && continue
            rm -rf \"\$f\"
        done
        echo \"Programm entfernt, Daten behalten: \$DIR/data\"
    fi
    rmdir /home/root/apps 2>/dev/null || true"
