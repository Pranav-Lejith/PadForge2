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
