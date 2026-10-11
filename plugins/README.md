# PadForge plugins

PadForge ships a small, stable plugin surface that Lua scripts and native
shared libraries both share. There are two plugin styles.

## Where plugins live

Plugins are loaded from two directories at start-up (and when you pick
**Plugins → Reload Plugins**):

| Location | Purpose |
| --- | --- |
| `<AppConfigLocation>/plugins` (typically `~/.config/PadForge/plugins`) | User plugins |
| `<appdir>/plugins` | Plugins bundled with an installation |

Lua plugins are files ending in `.lua`; native plugins are `.so` files built as
MODULE libraries.

## Lua plugins (recommended)

A Lua plugin defines an optional global `setup()` function. Inside it you
register commands with `padforge.add_menu_item`. The available host functions
are exposed on the global `padforge` table:

| Function | Description |
| --- | --- |
| `padforge.get_text()` | Return the active document as a string |
| `padforge.set_text(s)` | Replace the active document with `s` |
| `padforge.insert_text(s)` | Insert `s` at the caret |
| `padforge.line_count()` | Number of lines in the active document |
| `padforge.current_line()` | 1-based caret line |
| `padforge.goto_line(n)` | Move the caret to line `n` |
| `padforge.set_language(id)` | Apply a lexer by language id (e.g. `"cpp"`) |
| `padforge.message(s)` | Show `s` in the status bar |
| `padforge.add_menu_item(name, fn)` | Add `name` to the Plugins menu; call `fn` when chosen |

`print(...)` is also available and forwards to the status bar.

### Example

```lua
local function setup()
    padforge.add_menu_item("Word Count", function()
        local text = padforge.get_text()
        local words = 0
        for _ in text:gmatch("%S+") do words = words + 1 end
        padforge.message(string.format("%d words", words))
    end)
end
```

The `lua/` directory contains ready-to-use examples:

- `word_count.lua`
- `trim_trailing_whitespace.lua`
- `sort_lines.lua`
- `number_lines.lua`
- `reverse_lines.lua`
- `remove_duplicate_lines.lua`

Copy any of them into `~/.config/PadForge/plugins/` and reload.

## Native plugins

A native plugin is a shared object exporting one symbol, `padforge_plugin`, of
type `PadForgePlugin` (see `src/core/PluginApi.h`). The struct provides an
`init(PadForgeHost*)` entry point through which the plugin registers commands
and performs document operations.

Build the example:

```sh
cmake -S plugins/native/hello -B build-hello
cmake --build build-hello
cp build-hello/hello.so ~/.config/PadForge/plugins/
```

See `native/hello/hello_plugin.c` for a complete, minimal example.
