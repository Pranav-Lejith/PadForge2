# PadForge

**A fast, native Linux text editor with a Notepad++-style workflow.**

PadForge is a from-scratch project that brings the familiar tabbed, keyboard-first
editing experience popularised by Notepad++ to a modern Linux stack: C++20,
Qt 6 Widgets and the [Scintilla](https://www.scintilla.org/) editing component
with [Lexilla](https://www.scintilla.org/Lexilla.html) for syntax highlighting.

> PadForge is an independent project. It is *inspired by* Notepad++ but is not
> affiliated with, endorsed by, or derived from the Notepad++ source. See
> [`NOTICE`](NOTICE).

## Features

- **Tabbed editing** with a separate **split view** for side-by-side files.
- **Syntax highlighting for 50+ languages** via Lexilla, with automatic
  detection from the file extension, shebang or content.
- **Encoding aware**: UTF-8 (with/without BOM), UTF-16 LE/BE, UTF-32 LE/BE and
  Latin-1, with automatic detection on open and conversion on demand.
- **Line-ending control**: LF, CRLF and CR, detected per file.
- **Find & Replace** with plain, whole-word, extended (`\n`, `\t`, `\xHH`) and
  regular-expression modes, wrap-around, counts and "mark all".
- **Find in Files**: search a directory tree with filters and jump to a hit.
- **Bookmarks**, **code folding**, **line numbers**, **brace matching**,
  **auto-indent**, smart auto-close and **HTML/XML tag auto-close**.
- **Function List** outline dock and **parameter calltips** for C-like/Python
  code.
- **Line tools**: delete/duplicate/move lines, join & split, sort, remove
  duplicates, case conversion, trim whitespace, tabify/untabify.
- **Column editor** (repeated text or numbered columns), **word completion**
  (Ctrl+Space) and per-language **comment/uncomment**.
- **Column / multi-cursor editing** mode.
- **Macros**: record, save and replay editing sequences.
- **Plugin runtime** for **Lua** scripts and **native** shared libraries through
  a small, stable C ABI, plus an interactive **Lua script console** (REPL).
- **Tools**: Base64/URL/HTML/Unicode/Rot13/text↔hex conversions and MD5/SHA
  digests of the selection or document; **Copy File Name/Path**; read-only
  detection with an **R/O** status indicator and a **Read-Only Mode** toggle.
- **Eight themes** (light, dark, Solarized ×2, Monokai, Dracula, Nord, Gruvbox)
  with per-language style mapping, plus a **Preferences** dialog.
- **Window** menu with `Alt+1..9` document jumps, split orientation, **full
  screen** (F11), always-on-top and a **Run...** (F5) command runner.
- **Explorer dock**, **Document Map** minimap, **Function List** and
  **Character Map** docks, and **floating windows**
  (detach/reattach tabs, `Ctrl+Alt+D` to detach).
- **Export to HTML/RTF** and **print** with highlighting, plus an **Insert** menu
  for date/time, GUIDs and the file name/path.
- **Session restore** and a recent-files list.

## Building

### Dependencies

- CMake ≥ 3.21
- Ninja (recommended) or Make
- A C++20 compiler (GCC ≥ 12 or Clang ≥ 15)
- Qt 6 (Core, Gui, Widgets, PrintSupport, Core5Compat, Test)
- Python 3 (used once to generate the Scintilla Qt binding)
- Lua 5.4 or 5.5 (optional; enables the Lua plugin runtime)

On Arch Linux:

```sh
sudo pacman -S --needed base-devel cmake ninja qt6-base lua python
```

On Debian/Ubuntu:

```sh
sudo apt install build-essential cmake ninja-build python3 \
  qt6-base-dev qt6-base-dev-tools libgl1-mesa-dev liblua5.4-dev
```

Scintilla and Lexilla are **not** system dependencies: PadForge fetches pinned
source archives (`Scintilla 5.6.7`, `Lexilla 5.5.4`) with CMake `FetchContent`
at configure time and generates the Qt binding automatically. The first
configure therefore needs network access.

### Configure, build and test

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

If another Qt is installed, point CMake at the right one with
`-DCMAKE_PREFIX_PATH=/path/to/qt6`.

### Run

```sh
./build/src/gui/padforge [files...]
```

## CMake options

| Option | Default | Meaning |
| --- | --- | --- |
| `PADFORGE_BUILD_TESTS` | `ON` | Build the unit and smoke tests |
| `PADFORGE_ENABLE_LUA` | `ON` | Enable the Lua plugin runtime |
| `PADFORGE_USE_SYSTEM_SCINTILLA` | `OFF` | Skip FetchContent and build the core library only (fast core iteration; the GUI needs Scintilla) |

## Plugins

Drop Lua files into `~/.config/PadForge/plugins/` and choose
**Plugins → Reload Plugins**. Six example plugins ship in [`plugins/lua`](plugins/lua)
(word count, trim whitespace, sort/number/reverse lines, remove duplicates).

The full host API, plus how to build a native plugin, is documented in
[`plugins/README.md`](plugins/README.md) and [`docs/plugin-guide.md`](docs/plugin-guide.md).

## Documentation

- [`docs/install.md`](docs/install.md) — installation on popular distributions
- [`docs/features.md`](docs/features.md) — feature overview and comparison
- [`docs/keybindings.md`](docs/keybindings.md) — default shortcuts
- [`docs/plugin-guide.md`](docs/plugin-guide.md) — writing Lua and native plugins
- [`PLAN.md`](PLAN.md) — design notes, milestones and prior-art research
- [`CONTRIBUTING.md`](CONTRIBUTING.md) — how to build, test and contribute

## Packaging

See [`packaging/README.md`](packaging/README.md) for DEB, RPM, Arch, AppImage
and Flatpak recipes and the generic `cpack` tarball.

## License

PadForge is released under the **GNU General Public License, version 3 or
later** (GPL-3.0-or-later). See [`LICENSE`](LICENSE).

Scintilla and Lexilla are distributed under the permissive Historical
Permission Notice and Disclaimer (HPND) license; their notices are preserved by
their upstream sources. See [`NOTICE`](NOTICE) for details.
