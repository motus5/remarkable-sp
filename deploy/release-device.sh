#!/bin/sh
# Attaches a tablet build to an existing GitHub release so the in-app updater
# finds it. Usage: deploy/release-device.sh v0.3.0 build-rm/remarkable-sp arm|arm64
# Needs the GitHub CLI (gh) logged in with write access to the repository.
set -e
TAG=${1:?Tag angeben, z. B. v0.3.0}
BIN=${2:?Pfad zur Geräte-Binary}
ARCH=${3:?arm (rM1/rM2) oder arm64 (Paper Pro/Move)}
TMP=$(mktemp -d)
cp "$BIN" "$TMP/remarkable-sp-$ARCH"
(cd "$TMP" && sha256sum "remarkable-sp-$ARCH" > "remarkable-sp-$ARCH.sha256")
gh release upload "$TAG" "$TMP/remarkable-sp-$ARCH" "$TMP/remarkable-sp-$ARCH.sha256" --clobber
rm -rf "$TMP"
