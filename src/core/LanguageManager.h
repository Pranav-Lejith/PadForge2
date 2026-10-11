#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace PadForge {

struct Language {
    QString id;   // stable identifier, e.g. "cpp"
    QString name; // display name, e.g. "C++"
    QString lexer;
    QStringList extensions;
    QStringList shebangs;
    QString keywords;
};

// Maps file names / shebangs to a Lexilla lexer and supplies keyword sets.
// Covers 30+ languages out of the box.
class LanguageManager {
public:
    static const QList<Language>& languages();
    static const Language* byId(const QString& id);
    static const Language* forFile(const QString& path, const QString& firstLine = QString());
    static const Language* plainText();
    static QStringList displayNames();
    static int count();
};

} // namespace PadForge
