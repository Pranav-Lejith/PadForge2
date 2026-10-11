# Plugin guide

PadForge exposes a small, stable plugin surface shared by two plugin styles:

- **Lua plugins** — script files, the recommended way to extend PadForge.
- **Native plugins** — shared libraries built against the C ABI in
  [`src/core/PluginApi.h`](../src/core/PluginApi.h).

Both talk to the same host operations, so a Lua script and a native plugin can
do the same things.

## Where plugins are loaded from

At start-up (and on **Plugins → Reload Plugins**) PadForge scans:

| Directory | Purpose |
| --- | --- |
| `~/.config/PadForge/plugins` (i.e. `<AppConfigLocation>/plugins`) | User plugins |
| `<application dir>/plugins` | Bundled native plugins |

Lua files end in `.lua`; native plugins are `.so` files.

## The Lua API

A Lua plugin may define a global `setup()` function. Inside it, register
commands with `padforge.add_menu_item`. The host exposes these functions on the
global `padforge` table:

| Function | Description |
| --- | --- |
| `padforge.get_text()` | Active document contents as a string |
| `padforge.set_text(s)` | Replace the active document with `s` |
| `padforge.insert_text(s)` | Insert `s` at the caret |
| `padforge.line_count()` | Number of lines |
| `padforge.current_line()` | 1-based caret line |
| `padforge.goto_line(n)` | Move the caret to line `n` |
| `padforge.set_language(id)` | Apply a lexer by language id (e.g. `"cpp"`) |
| `padforge.message(s)` | Show `s` in the status bar |
| `padforge.add_menu_item(name, fn)` | Add `name` to the Plugins menu |

`print(...)` is also available and forwards to the status bar. The standard
`string`, `table`, `math` and `utf8` libraries are available; `io` and `os` are
not, so plugins cannot touch the filesystem directly.

### Example

```lua
function setup()
    padforge.add_menu_item("Uppercase", function()
        local text = padforge.get_text()
        padforge.set_text(string.upper(text))
    end)
end
```

Save as `~/.config/PadForge/plugins/uppercase.lua` and reload plugins.

### Bundled examples

| File | Command |
| --- | --- |
| `word_count.lua` | Word Count |
| `trim_trailing_whitespace.lua` | Trim Trailing Whitespace |
| `sort_lines.lua` | Sort Lines |
| `number_lines.lua` | Number Lines |
| `reverse_lines.lua` | Reverse Lines |
| `remove_duplicate_lines.lua` | Remove Duplicate Lines |

## Native plugins

A native plugin is a shared library exporting exactly one symbol,
`padforge_plugin`, of type `PadForgePlugin`:

```c
typedef struct PadForgePlugin {
    const char* name;
    const char* version;
    const char* author;
    const char* description;
    int (*init)(PadForgeHost* host);   /* return 0 on success */
    void (*shutdown)(void);            /* optional, may be NULL */
} PadForgePlugin;
```

`init` receives a `PadForgeHost*` whose function pointers mirror the Lua API.
Strings are UTF-8, and `get_text()` returns a pointer valid until the next host
call. Build it as a MODULE library and drop the `.so` into a plugin directory.

```sh
cmake -S plugins/native/hello -B build-hello
cmake --build build-hello
cp build-hello/hello.so ~/.config/PadForge/plugins/
```

See [`plugins/native/hello/hello_plugin.c`](../plugins/native/hello/hello_plugin.c)
for a complete example and [`plugins/README.md`](../plugins/README.md) for the
directory conventions.

## Stability

The plugin ABI carries `PADFORGE_PLUGIN_API_VERSION`; the host refuses to load a
plugin built against a different version. The Lua surface mirrors the same
operations, so scripts keep working as the host evolves.
