# Features

PadForge aims to reproduce the *workflow* of Notepad++ on modern Linux, without
being a port of its code.

## Editing

- Tabbed documents with drag-reorderable tabs and a closable tab bar.
- Optional **split view** with a second tab group; move documents between the two.
- Undo/redo, cut/copy/paste, select all, incremental search highlighting.
- **Bookmarks** (toggle, next/previous, clear all) with a dedicated margin.
- **Code folding** and a fold margin, configurable on load.
- **Line numbers**, **brace matching**, **current-line highlight** and a
  configurable **edge line**.
- **Auto-indent** and **smart auto-close** of quotes and brackets.
- **Column / multi-cursor** editing mode.
- **Large-file mode** that temporarily disables expensive features.

## Languages

- 50+ languages registered out of the box, each mapped to a Lexilla lexer.
- Automatic detection by file name, extension, shebang or content.
- Manual language selection from the **Language** menu.

## Files and encodings

- UTF-8, UTF-8 with BOM, UTF-16 LE/BE, UTF-32 LE/BE and Latin-1.
- Automatic BOM and charset detection, including a BOM-less UTF-16 heuristic.
- Line endings: LF, CRLF and CR, detected per file and convertible.
- Reopen from disk and a recent-files list.

## Search

- **Find / Replace** in plain, whole-word, extended (`\n`, `\t`, `\xHH`) and
  regular-expression modes, with wrap-around, search-up and `.` matching newline.
- Count matches and mark all matches in the document.
- **Find in Files** across a directory tree with name filters and clickable
  results.

## Automation

- **Macros**: record, store and replay editing sequences.
- **Plugins**: a small, stable C ABI plus a sandboxed **Lua** runtime. Both
  plugin styles share the same host capability surface.

## Appearance

- Light and dark themes with per-language style mapping.
- Configurable zoom.

## How PadForge compares

| | PadForge | Notepad++ | NotepadNext | Notepadqq | SciTE |
| --- | --- | --- | --- | --- | --- |
| Platform | Linux | Windows | Windows/Linux/macOS | Linux | cross-platform |
| Toolkit | Qt 6 + Scintilla 5 | Win32 + Scintilla 5 | Qt 6 + Scintilla 5 | Qt 5 + QScintilla | Win32/GTK |
| License | GPL-3.0-or-later | GPL-3.0-or-later | GPL-3.0 | GPL-3.0 | HPND |
| Language | C++20 | C++ | C++ | C++/Qt | C++ |
| Scripting | Lua + native C ABI | plugins (C++) | plugins (C++) | plugins (C++) | Lua |
| Status | active (this project) | active | active | unmaintained | active |

PadForge uses Scintilla and Lexilla directly rather than QScintilla, keeping the
editing core at the current upstream version and the license permissive for the
editing components.
