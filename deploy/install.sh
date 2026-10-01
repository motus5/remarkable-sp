#!/bin/sh
# Installs a release archive on the reMarkable into /home/root/apps/<app>.
#   deploy/install.sh <archive.tar.gz> [ssh-host]     (default host: root@10.11.99.1)
# Writes nothing outside /home/root/apps, starts nothing, no autostart.
# An existing data/ folder of the app is kept.
set -eu
ARCHIVE=${1:?Archiv angeben, z. B. remarkable-sp-rm1-0.3.1.tar.gz}
HOST=${2:-root@10.11.99.1}
APP=$(tar -tzf "$ARCHIVE" | head -1 | cut -d/ -f1)
case "$APP" in
    remarkable-sp|hello-remarkable) ;;
    *) echo "Unbekanntes Archiv (Ordner '$APP')" >&2; exit 1 ;;
esac
NEED_KB=$(( $(gzip -l "$ARCHIVE" | awk 'NR==2 {print $2}') / 1024 * 2 + 1024 ))

echo "Installiere $APP nach $HOST:/home/root/apps/$APP (benötigt ca. ${NEED_KB} KB)"
ssh "$HOST" "set -e
    FREE=\$(df -k /home | awk 'NR==2 {print \$4}')
    case \"\$FREE\" in ''|*[!0-9]*) echo 'Freier Platz in /home nicht ermittelbar' >&2; exit 1 ;; esac
    if [ \"\$FREE\" -lt $NEED_KB ]; then echo \"Zu wenig Platz in /home: \$FREE KB frei\" >&2; exit 1; fi
    mkdir -p /home/root/apps"
scp "$ARCHIVE" "$HOST:/home/root/apps/.$APP-install.tar.gz"
ssh "$HOST" "set -e
    cd /home/root/apps
    tar -xzf .$APP-install.tar.gz
    rm -f .$APP-install.tar.gz
    chmod +x $APP/start.sh
    echo \"Installiert: \$(cat $APP/VERSION 2>/dev/null) in /home/root/apps/$APP\"
    ls -la /home/root/apps/$APP"
