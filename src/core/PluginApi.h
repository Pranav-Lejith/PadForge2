/*
 * PadForge native plugin C ABI.
 *
 * This is intentionally small and stable. Lua plugins use the same host
 * operations through the `padforge` table (see docs/plugin-guide.md), so both
 * plugin styles share one capability surface.
 *
 * A native plugin is a shared object exporting a single symbol,
 * PADFORGE_PLUGIN_SYMBOL ("padforge_plugin"), of type PadForgePlugin.
 */
#ifndef PADFORGE_PLUGIN_API_H
#define PADFORGE_PLUGIN_API_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PADFORGE_PLUGIN_API_VERSION 1
#define PADFORGE_PLUGIN_SYMBOL "padforge_plugin"

typedef struct PadForgeHost PadForgeHost;

struct PadForgeHost {
    int api_version;
    void* userdata;

    /* Document operations. Strings are UTF-8. get_text() returns a pointer
     * valid until the next host call on the same document. */
    void (*insert_text)(void* userdata, const char* utf8);
    void (*set_text)(void* userdata, const char* utf8);
    const char* (*get_text)(void* userdata);
    int (*line_count)(void* userdata);
    int (*current_line)(void* userdata);
    void (*goto_line)(void* userdata, int one_based_line);
    void (*set_language)(void* userdata, const char* lexer_id);
    void (*message)(void* userdata, const char* utf8);

    /* Registers a command in the PadForge "Plugins" menu. The callback is
     * invoked on the UI thread. */
    void (*add_menu_item)(void* userdata, const char* name, void (*callback)(void*));

    void* reserved[6];
};

typedef struct PadForgePlugin {
    const char* name;
    const char* version;
    const char* author;
    const char* description;
    /* Return 0 on success. Called once after load. */
    int (*init)(PadForgeHost* host);
    /* Optional; called before unload. May be NULL. */
    void (*shutdown)(void);
} PadForgePlugin;

#ifdef __cplusplus
}
#endif

#endif /* PADFORGE_PLUGIN_API_H */
