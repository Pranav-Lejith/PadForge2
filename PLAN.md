# PadForge — Project Plan

> A fast, native **Linux** text editor that matches the **Notepad++ workflow**.
> Built with C++20, Qt 6 (Widgets) and the Scintilla/Lexilla editing engine.

This document records the design decisions, the study of the upstream Notepad++
project, a survey of existing Linux ports, and how PadForge differs.

---

## 1. Study of upstream Notepad++

The upstream tree was cloned to `upstream/` (read-only, git-ignored) and studied:

| Concern | Upstream location | Notes |
|---|---|---|
| License | `upstream/LICENSE` | GPL v3 (with clarifications) |
| Plugin API | `PowerEditor/src/MISC/PluginsManager/PluginInterface.h` | `extern "C"` exports: `setInfo`, `getName`, `getFuncsArray`, `beNotified`, `messageProc`, `isUnicode`; `NppData`, `FuncItem`, `ShortcutKey`. |
| Plugin messages | `.../PluginsManager/Notepad_plus_msgs.h` | `NPPM_*` messages (`WM_USER+1000`) and `NPPN_*` notifications. |
| Session model | `PowerEditor/src/Parameters.h` (`Session`, `sessionFileInfo`), `Parameters.cpp` (`getSessionFromXmlTree`, `loadSession`, `writeSession`) | XML (`session.xml`) under the user config dir; tracks per-file position, selection, language, encoding, fold states, marks. |
| Scintilla wrapper | `PowerEditor/src/ScintillaComponent/ScintillaEditView.*` | Thin `execute(SCI_*)` wrapper over a Win32 Scintilla control. |
| Encoding detection | `PowerEditor/src/Utf8_16.{h,cpp}`, `Buffer.cpp:detectCodepage` (bundled **uchardet**) | BOM detection (UTF-8/UTF-16 LE/BE), Win32 `IsTextUnicode`, then uchardet fallback; a large `EncodingMapper` (1250–1258, ISO-8859-*, DOS, Big5, GB2312, Shift_JIS, EUC-KR, KOI8, …). |
| EOL handling | `MISC/Common/NppConstants.h` (`EolType`) | Windows / Unix / Mac; converted on load/save. |
| Languages | `PowerEditor/src/langs.model.xml` | 95 language entries, each mapped to a Lexilla lexer. |
| Build | `PowerEditor/src/CMakeLists.txt`, `gcc/makefile` | Windows-only (`IF(WIN32)`), links static `libscintilla` + `liblexilla`, bundled Boost.Regex. |

**Takeaways that shaped PadForge**

1. Notepad++ is a Win32 application: its UI layer (docking panels, `HWND`
   plumbing, DarkMode via IAT hooking) is not portable. Only the *workflow* is
   reusable as a specification.
2. The editing semantics (encoding detection, EOL, lexer mapping, session
   contents) are the valuable parts to reproduce.
3. Plugins are Windows DLLs with a `__declspec(dllexport)` ABI — impossible to
   reuse on Linux. PadForge designs a portable plugin model instead (see §6).

## 2. Existing Linux ports / equivalents

| Project | Toolkit | License | Status | How PadForge differs |
|---|---|---|---|---|
| **Notepadqq** | C++/Qt + **QScintilla** | GPL-3.0 | **Unmaintained** (README asks for maintainers) | PadForge uses current Scintilla/Lexilla (not the stale QScintilla 3.10 base), is maintained, and adds a Lua plugin API. |
| **NotepadNext** | C++/Qt6 + vendored **Scintilla/Lexilla** + Advanced Docking | GPL-3.0 | Actively maintained | Closest relative. PadForge targets a lean, native Qt6 build focused on the Notepad++ workflow, ships first-class packaging for 5 formats, and exposes a Lua scripting layer. |
| **Notepad-- (ndd)** | C++/Qt5 + QScintilla | GPL-3.0 | Maintained (CN-centric) | PadForge is Qt6-only, English-first, no QScintilla. |
| **Nextpad++ / Notetux++** | C/GTK4 + Scintilla | GPL-3.0 | New (2026), claim-heavy | PadForge uses Qt6 (native look on KDE/GNOME), a stable build, and a documented plugin API. |
| **SciTE** | C++/GTK + Scintilla | HPND | Maintained | SciTE is a config-driven editor, not a tabbed Notepad++-style app. |
| Notepad++ under **Wine** | — | GPL-3.0 | Works but non-native | PadForge is a genuinely native Linux binary. |

**Conclusion:** no maintained project is a *native, Qt6, current-Scintilla,
Lua-scriptable* Notepad++-workflow editor with the packaging/CI breadth PadForge
targets. That is the niche PadForge fills.

## 3. Engine choice: Scintilla/Lexilla vs QScintilla

| | **Scintilla + Lexilla (ScintillaEdit)** | **QScintilla** |
|---|---|---|
| License | **HPND** (permissive, GPL-compatible) | GPL-3 **or** commercial |
| Engine version | current (5.6.x / Lexilla 5.5.x) | based on old Scintilla 3.10 |
| Lexers | Lexilla (140+) | bundled, older |
| Qt binding | in-tree `qt/ScintillaEdit`, built from source | ready-made class library |
| Integration | CMake wrapper (documented pattern) | Qt `qmake` / system package |

**Decision:** use **Scintilla 5.6.7 + Lexilla 5.5.4** compiled from source with
the in-tree Qt `ScintillaEdit` binding. Rationale: permissive license, modern
engine, access to all Lexilla lexers, and full control over the widget. The
`ScintillaEdit` header is generated from `WidgetGen.py` at configure time.

## 4. Architecture

```
padforge (GUI executable)
├── padforge_core        (static lib, Qt Core only → unit-testable, headless)
│   ├── Encoding         BOM/EOL/charset detection, conversion
│   ├── SearchEngine     find/replace (plain, extended, regex), find-in-files
│   ├── Session          session + recent-files model & JSON persistence
│   ├── LanguageManager  extension/shebang → Lexilla lexer, 30+ languages
│   ├── Theme            dark/light palettes + editor style definitions
│   ├── Macro            recordable command model, playback
│   └── PluginHost       Lua plugin runtime + PadForge C plugin API
├── src/gui
│   ├── MainWindow       menus, toolbar, docking, split view, tabs
│   ├── EditorWidget     ScintillaEdit wrapper (per document)
│   ├── FindReplaceDialog / FindInFilesPanel / ResultsPanel
│   └── dialogs (encoding, EOL, theme, about)
└── plugins/lua          sample Lua plugins
```

`padforge_core` deliberately depends only on `Qt6::Core` so the search,
encoding and session logic can be unit-tested without a display server. The GUI
layer is smoke-tested headlessly via `QT_QPA_PLATFORM=offscreen`.

## 5. Milestones

### Milestone 1 — Editor core
Tabbed editing; open/save with encoding detection (UTF-8/UTF-16/legacy/BOM);
line-ending detection & conversion (LF/CRLF/CR); large-file handling
(streaming read, no-op/read-only safeguards above a threshold); dark & light
themes.

### Milestone 2 — Language intelligence
Syntax highlighting for 30+ languages via Lexilla; code folding; auto-indent
(smart indent after `{`, `:` etc.); bracket matching & auto-close.

### Milestone 3 — Power editing
Multi-cursor / multi-selection; rectangular (column) mode; find/replace with
regex; find-in-files with a results panel; bookmarks.

### Milestone 4 — Workflow & extensibility
Session restore; recent files; split view; macro record & playback; a **Lua
plugin API** with two sample plugins.

## 6. Plugin model (portable, not the Win32 DLL ABI)

The Notepad++ plugin ABI cannot be reused on Linux. PadForge defines two layers:

1. **Lua plugins** (primary, the shipped sample plugins): a restricted Lua state
   gets a `padforge` table with an editor/document API. Plugins live in
   `~/.config/PadForge/plugins/*.lua` and register menu commands at load time.
2. **Native plugins** (optional): a small C ABI (`padforge_plugin.h`) with
   `padforge_plugin_init(PadForgeHost*)`, loaded via `QLibrary`. The host
   interface exposes the same calls the Lua bridge uses, so the API surface is
   identical.

The API intentionally mirrors the *spirit* of Notepad++'s `NppData`/`NPPN_*`
model (host handle, current document, notifications) without copying its
symbols.

## 7. Build, packaging, CI

- **Build:** CMake ≥ 3.21, C++20, Qt6 (`Core Gui Widgets PrintSupport Core5Compat Test`),
  Scintilla/Lexilla fetched and built from pinned sources.
- **Packaging:** `.deb` (Debian/Ubuntu), `.rpm` (Fedora), `PKGBUILD` (Arch),
  AppImage, Flatpak.
- **CI:** GitHub Actions runs the unit + headless smoke tests, then builds each
  package in `ubuntu`, `fedora` and `archlinux` containers and attaches the
  artifacts to a GitHub Release on version tags.

## 8. Licensing

PadForge is **GPL-3.0-or-later**. Scintilla/Lexilla are HPND (permissive,
GPL-compatible); Qt6 is LGPLv3 (dynamic linking). Notepad++ is credited as
inspiration only — no code, assets, name, or logo (chameleon) are reused.
