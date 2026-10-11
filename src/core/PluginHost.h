#pragma once

#include "PluginApi.h"

#include <QString>
#include <QStringList>

#include <functional>
#include <memory>
#include <vector>

struct lua_State;

namespace PadForge {

// Abstract view of the active document. The GUI implements this so the plugin
// runtime stays independent of the widget layer and can be tested headlessly.
class PluginContext {
public:
    virtual ~PluginContext() = default;

    virtual void insertText(const QString& text) = 0;
    virtual void setDocumentText(const QString& text) = 0;
    virtual QString documentText() const = 0;
    virtual int lineCount() const = 0;
    virtual int currentLine() const = 0;
    virtual void gotoLine(int oneBasedLine) = 0;
    virtual void setLanguage(const QString& lexerId) = 0;
    virtual void showMessage(const QString& message) = 0;
    virtual void addMenuItem(const QString& name, std::function<void()> callback) = 0;
};

class PluginHost {
public:
    explicit PluginHost(PluginContext* context);
    ~PluginHost();

    PluginHost(const PluginHost&) = delete;
    PluginHost& operator=(const PluginHost&) = delete;

    void setPluginDirectory(const QString& dir);
    QString pluginDirectory() const { return m_directory; }

    // Load a single Lua plugin file (registers its menu items immediately).
    bool loadLuaPlugin(const QString& path, QString* error = nullptr);

    // Load every *.lua plugin in `dir`, sorted by name.
    int loadLuaPluginsIn(const QString& dir);

    // Load a native .so/.dll exporting PADFORGE_PLUGIN_SYMBOL.
    bool loadNativePlugin(const QString& path, QString* error = nullptr);

    PluginContext* context() const { return m_context; }
    PadForgeHost* cHost() { return &m_cHost; }
    QString lastError() const { return m_lastError; }
    QStringList loadedPlugins() const { return m_loadedPlugins; }

    const PadForgeHost* cHost() const { return &m_cHost; }

#ifdef PADFORGE_ENABLE_LUA
    lua_State* luaState() const { return m_lua; }
    bool hasLua() const;
#endif

private:
    void setupCHost();

    PluginContext* m_context = nullptr;
    PadForgeHost m_cHost{};
    QString m_directory;
    QString m_lastError;
    QStringList m_loadedPlugins;
    QString m_textBuffer;

    struct NativeHandle;
    std::vector<std::unique_ptr<NativeHandle>> m_native;

#ifdef PADFORGE_ENABLE_LUA
    lua_State* m_lua = nullptr;
    void setupLua();
    void closeLua();
    friend struct LuaBridge;
#endif
};

} // namespace PadForge
