#include "PluginHost.h"

#include <QDir>
#include <QFileInfo>
#include <QLibrary>

#ifdef PADFORGE_ENABLE_LUA
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}
#endif

namespace PadForge {

namespace {

PluginHost* asHost(void* userdata)
{
    return static_cast<PluginHost*>(userdata);
}

void c_insert_text(void* u, const char* utf8)
{
    if (u && utf8)
        asHost(u)->context()->insertText(QString::fromUtf8(utf8));
}

void c_set_text(void* u, const char* utf8)
{
    if (u)
        asHost(u)->context()->setDocumentText(QString::fromUtf8(utf8 ? utf8 : ""));
}

const char* c_get_text(void* u)
{
    if (!u)
        return "";
    // Store in the host so the returned pointer stays valid until the next call.
    thread_local QByteArray buffer;
    buffer = asHost(u)->context()->documentText().toUtf8();
    return buffer.constData();
}

int c_line_count(void* u)
{
    return u ? asHost(u)->context()->lineCount() : 0;
}

int c_current_line(void* u)
{
    return u ? asHost(u)->context()->currentLine() : 0;
}

void c_goto_line(void* u, int line)
{
    if (u)
        asHost(u)->context()->gotoLine(line);
}

void c_set_language(void* u, const char* lexer)
{
    if (u && lexer)
        asHost(u)->context()->setLanguage(QString::fromUtf8(lexer));
}

void c_message(void* u, const char* utf8)
{
    if (u && utf8)
        asHost(u)->context()->showMessage(QString::fromUtf8(utf8));
}

void c_add_menu_item(void* u, const char* name, void (*callback)(void*))
{
    if (!u || !name || !callback)
        return;
    asHost(u)->context()->addMenuItem(QString::fromUtf8(name),
                                      [callback, u]() { callback(u); });
}

} // namespace

struct PluginHost::NativeHandle {
    std::unique_ptr<QLibrary> library;
    PadForgePlugin* plugin = nullptr;

    ~NativeHandle()
    {
        if (plugin && plugin->shutdown)
            plugin->shutdown();
    }
};

PluginHost::PluginHost(PluginContext* context)
    : m_context(context)
{
    setupCHost();
#ifdef PADFORGE_ENABLE_LUA
    setupLua();
#endif
}

PluginHost::~PluginHost()
{
    m_native.clear();
#ifdef PADFORGE_ENABLE_LUA
    closeLua();
#endif
}

void PluginHost::setupCHost()
{
    m_cHost.api_version = PADFORGE_PLUGIN_API_VERSION;
    m_cHost.userdata = this;
    m_cHost.insert_text = &c_insert_text;
    m_cHost.set_text = &c_set_text;
    m_cHost.get_text = &c_get_text;
    m_cHost.line_count = &c_line_count;
    m_cHost.current_line = &c_current_line;
    m_cHost.goto_line = &c_goto_line;
    m_cHost.set_language = &c_set_language;
    m_cHost.message = &c_message;
    m_cHost.add_menu_item = &c_add_menu_item;
}

void PluginHost::setPluginDirectory(const QString& dir)
{
    m_directory = dir;
}

bool PluginHost::loadNativePlugin(const QString& path, QString* error)
{
    auto handle = std::make_unique<NativeHandle>();
    handle->library = std::make_unique<QLibrary>(path);
    if (!handle->library->load()) {
        m_lastError = handle->library->errorString();
        if (error)
            *error = m_lastError;
        return false;
    }
    auto* plugin = reinterpret_cast<PadForgePlugin*>(
        handle->library->resolve(PADFORGE_PLUGIN_SYMBOL));
    if (!plugin) {
        m_lastError = QStringLiteral("missing symbol %1 in %2")
                          .arg(QString::fromLatin1(PADFORGE_PLUGIN_SYMBOL), path);
        if (error)
            *error = m_lastError;
        return false;
    }
    if (plugin->init && plugin->init(&m_cHost) != 0) {
        m_lastError = QStringLiteral("plugin init() failed: %1").arg(path);
        if (error)
            *error = m_lastError;
        return false;
    }
    handle->plugin = plugin;
    m_loadedPlugins.append(QFileInfo(path).fileName());
    m_native.push_back(std::move(handle));
    return true;
}

#ifdef PADFORGE_ENABLE_LUA

namespace {

const char kHostRegistryKey = 'P';

PluginHost* luaHost(lua_State* L)
{
    lua_pushlightuserdata(L, const_cast<char*>(&kHostRegistryKey));
    lua_gettable(L, LUA_REGISTRYINDEX);
    auto* host = static_cast<PluginHost*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return host;
}

int l_insert_text(lua_State* L)
{
    PluginHost* h = luaHost(L);
    const char* s = luaL_checkstring(L, 1);
    if (h)
        h->context()->insertText(QString::fromUtf8(s));
    return 0;
}

int l_set_text(lua_State* L)
{
    PluginHost* h = luaHost(L);
    const char* s = luaL_checkstring(L, 1);
    if (h)
        h->context()->setDocumentText(QString::fromUtf8(s));
    return 0;
}

int l_get_text(lua_State* L)
{
    PluginHost* h = luaHost(L);
    const QByteArray utf8 = (h ? h->context()->documentText() : QString()).toUtf8();
    lua_pushlstring(L, utf8.constData(), size_t(utf8.size()));
    return 1;
}

int l_line_count(lua_State* L)
{
    PluginHost* h = luaHost(L);
    lua_pushinteger(L, h ? h->context()->lineCount() : 0);
    return 1;
}

int l_current_line(lua_State* L)
{
    PluginHost* h = luaHost(L);
    lua_pushinteger(L, h ? h->context()->currentLine() : 0);
    return 1;
}

int l_goto_line(lua_State* L)
{
    PluginHost* h = luaHost(L);
    const int line = int(luaL_checkinteger(L, 1));
    if (h)
        h->context()->gotoLine(line);
    return 0;
}

int l_set_language(lua_State* L)
{
    PluginHost* h = luaHost(L);
    const char* id = luaL_checkstring(L, 1);
    if (h)
        h->context()->setLanguage(QString::fromUtf8(id));
    return 0;
}

int l_message(lua_State* L)
{
    PluginHost* h = luaHost(L);
    const char* s = luaL_checkstring(L, 1);
    if (h)
        h->context()->showMessage(QString::fromUtf8(s));
    return 0;
}

int l_add_menu_item(lua_State* L)
{
    PluginHost* h = luaHost(L);
    const char* name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    lua_pushvalue(L, 2);
    const int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    if (h) {
        h->context()->addMenuItem(QString::fromUtf8(name), [L, ref]() {
            lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
            if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
                const char* err = lua_tostring(L, -1);
                qWarning("PadForge Lua plugin error: %s", err ? err : "unknown");
                lua_pop(L, 1);
            }
        });
    }
    return 0;
}

int l_print(lua_State* L)
{
    PluginHost* h = luaHost(L);
    QString message;
    const int n = lua_gettop(L);
    for (int i = 1; i <= n; ++i) {
        if (i > 1)
            message += QLatin1Char('\t');
        size_t len = 0;
        const char* s = luaL_tolstring(L, i, &len);
        message += QString::fromUtf8(s, int(len));
        lua_pop(L, 1);
    }
    if (h)
        h->context()->showMessage(message);
    return 0;
}

int luaTraceback(lua_State* L)
{
    const char* msg = lua_tostring(L, 1);
    if (msg) {
        luaL_traceback(L, L, msg, 1);
    } else {
        lua_pushliteral(L, "(no error object)");
    }
    return 1;
}

} // namespace

void PluginHost::setupLua()
{
    m_lua = luaL_newstate();
    if (!m_lua)
        return;
    luaL_requiref(m_lua, LUA_GNAME, luaopen_base, 1);
    lua_pop(m_lua, 1);
    luaL_requiref(m_lua, LUA_TABLIBNAME, luaopen_table, 1);
    lua_pop(m_lua, 1);
    luaL_requiref(m_lua, LUA_STRLIBNAME, luaopen_string, 1);
    lua_pop(m_lua, 1);
    luaL_requiref(m_lua, LUA_MATHLIBNAME, luaopen_math, 1);
    lua_pop(m_lua, 1);
    luaL_requiref(m_lua, LUA_UTF8LIBNAME, luaopen_utf8, 1);
    lua_pop(m_lua, 1);

    lua_pushlightuserdata(m_lua, const_cast<char*>(&kHostRegistryKey));
    lua_pushlightuserdata(m_lua, this);
    lua_settable(m_lua, LUA_REGISTRYINDEX);

    static const luaL_Reg functions[] = {
        {"insert_text", l_insert_text},   {"set_text", l_set_text},
        {"get_text", l_get_text},         {"line_count", l_line_count},
        {"current_line", l_current_line}, {"goto_line", l_goto_line},
        {"set_language", l_set_language}, {"message", l_message},
        {"add_menu_item", l_add_menu_item},{NULL, NULL},
    };
    lua_newtable(m_lua);
    luaL_setfuncs(m_lua, functions, 0);
    lua_setglobal(m_lua, "padforge");

    lua_pushcfunction(m_lua, l_print);
    lua_setglobal(m_lua, "print");
}

void PluginHost::closeLua()
{
    if (m_lua) {
        lua_close(m_lua);
        m_lua = nullptr;
    }
}

bool PluginHost::hasLua() const
{
    return m_lua != nullptr;
}

bool PluginHost::loadLuaPlugin(const QString& path, QString* error)
{
    if (!m_lua) {
        m_lastError = QStringLiteral("Lua runtime is not available");
        if (error)
            *error = m_lastError;
        return false;
    }
    lua_State* L = m_lua;
    lua_pushcfunction(L, luaTraceback);
    const int errFunc = lua_gettop(L);
    const QByteArray utf8 = QFile::encodeName(path);
    if (luaL_loadfile(L, utf8.constData()) != LUA_OK
        || lua_pcall(L, 0, 0, errFunc) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        m_lastError = QString::fromUtf8(err ? err : "unknown Lua error");
        lua_pop(L, 1);
        lua_remove(L, errFunc);
        if (error)
            *error = m_lastError;
        return false;
    }
    lua_remove(L, errFunc);

    // Optional convention: plugins define setup() to register their commands.
    lua_getglobal(L, "setup");
    if (lua_isfunction(L, -1)) {
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            const char* err = lua_tostring(L, -1);
            m_lastError = QString::fromUtf8(err ? err : "unknown Lua error");
            lua_pop(L, 1);
            if (error)
                *error = m_lastError;
            return false;
        }
    } else {
        lua_pop(L, 1);
    }

    m_loadedPlugins.append(QFileInfo(path).fileName());
    return true;
}

int PluginHost::loadLuaPluginsIn(const QString& dir)
{
    QDir d(dir);
    if (!d.exists())
        return 0;
    int loaded = 0;
    const QFileInfoList entries =
        d.entryInfoList({QStringLiteral("*.lua")}, QDir::Files, QDir::Name);
    for (const QFileInfo& info : entries) {
        QString error;
        if (loadLuaPlugin(info.absoluteFilePath(), &error))
            ++loaded;
        else
            qWarning("PadForge: failed to load plugin %s: %s",
                     qPrintable(info.fileName()), qPrintable(error));
    }
    return loaded;
}

#else

bool PluginHost::loadLuaPlugin(const QString&, QString* error)
{
    if (error)
        *error = QStringLiteral("Lua support was not compiled in");
    return false;
}

int PluginHost::loadLuaPluginsIn(const QString&)
{
    return 0;
}

#endif

} // namespace PadForge
