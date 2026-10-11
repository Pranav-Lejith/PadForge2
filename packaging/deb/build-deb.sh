#!/usr/bin/env bash
#
# Build a Debian package for PadForge from the repository root.
#
#   packaging/deb/build-deb.sh [build-dir]
#
# Produces dist/padforge_<version>_<arch>.deb. Requires cmake, ninja, a C++
# toolchain, Qt 6 development files and dpkg-deb.

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
build_dir="${1:-${repo_root}/build-deb}"
staging="$(mktemp -d)"
trap 'rm -rf "${staging}"' EXIT

version="$(sed -n 's/^project(PadForge.*VERSION \([0-9.]*\).*/\1/p' "${repo_root}/CMakeLists.txt" | head -n1)"
version="${version:-0.1.0}"
arch="$(dpkg --print-architecture 2>/dev/null || echo amd64)"

cmake -S "${repo_root}" -B "${build_dir}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DPADFORGE_BUILD_TESTS=OFF
cmake --build "${build_dir}"

DESTDIR="${staging}" cmake --install "${build_dir}"
install -Dm644 "${repo_root}/LICENSE" "${staging}/usr/share/licenses/padforge/LICENSE"
mkdir -p "${staging}/usr/share/PadForge/plugins"
cp -r "${repo_root}/plugins/lua/." "${staging}/usr/share/PadForge/plugins/"

mkdir -p "${staging}/DEBIAN"
sed "s/^Version:.*/Version: ${version}/; s/^Architecture:.*/Architecture: ${arch}/" \
    "${repo_root}/packaging/deb/control" > "${staging}/DEBIAN/control"
installed_size="$(du -sk "${staging}/usr" | cut -f1)"
sed -i "s/^Installed-Size:.*/Installed-Size: ${installed_size}/" "${staging}/DEBIAN/control"

mkdir -p "${repo_root}/dist"
output="${repo_root}/dist/padforge_${version}_${arch}.deb"
dpkg-deb --build --root-owner-group "${staging}" "${output}"
echo "Built ${output}"
