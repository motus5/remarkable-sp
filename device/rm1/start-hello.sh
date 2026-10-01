#!/bin/sh
# Starts the hello world test app on a reMarkable 1 (firmware 3.26, Qt 6 from the system).
# xochitl must be stopped first: systemctl stop xochitl   (see TEST-RM1.md)
# Everything lives in this directory; nothing outside /home/root/apps is touched.
DIR=$(cd "$(dirname "$0")" && pwd)

# Settings from https://developer.remarkable.com/documentation/qt_epaper (rm1).
export QT_QUICK_BACKEND=epaper
export QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS="${QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS:-rotate=180}"

# Use the bundled build of reMarkable's epaper platform plugin only if the
# system does not provide one.
if [ ! -e /usr/lib/plugins/platforms/libepaper.so ]; then
    export QT_PLUGIN_PATH="$DIR/fallback${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}"
    echo "start: using bundled epaper platform plugin" >&2
fi



exec "$DIR/hello_remarkable" -platform epaper "$@"
