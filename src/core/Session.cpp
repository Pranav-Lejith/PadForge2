#include "Session.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>

namespace PadForge {

namespace {

QJsonArray intListToJson(const QList<int>& values)
{
    QJsonArray array;
    for (int v : values)
        array.append(v);
    return array;
}

QList<int> jsonToIntList(const QJsonArray& array)
{
    QList<int> values;
    values.reserve(array.size());
    for (const QJsonValue& v : array)
        values.append(v.toInt());
    return values;
}

QJsonObject fileToJson(const SessionFile& f)
{
    QJsonObject o;
    o["path"] = f.path;
    o["cursor"] = f.cursor;
    o["anchor"] = f.anchor;
    o["firstVisibleLine"] = f.firstVisibleLine;
    o["scrollPos"] = f.scrollPos;
    o["language"] = f.language;
    o["encoding"] = f.encoding;
    o["eol"] = f.eol;
    o["readOnly"] = f.readOnly;
    o["pinned"] = f.pinned;
    o["untitled"] = f.untitled;
    o["untitledText"] = f.untitledText;
    o["bookmarks"] = intListToJson(f.bookmarkLines);
    return o;
}

SessionFile fileFromJson(const QJsonObject& o)
{
    SessionFile f;
    f.path = o.value("path").toString();
    f.cursor = o.value("cursor").toInt();
    f.anchor = o.value("anchor").toInt();
    f.firstVisibleLine = o.value("firstVisibleLine").toInt();
    f.scrollPos = o.value("scrollPos").toInt();
    f.language = o.value("language").toString();
    f.encoding = o.value("encoding").toString();
    f.eol = o.value("eol").toString();
    f.readOnly = o.value("readOnly").toBool();
    f.pinned = o.value("pinned").toBool();
    f.untitled = o.value("untitled").toBool();
    f.untitledText = o.value("untitledText").toString();
    f.bookmarkLines = jsonToIntList(o.value("bookmarks").toArray());
    return f;
}

} // namespace

QJsonObject SessionManager::toJson(const Session& session)
{
    QJsonObject root;
    root["version"] = session.version;

    QJsonArray files;
    for (const SessionFile& f : session.files)
        files.append(fileToJson(f));
    root["files"] = files;
    root["activeIndex"] = session.activeIndex;
    root["splitView"] = session.splitView;
    root["activeView"] = session.activeView;

    QJsonArray sub;
    for (const SessionFile& f : session.subViewFiles)
        sub.append(fileToJson(f));
    root["subViewFiles"] = sub;

    QJsonArray recent;
    for (const QString& r : session.recentFiles)
        recent.append(r);
    root["recentFiles"] = recent;
    return root;
}

Session SessionManager::fromJson(const QJsonObject& object)
{
    Session s;
    s.version = object.value("version").toInt(1);

    for (const QJsonValue& v : object.value("files").toArray())
        s.files.append(fileFromJson(v.toObject()));
    s.activeIndex = object.value("activeIndex").toInt(0);
    s.splitView = object.value("splitView").toBool(false);
    s.activeView = object.value("activeView").toInt(0);

    for (const QJsonValue& v : object.value("subViewFiles").toArray())
        s.subViewFiles.append(fileFromJson(v.toObject()));

    for (const QJsonValue& v : object.value("recentFiles").toArray())
        s.recentFiles.append(v.toString());
    return s;
}

bool SessionManager::save(const Session& session, const QString& path, QString* error)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error)
            *error = file.errorString();
        return false;
    }
    const QJsonDocument doc(toJson(session));
    const qint64 written = file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return written >= 0;
}

Session SessionManager::load(const QString& path, bool* ok)
{
    Session session;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (ok)
            *ok = false;
        return session;
    }
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (ok)
            *ok = false;
        return session;
    }
    if (ok)
        *ok = true;
    return fromJson(doc.object());
}

QString SessionManager::defaultPath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return dir + QStringLiteral("/session.json");
}

RecentFiles::RecentFiles(int max)
    : m_max(qMax(1, max))
{
}

void RecentFiles::add(const QString& path)
{
    if (path.isEmpty())
        return;
    const QString clean = QDir::cleanPath(path);
    m_files.removeAll(clean);
    m_files.prepend(clean);
    while (m_files.size() > m_max)
        m_files.removeLast();
}

void RecentFiles::remove(const QString& path)
{
    m_files.removeAll(QDir::cleanPath(path));
}

void RecentFiles::clear()
{
    m_files.clear();
}

void RecentFiles::setList(const QStringList& files)
{
    m_files.clear();
    for (const QString& f : files) {
        if (!f.isEmpty() && !m_files.contains(f))
            m_files.append(f);
        if (m_files.size() >= m_max)
            break;
    }
}

bool RecentFiles::save(const QString& path) const
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    QJsonArray array;
    for (const QString& f : m_files)
        array.append(f);
    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    return true;
}

bool RecentFiles::load(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray())
        return false;
    QStringList files;
    for (const QJsonValue& v : doc.array())
        files.append(v.toString());
    setList(files);
    return true;
}

} // namespace PadForge
