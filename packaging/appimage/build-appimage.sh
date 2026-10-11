#!/usr/bin/env bash
#
# Build a portable AppImage for PadForge.
#
#   packaging/appimage/AppRun-build.sh
#
# Requires: cmake, ninja, a C++ toolchain, Qt 6 dev files, and linuxdeploy plus
# the linuxdeploy Qt plugin (https://github.com/linuxdeploy). Set
# QMAKE=/path/to/qmake if more than one Qt is installed.

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf "${work}"' EXIT

appdir="${work}/AppDir"
build_dir="${work}/build"

cmake -S "${repo_root}" -B "${build_dir}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DPADFORGE_BUILD_TESTS=OFF
cmake --build "${build_dir}"
DESTDIR="${appdir}" cmake --install "${build_dir}"

install -Dm644 "${repo_root}/packaging/appimage/AppRun" "${appdir}/AppRun"
chmod +x "${appdir}/AppRun"
mkdir -p "${appdir}/usr/share/PadForge/plugins"
cp -r "${repo_root}/plugins/lua/." "${appdir}/usr/share/PadForge/plugins/"

export OUTPUT="${repo_root}/dist/PadForge-x86_64.AppImage"
export QMAKE="${QMAKE:-$(command -v qmake6 || command -v qmake)}"
mkdir -p "${repo_root}/dist"

linuxdeploy --appdir "${appdir}" \
    --plugin qt \
    --desktop-file "${appdir}/usr/share/applications/org.padforge.PadForge.desktop" \
    --icon-file "${appdir}/usr/share/icons/hicolor/scalable/apps/org.padforge.PadForge.svg" \
    --output appimage

echo "Built ${OUTPUT}"
