#pragma once

#include <QColor>
#include <QHash>
#include <QList>
#include <QString>

namespace PadForge {

enum class StyleRole {
    Default,
    Comment,
    CommentDoc,
    Keyword,
    Keyword2,
    Number,
    String,
    Character,
    Preprocessor,
    Operator,
    Identifier,
    Function,
    Type,
    Tag,
    Attribute,
    Value,
    Constant,
    Variable,
};

struct ThemeStyle {
    StyleRole role = StyleRole::Default;
    QColor foreground;
    QColor background;
    bool bold = false;
    bool italic = false;
};

struct Theme {
    QString id;
    QString name;
    bool dark = false;

    QColor background;
    QColor foreground;
    QColor caret;
    QColor selectionBg;
    QColor selectionFg;
    QColor currentLine;
    QColor lineNumberFg;
    QColor lineNumberBg;
    QColor foldFg;
    QColor matchedBraceBg;
    QColor bookmarkFg;
    QColor indentGuide;
    QColor edgeLine;

    // Optional per-theme overrides keyed by StyleRole (as int).
    QHash<int, QColor> roleOverrides;

    QColor roleColor(StyleRole role) const;
    QColor colorFor(const QString& lexer, int style) const;
    bool boldFor(const QString& lexer, int style) const;
    bool italicFor(const QString& lexer, int style) const;
    QString styleSheet() const;
};

class ThemeManager {
public:
    static const QList<Theme>& themes();
    static const Theme& byId(const QString& id);
    static const Theme& current();
    static void setCurrent(const QString& id);

private:
    static QString s_current;
};

// Exposed for tests / advanced use: map a Lexilla style number to a role.
StyleRole styleRole(const QString& lexer, int style);

} // namespace PadForge
