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

    // Comment tokens used by the comment toggle commands.
    QString lineComment;
    QString blockCommentStart;
    QString blockCommentEnd;

    bool hasLineComment() const { return !lineComment.isEmpty(); }
    bool hasBlockComment() const
    {
        return !blockCommentStart.isEmpty() && !blockCommentEnd.isEmpty();
    }
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
