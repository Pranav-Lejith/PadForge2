/*
 * PadForge example native plugin.
 *
 * A native plugin is a shared object that exports a single symbol named by
 * PADFORGE_PLUGIN_SYMBOL ("padforge_plugin") of type PadForgePlugin. Build it
 * as a MODULE library and drop the resulting .so into
 *   <config>/PadForge/plugins/   (user plugins), or
 *   <appdir>/plugins/            (bundled plugins).
 *
 * This example registers one command, "Uppercase Document", which upper-cases
 * the ASCII letters of the active document.
 */

#include "PluginApi.h"

#include <stdlib.h>
#include <string.h>

static void on_uppercase(void* userdata)
{
    PadForgeHost* host = (PadForgeHost*)userdata;
    const char* text = host->get_text(host->userdata);
    if (!text)
        return;

    const size_t len = strlen(text);
    char* buffer = (char*)malloc(len + 1);
    if (!buffer)
        return;
    for (size_t i = 0; i < len; ++i) {
        char c = text[i];
        buffer[i] = (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
    }
    buffer[len] = '\0';

    host->set_text(host->userdata, buffer);
    host->message(host->userdata, "Document upper-cased");
    free(buffer);
}

static int plugin_init(PadForgeHost* host)
{
    if (host->api_version != PADFORGE_PLUGIN_API_VERSION)
        return 1;
    host->add_menu_item(host->userdata, "Uppercase Document", on_uppercase);
    host->message(host->userdata, "Hello plugin loaded");
    return 0;
}

static void plugin_shutdown(void)
{
    /* Nothing to release in this example. */
}

PadForgePlugin padforge_plugin = {
    "Hello",
    "1.0.0",
    "PadForge contributors",
    "Example native PadForge plugin",
    plugin_init,
    plugin_shutdown
};
