# PadForge

<div align="center">

![PadForge Logo](resources/icons/org.padforge.PadForge.svg)

**A fast, native text editor inspired by the Notepad++ workflow.**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/)
[![Qt 6](https://img.shields.io/badge/Qt-6-blue.svg)](https://www.qt.io/)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](#building-from-source)
[![Tests](https://img.shields.io/badge/tests-10%2F10-brightgreen.svg)](#testing)

</div>

PadForge brings the familiar, efficient editing experience popularised by Notepad++ to Linux and macOS. Built with **C++20**, **Qt 6 Widgets**, and the [Scintilla](https://www.scintilla.org/) editing component with [Lexilla](https://www.scintilla.org/Lexilla.html) for syntax highlighting, PadForge is lightweight, fast, and designed for power users.

> PadForge is an independent project. It is *inspired by* Notepad++ but is not affiliated with, endorsed by, or derived from the Notepad++ source. See [`NOTICE`](NOTICE).

## Table of Contents

- [Features](#features)
- [Screenshots](#screenshots)
- [Installation](#installation)
  - [AppImage](#appimage)
  - [Debian and Ubuntu](#debian-and-ubuntu)
  - [macOS](#macos)
  - [Build from Source](#build-from-source)
- [Building from Source](#building-from-source)
- [Usage](#usage)
- [Testing](#testing)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [License](#license)
- [Acknowledgments](#acknowledgments)

## Features

### Core Editing
- **Tabbed & Split View**: Work with multiple documents side-by-side with synchronized scrolling
- **Encoding Aware**: UTF-8 (with/without BOM), UTF-16 LE/BE, UTF-32 LE/BE and Latin-1 with automatic detection
- **Line-Endings**: LF, CRLF, and CR with per-file detection and conversion
- **Large-File Mode**: Automatically disables expensive features for improved performance on large files
- **Column / Multi-Cursor Editing**: Full rectangular selection and multi-cursor support via Scintilla

### Search & Navigation
- **Find & Replace**: Plain text, whole-word, extended (`\n`, `\t`, `\xHH`) and regex modes with wrap-around, counting, and "mark all"
- **Find in Files**: Search across directory trees with filters and jump-to-hit navigation
- **Find All**: Search current or all open documents with results grouped in the clickable Search Results dock
- **Go to Line**, **Brace Matching**, and **Code Folding** with margin controls

### Syntax & Intelligence
- **50+ Languages**: Extensive lexer support via Lexilla with auto-detection by extension, shebang, or content
- **Syntax Highlighting**: Rich, themeable highlighting with per-language styling
- **Function List**: Document outline dock showing classes, functions, and symbols
- **Parameter Calltips**: Intelligent signature hints for C-like and Python code
- **Auto-Completion**: Word completion (`Ctrl+Space`) and function/symbol completion (`Ctrl+Enter`)
- **Smart Auto-Close**: Brackets, quotes, and HTML/XML tags

### Productivity Tools
- **Bookmarks**: Mark and navigate important lines quickly
- **Line Operations**: Duplicate, move, join/split, sort, deduplicate, case conversion, trim whitespace, tabify/untabify
- **Column Editor**: Insert repeated text or numbered columns with padding control
- **Comment/Uncomment**: Language-aware line and block commenting
- **Macros**: Record, save, and replay custom editing sequences
- **Convert & Hash**: Base64, URL, HTML, Unicode, Rot13, text↔hex conversions plus MD5/SHA digests
- **Insert Tools**: Date/time (multiple formats), GUIDs (5 shapes), and current file name/path

### Workflow & Organization
- **Sessions**: Automatic save/restore of open documents, view state, and window layout
- **Recent Files**: Quick access to recently opened files
- **Explorer Dock**: Folder-as-workspace file tree with follow-active-file
- **Document Map**: Clickable minimap for quick navigation
- **Character Map**: Browse and insert Unicode characters
- **Floating Windows**: Detach tabs into separate windows and reattach with ease

### Backup & Recovery
- **Backup on Save**: Create `.bak` files when overwriting
- **Periodic Snapshots**: Every 20 seconds, modified unsaved documents are snapshotted to disk
- **Crash Recovery**: Restore unsaved work after an unclean exit
- **Compare Documents**: Side-by-side diff with change highlighting and F7/Shift+F7 navigation

### Customization & Extensibility
- **8 Beautiful Themes**: Light, Dark, Solarized (Light/Dark), Monokai, Dracula, Nord, and Gruvbox
- **Preferences Dialog**: Comprehensive settings for fonts, tabs, margins, display, and behavior
- **Plugin System**: Restricted **Lua** plugins and a stable **C ABI** for native plugins
- **Script Console**: Interactive Lua REPL with command history
- **Read-Only Mode**: Toggle read-only state with automatic detection for locked files

### Export & Integration
- **Export**: Save documents with syntax highlighting as HTML or RTF
- **Print & Print Preview**: Theme-aware printing
- **Run Command**: Execute external commands with F5
- **File Manager Integration**: Open containing folder, copy full path/filename

## Screenshots

*Coming soon — PadForge in action with different themes and layouts.*

## Installation

The source repository and release downloads are public. **You do not need a
GitHub account** to browse or download PadForge.

PadForge checks the public GitHub release for updates at most once per day. If a
new version is available, it shows an optional notification; choose **Ignore
this version** or close the notification to dismiss it. Choosing **View update**
opens the release page in your browser. PadForge never installs updates
automatically.

### AppImage

The AppImage is the easiest option on most Linux distributions. It bundles the
application's Qt and Lua runtime libraries, so you do not need to install the
build dependencies listed below. Download the latest public release, make it
executable, and run it:

```bash
curl -fL -o PadForge-x86_64.AppImage \
  https://github.com/Pranav-Lejith/PadForge2/releases/latest/download/PadForge-x86_64.AppImage
chmod +x PadForge-x86_64.AppImage
./PadForge-x86_64.AppImage
```

### Debian and Ubuntu

Download and install the `.deb` package. `apt` resolves and installs the
required system libraries automatically:

```bash
curl -fL -o PadForge-amd64.deb \
  https://github.com/Pranav-Lejith/PadForge2/releases/latest/download/PadForge-amd64.deb
sudo apt install ./PadForge-amd64.deb
```

These are public release assets; opening the [releases page](https://github.com/Pranav-Lejith/PadForge2/releases/latest)
does not require signing in. Arch users can use the signed pacman repository
below. Building from source is for contributors and requires the development
dependencies below.

### Arch Linux

Download the public repository key and configuration, then verify that the
displayed fingerprint matches
`812A 7B7D F519 5EEA 21E1 F8C9 0CAA 49C6 DDFE EF4D`:

```bash
curl -fLO https://pranav-lejith.github.io/PadForge2/padforge.gpg
curl -fLO https://pranav-lejith.github.io/PadForge2/padforge.conf
gpg --show-keys --with-fingerprint padforge.gpg
```

Trust the key and add the repository once:

```bash
sudo pacman-key --add padforge.gpg
sudo pacman-key --lsign-key 812A7B7DF5195EEA21E1F8C90CAA49C6DDFEEF4D
sudo install -Dm644 padforge.conf /etc/pacman.d/padforge.conf
grep -qF 'Include = /etc/pacman.d/padforge.conf' /etc/pacman.conf || \
  echo 'Include = /etc/pacman.d/padforge.conf' | sudo tee -a /etc/pacman.conf
```

Now install and update PadForge with pacman:

```bash
sudo pacman -Syu padforge
```

The repository is hosted publicly and does not require a GitHub account.

### macOS

PadForge's macOS build targets Apple Silicon. Download the disk image from the
[latest release](https://github.com/Pranav-Lejith/PadForge2/releases/latest)
without signing in to GitHub, then open `PadForge-macos-arm64.dmg` and drag
`PadForge.app` into Applications. Eject the mounted disk image when installation
is complete. These initial builds are ad-hoc signed; if macOS blocks the first
launch, Control-click the app in Finder and choose **Open**.

## Building from Source

### Requirements

- **CMake** ≥ 3.21
- **C++20** compiler (GCC 13+ or Clang 15+)
- **Qt 6** (Core, Gui, Network, Widgets, PrintSupport, Core5Compat, Test)
- **Ninja** (recommended) or Make
- **Python 3** (required for Scintilla/Lexilla WidgetGen)
- **Lua 5.4** (optional but recommended for plugin support)

#### Install Dependencies

**Debian/Ubuntu:**

```bash
sudo apt install cmake ninja-build g++ python3 qt6-base-dev qt6-base-dev-tools qt6-5compat-dev libgl1-mesa-dev liblua5.4-dev
```

**Fedora:**

```bash
sudo dnf install cmake ninja-build gcc-c++ python3 qt6-qtbase-devel qt6-qt5compat-devel lua-devel
```

**Arch Linux:**

```bash
sudo pacman -S cmake ninja gcc python qt6-base qt6-5compat lua54
```

**macOS (Homebrew):**

```bash
brew install cmake ninja qt lua@5.4
```

### Build Steps

```bash
git clone https://github.com/Pranav-Lejith/PadForge2.git
cd PadForge2
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DPADFORGE_ENABLE_LUA=ON -DPADFORGE_BUILD_TESTS=ON
cmake --build build --parallel
```

To build and test the macOS app bundle and create a disk image, run:

```bash
./packaging/macos/package-macos.sh
```

This script builds the Qt app bundle, includes the Lua runtime, runs the test
suite, and writes `dist/PadForge-macos-arm64.dmg`. The macOS CI job runs the
same script on an Apple Silicon runner.

### Install Locally

```bash
cmake --install build --prefix ~/.local
```

### Run

```bash
~/.local/bin/padforge  # or just `padforge` if ~/.local/bin is in PATH
```

## Usage

Launch PadForge from your app menu or terminal:

```bash
padforge [files...]
```

Open specific files:

```bash
padforge document.txt script.py README.md
```

Get help:

```bash
padforge --help
padforge --version
```

### Default Shortcuts (Highlights)

| Shortcut | Action |
| --- | --- |
| `Ctrl+N` | New Document |
| `Ctrl+O` | Open |
| `Ctrl+S` | Save |
| `Ctrl+Shift+S` | Save As |
| `Ctrl+Alt+S` | Save a Copy As |
| `Ctrl+F` | Find |
| `Ctrl+H` | Replace |
| `Ctrl+Shift+F` | Find in Files |
| `Ctrl+G` | Go to Line |
| `Ctrl+B` | Go to Matching Brace |
| `F7` / `Shift+F7` | Next/Previous Difference (Compare) |
| `Ctrl+Space` | Word Completion |
| `Ctrl+Enter` | Function/Symbol Completion |
| `Ctrl+Alt+D` | Detach Tab to Floating Window |
| `Ctrl+Tab` / `Ctrl+Shift+Tab` | Switch Documents |
| `F11` | Full Screen |

On macOS, the `Ctrl` modifier in this list uses the Command (`⌘`) key.
See [`docs/keybindings.md`](docs/keybindings.md) for the complete list.

## Testing

PadForge includes a comprehensive test suite (unit + headless smoke tests):

```bash
cd build
QT_QPA_PLATFORM=offscreen ctest --output-on-failure
```

All tests pass (10/10) covering encoding, search, sessions, macros, text operations, outline parsing, backups, diff, plugins, and smoke scenarios.

## Documentation

- [`docs/features.md`](docs/features.md) — Detailed feature overview and comparison
- [`docs/keybindings.md`](docs/keybindings.md) — Complete default keyboard shortcuts
- [`docs/install.md`](docs/install.md) — Installation instructions for various distributions
- [`docs/plugin-guide.md`](docs/plugin-guide.md) — Writing Lua and native plugins
- [`PLAN.md`](PLAN.md) — Design notes, milestones, and prior-art research
- [`CONTRIBUTING.md`](CONTRIBUTING.md) — Build, test, and contribution guidelines

## Contributing

Contributions are welcome! Please read [`CONTRIBUTING.md`](CONTRIBUTING.md) for details on our code of conduct, development setup, and the pull request process.

## License

PadForge is licensed under the **GNU General Public License v3.0 or later**. See [`LICENSE`](LICENSE) for the full text.

Scintilla and Lexilla are HPND-licensed (permissive, GPL-compatible). Qt 6 is LGPLv3. Lua is MIT-licensed (when enabled). See [`NOTICE`](NOTICE) for complete attribution.

## Acknowledgments

- **[Notepad++](https://notepad-plus-plus.org/)** — The original inspiration for this workflow
- **[Scintilla](https://www.scintilla.org/)** & **[Lexilla](https://www.scintilla.org/Lexilla.html)** — The rock-solid editing engine
- **[Qt](https://www.qt.io/)** — Cross-platform UI framework
- **All contributors** who help make PadForge better
