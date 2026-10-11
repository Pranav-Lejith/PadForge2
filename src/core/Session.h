#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

namespace PadForge {

struct SessionFile {
    QString path;
    int cursor = 0;
    int anchor = 0;
    int firstVisibleLine = 0;
    int scrollPos = 0;
    QString language;
    QString encoding;
    QString eol;
    bool readOnly = false;
    bool pinned = false;
    bool untitled = false;
    QString untitledText;
    QList<int> bookmarkLines;

    bool isValid() const { return !path.isEmpty() || untitled; }
};

struct Session {
    int version = 1;
    QList<SessionFile> files;
    int activeIndex = 0;
    bool splitView = false;
    int activeView = 0;
    QList<SessionFile> subViewFiles;
    QStringList recentFiles;

    bool isEmpty() const { return files.isEmpty(); }
};

class SessionManager {
public:
    static bool save(const Session& session, const QString& path, QString* error = nullptr);
    static Session load(const QString& path, bool* ok = nullptr);
    static QJsonObject toJson(const Session& session);
    static Session fromJson(const QJsonObject& object);
    static QString defaultPath();
};

class RecentFiles {
public:
    explicit RecentFiles(int max = 15);

    void add(const QString& path);
    void remove(const QString& path);
    void clear();

    QStringList list() const { return m_files; }
    void setList(const QStringList& files);
    int max() const { return m_max; }

    bool save(const QString& path) const;
    bool load(const QString& path);

private:
    QStringList m_files;
    int m_max;
};

} // namespace PadForge
