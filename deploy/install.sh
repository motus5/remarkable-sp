#!/bin/sh
# Copies a cross-compiled build onto the tablet over SSH (USB: 10.11.99.1).
# Usage: deploy/install.sh <path-to-arm-binary> [host]
set -e
BIN=${1:?Pfad zur ARM-Binary angeben}
HOST=${2:-root@10.11.99.1}
ssh "$HOST" 'mkdir -p /opt/bin /opt/etc/draft /opt/usr/share/applications'
scp "$BIN" "$HOST:/opt/bin/remarkable-sp"
scp "$(dirname "$0")/remarkable-sp.draft" "$HOST:/opt/etc/draft/remarkable-sp"
scp "$(dirname "$0")/remarkable-sp.oxide" "$HOST:/opt/usr/share/applications/remarkable-sp.oxide"
echo "Installiert. Start über den Launcher (Oxide/Draft/remux)."
