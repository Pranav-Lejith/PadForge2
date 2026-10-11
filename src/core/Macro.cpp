#include "Macro.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace PadForge {

void MacroRecorder::start(const QString& name)
{
    m_recording = true;
    m_macro = Macro{};
    m_macro.name = name;
    m_macro.steps.clear();
}

void MacroRecorder::record(const MacroStep& step)
{
    if (m_recording)
        m_macro.steps.append(step);
}

Macro MacroRecorder::stop()
{
    m_recording = false;
    return m_macro;
}

void MacroRecorder::cancel()
{
    m_recording = false;
    m_macro = Macro{};
}

namespace {

QJsonObject stepToJson(const MacroStep& s)
{
    QJsonObject o;
    o["kind"] = int(s.kind);
    o["text"] = s.text;
    o["position"] = s.position;
    o["length"] = s.length;
    o["command"] = s.command;
    return o;
}

MacroStep stepFromJson(const QJsonObject& o)
{
    MacroStep s;
    s.kind = static_cast<MacroStep::Kind>(o.value("kind").toInt(0));
    s.text = o.value("text").toString();
    s.position = o.value("position").toInt(-1);
    s.length = o.value("length").toInt(0);
    s.command = o.value("command").toString();
    return s;
}

} // namespace

bool MacroLibrary::save(const QList<Macro>& macros, const QString& path)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QJsonArray array;
    for (const Macro& m : macros) {
        QJsonObject o;
        o["name"] = m.name;
        o["shortcut"] = m.shortcut;
        QJsonArray steps;
        for (const MacroStep& s : m.steps)
            steps.append(stepToJson(s));
        o["steps"] = steps;
        array.append(o);
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    return true;
}

QList<Macro> MacroLibrary::load(const QString& path, bool* ok)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (ok)
            *ok = false;
        return {};
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray()) {
        if (ok)
            *ok = false;
        return {};
    }
    QList<Macro> macros;
    for (const QJsonValue& v : doc.array()) {
        const QJsonObject o = v.toObject();
        Macro m;
        m.name = o.value("name").toString();
        m.shortcut = o.value("shortcut").toString();
        for (const QJsonValue& sv : o.value("steps").toArray())
            m.steps.append(stepFromJson(sv.toObject()));
        macros.append(m);
    }
    if (ok)
        *ok = true;
    return macros;
}

QString MacroLibrary::defaultPath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return dir + QStringLiteral("/macros.json");
}

} // namespace PadForge
