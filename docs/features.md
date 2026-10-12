# Features

PadForge aims to reproduce the *workflow* of Notepad++ on modern Linux, without
being a port of its code.

## Editing

- Tabbed documents with drag-reorderable tabs and a closable tab bar.
- **Tab context menu** (right-click a tab): Close, Close All But This, Close All
  to the Left/Right, Close All, Save, Save As, Reload from Disk, Rename, Copy
  Full Path, Open Containing Folder, Move to Other View. **Middle-click** a tab
  to close it; **Sort Tabs** (Window menu) orders tabs by name or path.
- **Rename** a document (File menu or tab menu) renames the file on disk and
  updates its tab; an untitled document is saved under the new name instead.
- **Open Containing Folder** reveals the current file in the system file manager.
- **Close All** and **Close All But This** from the File menu.
- Optional **split view** with a second tab group; move documents between the two.
- Undo/redo, cut/copy/paste, select all, incremental search highlighting.
- **Bookmarks** (toggle, next/previous, clear, invert) with a dedicated margin.
- **Bookmarked-line operations**: copy, cut, delete and paste-to, straight from
  the Edit menu.
- **Code folding** and a fold margin, configurable on load.
- **Line numbers**, **brace matching**, **current-line highlight** and a
  configurable **edge line**.
- **Line operations**: delete, duplicate, move up/down current line, join and
  split lines, sort (ascending/descending/case-insensitive), remove duplicate or
  empty lines, tabify/untabify.
- **Case conversion**: upper, lower, title and sentence case.
- **Whitespace tools**: trim trailing, leading or both.
- **Comment/uncomment** toggles for line and block comments, driven by the active
  language's comment tokens.
- **Column editor**: insert repeated text or consecutive, zero-padded numbers
  across a line range.
- **Word completion** (Ctrl+Space) fed by words already present in the document.
- **Function/symbol completion** (Ctrl+Enter) fed by the document outline
  (function, class, struct, interface names), falling back to document words in
  languages without an outline.
- **Auto-indent** and **smart auto-close** of quotes and brackets.
- **Auto-close HTML/XML tags**: typing `>` after an opening tag inserts the
  matching closing tag, skipping void elements and comments.
- **Parameter calltips**: typing `(` after a function defined in the current
  document shows its signature (C-like and Python languages).
- **Column / multi-cursor** editing mode.
- **Large-file mode** that temporarily disables expensive features.
- **Change-history markers**: green/red margin indicators flag lines modified since
  the document was loaded (toggleable in Preferences).

## Insert

- **Date and time** in three ready formats (`yyyy-MM-dd HH:mm:ss`,
  `yyyy-MM-dd`, `HH:mm:ss`) from the Insert menu.
- **GUID/UUID** generation in five shapes: lowercase, uppercase, uppercase with
  braces, plain, and compact.
- **File name** and **full path** of the current document.

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
- **Find All** in the current document or in all open documents from the Find
  dialog: matches open in the **Search Results** dock, grouped per document with
  click-to-jump.

## Export and printing

- **Export** the current document with syntax highlighting to **HTML** or **RTF**.
- **Print** and **print preview** with the printed output styled after the active
  theme.

## Automation

- **Macros**: record, store and replay editing sequences.
- **Plugins**: a small, stable C ABI plus a sandboxed **Lua** runtime. Both
  plugin styles share the same host capability surface.
- **Script Console** (`View > Script Console` or `Tools > Script Console`): an
  interactive **Lua REPL** sharing the plugin runtime, with command history
  (Up/Down), `print(...)` output and error reporting. The same `padforge` table
  available to plugins (document text, line count, go-to-line, language, …) is
  available at the prompt.

## Tools

- **Convert** submenu: Base64 encode/decode, URL percent-encode/decode, HTML
  entity encode/decode, Unicode (`\uXXXX`) escape/unescape, Rot13, and
  text↔hex.
- **Hash / Digest** submenu: MD5, SHA-1, SHA-256, SHA-384 and SHA-512 of the
  selection (or the whole document when nothing is selected), copied to the
  clipboard.
- Conversions act on the current selection, or on the entire document when there
  is no selection.
- **Copy File Name** / **Copy Full Path** for the active document.
- **Read-Only Mode** toggle plus automatic read-only detection for files that are
  not writable, shown as **R/O** in the status bar and preserved across sessions.
- **Save a Copy As...** (Ctrl+Alt+S) writes the current contents to a new file
  without changing the document's path or save state.

## Backup and recovery

- **Backup on save** (Preferences > Editing): when enabled, saving over an
  existing file first shunts the previous contents into `<name>.bak`.
- **Periodic snapshots**: every 20 seconds, modified and unsaved documents are
  snapshotted into the per-user data directory (off by default; enabled in
  Preferences > Backup).
- **Crash recovery**: on the next launch after an unclean exit, PadForge offers
  to restore every snapshotted document as recovered editing tabs. A clean quit
  clears the snapshots.

## Appearance

- Eight built-in themes: light and dark, Solarized (light/dark), Monokai,
  Dracula, Nord and Gruvbox, with per-language style mapping.
- Configurable zoom.
- **Preferences** dialog and a **Settings** menu persist editor options
  (font, tab width, word wrap, margins, whitespace/EOL display, edge line,
  completion, bracket and tag auto-close, calltips, backup on save, periodic
  snapshots, synchronized scrolling) and recent-files list across sessions.

## Window management

- **Window** menu lists every open document with 1..9 quick-switch shortcuts;
  next/previous tab (Ctrl+Tab / Ctrl+Shift+Tab).
- **Floating windows**: detach the current tab (Ctrl+Alt+D) into its own window
  and reattach it from there; merge all floating windows back into the editor.
- **Explorer dock** (View > Explorer): folder-as-workspace file tree; opening a
  file opens it in the editor, and the Follow button keeps the tree synced to the
  active document.
- **Document Map** dock (View > Document Map): a clickable minimap of the whole
  file showing per-line styles, the visible viewport and the caret line.
- **Function List** dock (View > Function List): a document outline of classes,
  functions and (for Markdown) headings built from the active buffer; double-click
  an entry to jump to its line. Refreshes as you type.
- **Character Map** dock (View > Character Map): a browsable grid of Unicode
  blocks; double-click a glyph to insert it at the caret.
- Split **orientation** (horizontal or vertical) for the second tab group.
- **Synchronized Scrolling** (View menu): keeps the top visible line of both tab
  groups in lockstep for side-by-side diffing (Views side by side recommended).
- **Compare Documents...** (Tools menu, F7/Shift+F7 to jump between changes):
  opens two documents side by side in the split view, highlights the changed
  lines on each side (removed in one, added in the other), turns on synchronized
  scrolling, and summarizes the change counts in the status bar. Clearing the
  compare restores your scroll-sync preference. Flat files and documents with no
  shared prefix are diffed line-wise via the `Diff` core module.
- **Search Results** dock (View > Search Results): results from Find All group
  per document with line/column columns; double-click a row to jump there.
- **Full screen** (F11) and **always on top**.
- **Run...** (F5) executes a shell command, expanding `$(FULL_CURRENT_PATH)` /
  `$(CURRENT_DIRECTORY)` / `$(CURRENT_FILENAME)` style variables, and shows
  captured output.
- **Go to line**, **Document summary** (words, lines, characters, bytes) with
  copy-to-clipboard.

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
