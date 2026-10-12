# Changelog

All notable changes to PadForge are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Seven more themes: Solarized (light/dark), Monokai, Dracula, Nord and
  Gruvbox, implemented as per-role color overrides.
- Comment tokens per language, powering line/block comment toggles
  (`TextOps::toggleLineComment`/`toggleBlockComment`).
- `TextOps` core module: line ops (duplicate/delete/join/split/sort/dedupe),
  whitespace trimming, case conversion, tabify/untabify, blank-line removal and
  column editing with numbers.
- `AppSettings` persistence via `QSettings` (`Settings.{h,cpp}`) covering theme,
  font, tab width, word wrap, margins, whitespace/EOL/guide display, edge line,
  auto-completion and bracket auto-close.
- `EditorWidget` operations: full-line transformations over the current
  selection, bookmarked-line copy/cut/delete/paste/invert, display toggles,
  font/tab settings, word completion (Ctrl+Space) and a right-click context
  menu.
- New dialogs: `PreferencesDialog`, `ColumnEditorDialog`, `GotoLineDialog`,
  `DocumentSummaryDialog` and `RunDialog` (F5).
- Extended menus: Edit transformation and bookmark menus, Settings menu,
  Window menu with document list and `Alt+1..9` jumps, split orientation,
  full screen (F11) and always-on-top.
- `test_textops` unit suite; smoke tests for transformations and bookmarks.
- **Explorer dock**: folder-as-workspace file tree with follow-active-file,
  auto-opening on activation and Next/Previous subtle navigation.
- **Floating windows**: detach the current tab (`Ctrl+Alt+D`), reattach from the
  window itself, and merge all floating windows back into the main window.
- **Export to HTML/RTF** with syntax highlighting, plus **print** and **print
  preview** styled after the active theme via `Qt6::PrintSupport`.
- **Insert menu**: date and time in three formats, GUIDs in five shapes, and the
  current file name/path.
- **Language menu** now lists every registered language with a check reflecting
  the active tab's language, switching it per tab.
- **Document Map dock**: clickable minimap rendering per-line styles, the visible
  viewport and the caret line.
- **Change-history markers**: red/green margin indicators for changed lines since
  load, shown by default and toggleable in Preferences (`editor/showChangeHistory`).
- **Lua Script Console** dock: an interactive REPL over the shared plugin Lua
  state (`PluginHost::evaluateLua`, expression-first with statement fallback),
  command history, `print(...)` capture via a new `PluginHost` output sink, and
  error reporting.
- **File permission detection**: opening a non-writable file enables read-only
  mode automatically; a **R/O** status-bar indicator and a **Read-Only Mode**
  File-menu toggle manage it, and the state is saved/restored in the session.
- **File menu**: **Copy File Name** and **Copy Full Path** actions.
- **Tools menu** with a **Convert** submenu (Base64, URL, HTML entities, Unicode
  escape, Rot13, text↔hex) and a **Hash / Digest** submenu (MD5, SHA-1/256/384/512),
  acting on the selection or the whole document (`TextOps` conversion helpers).
- **Function List** dock (View > Function List): a document outline parsed from
  the active buffer via the new `OutlineParser` core module (C-like, Python,
  shell and Markdown), refreshed as you type, with click-to-jump.
- **Character Map** dock (View > Character Map): a browsable grid of Unicode
  blocks that inserts the selected glyph at the caret.
- **Auto-close HTML/XML tags**: typing `>` inserts the matching closing tag,
  skipping void elements, comments and declarations; toggleable in Preferences
  (`editor/autoCloseTags`).
- **Parameter calltips**: typing `(` after a function known to the outline shows
  its signature via `SCI_CALLTIPSHOW`; toggleable in Preferences
  (`editor/calltips`).
- `EditorWidget::textChanged` signal emitted on every document modification.
- `test_outline` unit suite and smoke tests for the Function List and Character
  Map panels.
- **Backup on save** (`editor/backupOnSave`, Preferences > Editing): when
  enabled, saving over an existing file first copies the previous contents to
  `<name>.bak` (`saveFileAs`).
- **Periodic snapshots + crash recovery**: a 20-second timer snapshots modified
  and unsaved documents through the new `BackupManager` core module (per-entry
  JSON in the user data directory); on next launch after an unclean exit PadForge
  offers to restore them, and a clean quit clears the snapshots
  (`backup/enabled`).
- **Synchronized Scrolling** (`view/syncScroll`): keeps the two split-view tab
  groups' top visible line in lockstep via a new `EditorWidget::scrolled()`
  signal and `setFirstVisibleLine`.
- **Function/symbol completion** (`Ctrl+Enter`): autocompletion fed by the
  document outline (classes, functions) with a word-Completion fallback
  (`EditorWidget::triggerSymbolCompletion`).
- **Save a Copy As...** (`Ctrl+Alt+S`): writes the current contents to a new path
  without touching the document's path or save state (`saveCopyAs`).
- `test_backup` unit suite and smoke tests for `.bak`-on-save, save-copy and
  top-line scrolling (14 smoke tests).
- **Compare Documents...** (Tools menu): opens two documents side by side in the
  split view, marks changed lines (removed/added) with per-side highlight
  indicators via a new `Diff` core module, enables synchronized scrolling for
  the comparison and reports change counts in the status bar (`next/previous
  difference`: F7 / Shift+F7, `Clear Compare Highlight`).
- **Find All in Current** / **Find All in Open Documents** in the Find dialog
  with results grouped per document in the new **Search Results** dock
  (`SearchResultsPanel`), click-to-jump included (View menu toggle).
- `TextOps` line-address helpers (`lineNumberAt`, `lineStartAt`, `lineContent`)
  powering the results panel columns.
- `test_diff` unit suite (LCS-based line diff with reconstruction round-trips),
  `test_textops` line-address tests and smoke tests for compare markers and the
  results panel (16 smoke tests).
- **Tab context menu** (right-click a tab): Close, Close All But This, Close All
  to the Left/Right, Close All, Save, Save As, Reload from Disk, Rename, Copy
  Full Path, Open Containing Folder and Move to Other View; **middle-click** a tab
  to close it.
- **Rename...** (File menu and tab menu) renames the file on disk and updates its
  tab (an untitled document is saved under the new name); **Open Containing
  Folder** reveals the current file in the system file manager.
- **Close All** and **Close All But This** in the File menu, and **Sort Tabs**
  (by name or path) in the Window menu.
- Smoke tests for rename, close-all/close-others, tab sorting and the modified
  tab-title marker (20 smoke tests).

### Fixed

- Tab operations (title updates, the modified `*` marker, close, Move to Other
  View, and the document map/function-list source tracking) resolved the owning
  tab widget from a page's `parentWidget()`, which is the `QTabWidget`'s internal
  `QStackedWidget`; they now resolve it by index (`tabWidgetFor`). Tab titles,
  the modified marker and cross-view moves now work reliably.

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
