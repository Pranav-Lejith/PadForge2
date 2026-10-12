#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
qt_prefix="${QT_PREFIX:-$(brew --prefix qt)}"
lua_prefix="${LUA_PREFIX:-$(brew --prefix lua@5.4)}"
build_dir="${BUILD_DIR:-${repo_root}/build-macos}"
stage_dir="${STAGE_DIR:-${repo_root}/dist/macos-stage}"
dist_dir="${DIST_DIR:-${repo_root}/dist}"

export PKG_CONFIG_PATH="${lua_prefix}/lib/pkgconfig:${PKG_CONFIG_PATH:-}"

cmake -S "${repo_root}" -B "${build_dir}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="${qt_prefix}" \
    -DPADFORGE_BUILD_TESTS=ON \
    -DPADFORGE_ENABLE_LUA=ON
cmake --build "${build_dir}" --parallel
QT_QPA_PLATFORM=offscreen ctest --test-dir "${build_dir}" --output-on-failure

cmake --install "${build_dir}" --prefix "${stage_dir}"
app_bundle="${stage_dir}/PadForge.app"
executable="${app_bundle}/Contents/MacOS/PadForge"
frameworks="${app_bundle}/Contents/Frameworks"
test -x "${executable}"

"${qt_prefix}/bin/macdeployqt" "${app_bundle}" -always-overwrite

# Lua is a non-Qt runtime library, so macdeployqt does not copy it into the app.
lua_library="$(find "${lua_prefix}/lib" -maxdepth 1 -type f -name 'liblua*.dylib' -print -quit)"
if [[ -z "${lua_library}" ]]; then
    echo "Could not find the Lua 5.4 shared library under ${lua_prefix}/lib" >&2
    exit 1
fi
mkdir -p "${frameworks}"
lua_name="$(basename "${lua_library}")"
install -m644 "${lua_library}" "${frameworks}/${lua_name}"
lua_dependency="$(otool -L "${executable}" | awk '/liblua[^ ]*\.dylib/ { print $1; exit }')"
if [[ -z "${lua_dependency}" ]]; then
    echo "The PadForge executable does not link to the Lua runtime" >&2
    exit 1
fi
install_name_tool -id "@rpath/${lua_name}" "${frameworks}/${lua_name}"
install_name_tool -change "${lua_dependency}" "@rpath/${lua_name}" "${executable}"
if ! otool -l "${executable}" | grep -A2 LC_RPATH | grep -q '@executable_path/../Frameworks'; then
    install_name_tool -add_rpath '@executable_path/../Frameworks' "${executable}"
fi

mkdir -p "${app_bundle}/Contents/MacOS/plugins"
cp -R "${repo_root}/plugins/lua/." "${app_bundle}/Contents/MacOS/plugins/"

# Re-sign the modified bundle with an ad-hoc signature and check for build-host
# library paths before producing the disk image.
codesign --force --deep --sign - "${app_bundle}"
codesign --verify --deep --strict "${app_bundle}"
if otool -L "${executable}" | grep -E '/opt/homebrew|/usr/local/Cellar'; then
    echo "The app still references a Homebrew library outside its bundle" >&2
    exit 1
fi

mkdir -p "${dist_dir}"
dmg_path="${dist_dir}/PadForge-macos-arm64.dmg"
hdiutil create -volname PadForge -srcfolder "${app_bundle}" \
    -ov -format UDZO "${dmg_path}"
hdiutil verify "${dmg_path}"
echo "Created ${dmg_path}"
