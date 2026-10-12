#!/usr/bin/env bash
# Install PadForge locally to ~/.local
set -euo pipefail

REPO="${REPO:-https://github.com/padforge/padforge.git}"
BUILD_DIR="build-install"
PREFIX="${PREFIX:-$HOME/.local}"

git clone --depth 1 "$REPO" /tmp/padforge-install 2>/dev/null || {
  # If already in repo
  cd "${PWD}"
}

# If running from repo, use current
if [ -f CMakeLists.txt ] && [ -d src ]; then
  SRC_DIR="$PWD"
else
  SRC_DIR="/tmp/padforge-install"
fi

cmake -S "$SRC_DIR" -B "$SRC_DIR/$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DPADFORGE_ENABLE_LUA=ON \
  -DPADFORGE_BUILD_TESTS=OFF 2>/dev/null || \
cmake -S "$SRC_DIR" -B "$SRC_DIR/$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DPADFORGE_ENABLE_LUA=ON \
  -DPADFORGE_BUILD_TESTS=OFF

cmake --build "$SRC_DIR/$BUILD_DIR" --parallel
cmake --install "$SRC_DIR/$BUILD_DIR" --prefix "$PREFIX"

echo "Installed to $PREFIX/bin/padforge"
command -v padforge >/dev/null && echo "You can now run: padforge"
