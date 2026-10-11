#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace PadForge {

struct MacroStep {
    enum class Kind {
        InsertText,
        ReplaceText,
        DeleteText,
        Command,
    };

    Kind kind = Kind::InsertText;
    QString text;
    int position = -1;
    int length = 0;
    QString command;
};

struct Macro {
    QString name;
    QString shortcut;
    QList<MacroStep> steps;

    bool isEmpty() const { return steps.isEmpty(); }
};

class MacroRecorder {
public:
    void start(const QString& name);
    bool isRecording() const { return m_recording; }
    void record(const MacroStep& step);
    Macro stop();
    void cancel();
    const Macro& current() const { return m_macro; }

private:
    bool m_recording = false;
    Macro m_macro;
};

class MacroLibrary {
public:
    static bool save(const QList<Macro>& macros, const QString& path);
    static QList<Macro> load(const QString& path, bool* ok = nullptr);
    static QString defaultPath();
};

} // namespace PadForge
