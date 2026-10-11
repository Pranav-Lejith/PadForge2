# Changelog

All notable changes to PadForge are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.0] - 2026-10-11

Initial development release.

### Added

- CMake build system that fetches pinned Scintilla 5.6.7 and Lexilla 5.5.4,
  generates the Qt binding with `WidgetGen.py`, and builds a static core.
- Core library (`padforge_core`):
  - encoding/BOM/EOL detection and conversion (`Encoding`)
  - plain, whole-word, extended and regex search plus directory search
    (`SearchEngine`, `FileSearcher`)
  - session and recent-files persistence (`Session`, `RecentFiles`)
  - language registry mapping 50+ languages to Lexilla lexers (`LanguageManager`)
  - light/dark themes with per-language style mapping (`Theme`)
  - macro recording and library persistence (`Macro`)
  - a small, stable plugin C ABI and host with a sandboxed Lua runtime
    (`PluginApi`, `PluginHost`)
- GUI (`padforge_gui`, `padforge`):
  - `EditorWidget` wrapping ScintillaEdit: open/save, encoding and EOL control,
    language and theme application, bookmarks, folding, multi-cursor mode and
    incremental highlighting
  - `MainWindow`: tabbed documents, split view, menus/toolbar/status bar,
    session and recent-file restore, macros and plugin menus
  - `FindReplaceDialog` and `FindInFilesPanel`
- Six example Lua plugins and a native example plugin.
- Unit tests for encoding, search, session and macros, a plugin test that loads
  the Lua samples, and a headless UI smoke test.
- Packaging recipes for Arch (`PKGBUILD`), RPM (`.spec`), Debian (`build-deb.sh`),
  AppImage and Flatpak, plus desktop entry, icon and AppStream metadata.
- GitHub Actions CI covering the full build and the core-only build.

[Unreleased]: https://github.com/padforge/padforge/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/padforge/padforge/releases/tag/v0.1.0
