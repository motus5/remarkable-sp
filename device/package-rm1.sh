#!/bin/sh
# Builds the hello world and the app with the official reMarkable 1 SDK and
# packs them for /home/root/apps on the device.
#   device/package-rm1.sh <sdk-dir> <out-dir>
# <sdk-dir> is where the SDK installer put its files (environment-setup-* inside).
# Optional: EPAPER_QPA=<path to a checkout of github.com/reMarkable/epaper-qpa>
set -eu
SDK=$(cd "${1:?SDK-Verzeichnis angeben}" && pwd)
OUT=$(mkdir -p "${2:?Ausgabeverzeichnis angeben}" && cd "$2" && pwd)
ROOT=$(cd "$(dirname "$0")/.." && pwd)
VERSION=$(sed -n 's/^project(remarkable-sp VERSION \([0-9.]*\).*/\1/p' "$ROOT/CMakeLists.txt")
BUILD=$(mktemp -d)
trap 'rm -rf "$BUILD"' EXIT

# shellcheck disable=SC1090
. "$SDK"/environment-setup-cortexa9hf-neon-remarkable-linux-gnueabi

cmake -S "$ROOT/device/hello" -B "$BUILD/hello" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD/hello" -j"$(nproc)"
cmake -S "$ROOT" -B "$BUILD/app" -DCMAKE_BUILD_TYPE=Release -DRMSP_BUILD_TESTS=OFF \
      -DRMSP_UPDATE_ASSET=remarkable-sp-rm1
cmake --build "$BUILD/app" -j"$(nproc)"

FALLBACK=""
if [ -n "${EPAPER_QPA:-}" ]; then
    cmake -S "$EPAPER_QPA" -B "$BUILD/qpa" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
    cmake --build "$BUILD/qpa" -j"$(nproc)" --target epaper
    FALLBACK="$BUILD/qpa/libepaper.so"
fi

pack() { # pack <name> <binary> <start-script>
    d="$BUILD/stage/$1"
    mkdir -p "$d/fallback/platforms" "$d/licenses"
    $STRIP -o "$d/$(basename "$2")" "$2"
    install -m 755 "$3" "$d/start.sh"
    echo "$VERSION" > "$d/VERSION"
    cp "$ROOT/LICENSE" "$d/licenses/remarkable-sp-MIT.txt"
    if [ -n "$FALLBACK" ]; then
        $STRIP -o "$d/fallback/platforms/libepaper.so" "$FALLBACK"
        printf '%s\n' "libepaper.so: build of https://github.com/reMarkable/epaper-qpa" \
            "(LGPL-2.1, Copyright The Qt Company / reMarkable AS); used only if the" \
            "system lacks /usr/lib/plugins/platforms/libepaper.so." > "$d/licenses/epaper-qpa.txt"
    fi
}
pack hello-remarkable "$BUILD/hello/hello_remarkable" "$ROOT/device/rm1/start-hello.sh"
pack remarkable-sp "$BUILD/app/remarkable-sp" "$ROOT/device/rm1/start-remarkable-sp.sh"
cp "$ROOT/third_party/fonts/OFL.txt" "$BUILD/stage/remarkable-sp/licenses/NotoSans-OFL.txt"
cp "$ROOT/third_party/argon2/LICENSE" "$BUILD/stage/remarkable-sp/licenses/argon2.txt"

tar -C "$BUILD/stage" -czf "$OUT/hello-remarkable-rm1.tar.gz" hello-remarkable
tar -C "$BUILD/stage" -czf "$OUT/remarkable-sp-rm1-$VERSION.tar.gz" remarkable-sp
# Plain binary for the in-app updater (src/updater.cpp looks for this name).
cp "$BUILD/stage/remarkable-sp/remarkable-sp" "$OUT/remarkable-sp-rm1"
(cd "$OUT" && sha256sum remarkable-sp-rm1 > remarkable-sp-rm1.sha256 \
    && sha256sum hello-remarkable-rm1.tar.gz "remarkable-sp-rm1-$VERSION.tar.gz" remarkable-sp-rm1 > SHA256SUMS)
ls -l "$OUT"
