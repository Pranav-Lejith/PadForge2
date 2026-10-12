# Packaging PadForge

PadForge's public release downloads do not require a GitHub account. The
AppImage bundles its application libraries; installing the released `.deb`
through `apt` installs its declared dependencies automatically. These recipes
are for maintainers building packages from source.

PadForge uses a standard CMake install layout:

| Path | Contents |
| --- | --- |
| `bin/padforge` | The application |
| `share/applications/org.padforge.PadForge.desktop` | Desktop entry |
| `share/icons/hicolor/scalable/apps/org.padforge.PadForge.svg` | Application icon |
| `share/metainfo/org.padforge.PadForge.metainfo.xml` | AppStream metadata |

> **Note:** because upstream Scintilla and Lexilla ship without build files,
> PadForge fetches pinned source archives with CMake `FetchContent` during
> configuration. Distribution builds therefore need network access (or a warm
> `FetchContent` cache / vendored sources).

## Arch Linux

End users can add the signed PadForge repository once using the
[setup guide](https://pranav-lejith.github.io/PadForge2/), then install or update
with `sudo pacman -Syu padforge`. The repository is public and requires no
GitHub account.

To build the package from source instead:

```sh
cd packaging/arch
makepkg -si
```

## Fedora / openSUSE / RHEL (RPM)

```sh
rpmbuild -ba packaging/rpm/padforge.spec
```

## Debian / Ubuntu (DEB)

```sh
packaging/deb/build-deb.sh
sudo apt install ./dist/padforge_0.1.0_amd64.deb
```

## AppImage

```sh
packaging/appimage/build-appimage.sh
```

Requires `linuxdeploy` and the `linuxdeploy-plugin-qt` binaries on `PATH`.

## macOS

On an Apple Silicon Mac with Homebrew, install the build tools and run the
packaging script. It builds and tests the app bundle, embeds Qt and Lua runtime
libraries, and creates `dist/PadForge-macos-arm64.dmg`:

```sh
brew install cmake ninja qt lua@5.4
packaging/macos/package-macos.sh
```

The release workflow runs the same build on macOS and attaches the disk image
to tagged releases.

## Flatpak

```sh
flatpak-builder --user --install --force-clean build-flatpak \
    packaging/flatpak/org.padforge.PadForge.yaml
```

## Generic tarball

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build && cpack -G TGZ
```
