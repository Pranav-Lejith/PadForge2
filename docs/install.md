# Installing PadForge

PadForge is currently distributed as source and through the packaging recipes in
[`packaging`](../packaging). Pre-built packages will be attached to releases as
the project matures.

## Build from source

### 1. Install dependencies

| Distribution | Command |
| --- | --- |
| Arch Linux | `sudo pacman -S --needed base-devel cmake ninja qt6-base lua python` |
| Debian / Ubuntu | `sudo apt install build-essential cmake ninja-build python3 qt6-base-dev qt6-base-dev-tools libgl1-mesa-dev liblua5.4-dev` |
| Fedora | `sudo dnf install gcc-c++ cmake ninja-build python3 qt6-qtbase-devel lua-devel` |
| openSUSE | `sudo zypper install gcc-c++ cmake ninja python3 qt6-base-devel lua54-devel` |

### 2. Configure and build

```sh
git clone https://github.com/padforge/padforge.git
cd padforge
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Scintilla and Lexilla are downloaded and built automatically on the first
configure.

### 3. Install

```sh
sudo cmake --install build
```

This installs the `padforge` binary, the desktop entry, the icon and the
AppStream metadata. Then launch **PadForge** from your application menu or run
`padforge` from a terminal.

## Distribution packages

See [`packaging/README.md`](../packaging/README.md). In short:

- **Arch**: `cd packaging/arch && makepkg -si`
- **Fedora/RHEL/openSUSE**: `rpmbuild -ba packaging/rpm/padforge.spec`
- **Debian/Ubuntu**: `packaging/deb/build-deb.sh` then `apt install ./dist/*.deb`
- **AppImage**: `packaging/appimage/build-appimage.sh`
- **Flatpak**: `flatpak-builder --user --install --force-clean build-flatpak packaging/flatpak/org.padforge.PadForge.yaml`

## Verifying the build

```sh
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
```

## Troubleshooting

- **`Could not find a package configuration file for Qt6`** — install the Qt 6
  development packages, or pass `-DCMAKE_PREFIX_PATH=/path/to/qt6`.
- **Configure fails while downloading Scintilla/Lexilla** — the first configure
  needs network access. Behind a proxy set `https_proxy`, or pre-populate the
  CMake `FetchContent` cache.
- **No syntax highlighting** — the file type may be unknown. Pick a language
  manually from the **Language** menu or add the extension to
  `src/core/LanguageManager.cpp`.
