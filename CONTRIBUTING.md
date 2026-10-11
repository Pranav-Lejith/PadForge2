# Contributing to PadForge

Thanks for your interest in PadForge! This document explains how to build the
project, run the tests and submit changes.

## Getting set up

1. Install the dependencies listed in [`README.md`](README.md#dependencies).
2. Configure and build:

   ```sh
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```

3. Run the tests:

   ```sh
   QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
   ```

The GUI tests run headlessly with `QT_QPA_PLATFORM=offscreen`, so they work on
machines without a display.

## Project layout

```
src/core/   platform-independent library: encoding, search, sessions,
            languages, themes, macros and the plugin host (no Qt Widgets)
src/gui/    Qt 6 Widgets front-end built on top of src/core + Scintilla
tests/      unit tests and the headless UI/plugin smoke tests
plugins/    example Lua plugins and a native plugin example
cmake/      Scintilla/Lexilla FetchContent integration
packaging/  distribution recipes (Arch, RPM, DEB, AppImage, Flatpak)
resources/  desktop entry, icon and AppStream metadata
```

The core library is deliberately independent of the widget layer and depends
only on `Qt6::Core`/`Qt6::Gui`, which keeps it easy to unit test. Prefer adding
logic there and keeping the GUI thin.

## Coding style

- C++20, four-space indents, 100-column soft limit.
- The repository ships a `.clang-format`; run `clang-format -i` on changed files.
- No third-party dependencies beyond Qt, Scintilla/Lexilla and (optionally) Lua.
- Do not add comments unless they explain non-obvious reasoning. Match the
  surrounding style.

## Tests

- Add or update tests under `tests/` for behaviour you change.
- New plugins should be exercised through the `PluginHost` API.
- Keep the suite fast; it runs on every pull request in CI.

## Commits and pull requests

- Write focused commits with an imperative summary, optionally scoped, e.g.
  `feat(core): add UTF-16 detection` or `fix(gui): keep caret on reload`.
- One logical change per pull request where practical.
- Update `CHANGELOG.md` under `## [Unreleased]` for user-visible changes.
- Make sure the full build and the test suite pass before opening a PR.

## Licensing

By contributing you agree that your contributions are licensed under the
GNU GPL v3 or later, the same license as the project. Do not copy code from
Notepad++ or other GPL-incompatible sources; Scintilla and Lexilla are used
under their permissive HPND license and are fetched from upstream.
