#include "Theme.h"

namespace PadForge {

namespace {

bool isCppFamily(const QString& lexer)
{
    return lexer == "cpp" || lexer == "cs";
}

StyleRole cppRole(int style)
{
    switch (style) {
    case 1:
    case 2:
        return StyleRole::Comment;
    case 3:
    case 16:
        return StyleRole::CommentDoc;
    case 4:
        return StyleRole::Number;
    case 5:
        return StyleRole::Keyword;
    case 6:
    case 17:
    case 18:
        return StyleRole::Keyword2;
    case 7:
    case 13:
    case 14:
    case 15:
    case 19:
        return StyleRole::String;
    case 8:
        return StyleRole::Character;
    case 9:
    case 10:
        return StyleRole::Preprocessor;
    case 11:
        return StyleRole::Operator;
    case 12:
        return StyleRole::Identifier;
    default:
        return StyleRole::Default;
    }
}

StyleRole pythonRole(int style)
{
    switch (style) {
    case 1:
        return StyleRole::Comment;
    case 2:
        return StyleRole::Number;
    case 3:
    case 6:
    case 7:
    case 13:
    case 16:
        return StyleRole::String;
    case 4:
        return StyleRole::Character;
    case 5:
        return StyleRole::Keyword;
    case 8:
        return StyleRole::Type;
    case 9:
        return StyleRole::Function;
    case 10:
        return StyleRole::Operator;
    case 11:
        return StyleRole::Identifier;
    case 12:
        return StyleRole::CommentDoc;
    case 14:
        return StyleRole::Keyword2;
    case 15:
        return StyleRole::Type;
    default:
        return StyleRole::Default;
    }
}

StyleRole javaScriptRole(int style)
{
    switch (style) {
    case 1:
    case 2:
        return StyleRole::Comment;
    case 3:
    case 4:
        return StyleRole::CommentDoc;
    case 5:
        return StyleRole::Number;
    case 6:
        return StyleRole::Keyword;
    case 7:
        return StyleRole::Keyword2;
    case 8:
    case 14:
    case 15:
    case 16:
        return StyleRole::String;
    case 9:
        return StyleRole::Character;
    case 10:
    case 11:
        return StyleRole::Preprocessor;
    case 12:
        return StyleRole::Operator;
    case 13:
        return StyleRole::Identifier;
    default:
        return StyleRole::Default;
    }
}

StyleRole hypertextRole(int style)
{
    switch (style) {
    case 1:
        return StyleRole::Tag;
    case 2:
        return StyleRole::Tag;
    case 3:
        return StyleRole::Attribute;
    case 4:
        return StyleRole::Attribute;
    case 5:
        return StyleRole::Number;
    case 6:
    case 7:
        return StyleRole::String;
    case 9:
        return StyleRole::Comment;
    case 10:
        return StyleRole::Preprocessor;
    case 11:
        return StyleRole::Keyword;
    case 12:
    case 13:
        return StyleRole::Variable;
    case 14:
        return StyleRole::Attribute;
    default:
        return StyleRole::Default;
    }
}

StyleRole cssRole(int style)
{
    switch (style) {
    case 1:
        return StyleRole::Tag;
    case 2:
        return StyleRole::Function;
    case 3:
        return StyleRole::Keyword;
    case 5:
        return StyleRole::Operator;
    case 6:
    case 7:
        return StyleRole::Identifier;
    case 8:
        return StyleRole::Value;
    case 9:
        return StyleRole::Comment;
    case 10:
        return StyleRole::Type;
    case 11:
    case 12:
        return StyleRole::Keyword;
    default:
        return StyleRole::Default;
    }
}

StyleRole xmlRole(int style)
{
    switch (style) {
    case 1:
    case 2:
    case 11:
    case 12:
    case 13:
        return StyleRole::Tag;
    case 3:
    case 4:
        return StyleRole::Attribute;
    case 5:
        return StyleRole::Number;
    case 6:
    case 7:
    case 14:
        return StyleRole::String;
    case 9:
        return StyleRole::Comment;
    case 10:
        return StyleRole::Preprocessor;
    case 15:
        return StyleRole::Keyword;
    case 16:
        return StyleRole::Value;
    default:
        return StyleRole::Default;
    }
}

StyleRole yamlRole(int style)
{
    switch (style) {
    case 1:
        return StyleRole::Comment;
    case 2:
        return StyleRole::Identifier;
    case 3:
        return StyleRole::Keyword;
    case 4:
        return StyleRole::Number;
    case 5:
        return StyleRole::Value;
    case 9:
        return StyleRole::String;
    default:
        return StyleRole::Default;
    }
}

StyleRole genericRole(int style)
{
    switch (style) {
    case 1:
    case 2:
    case 12:
    case 16:
    case 17:
        return StyleRole::Comment;
    case 3:
        return StyleRole::CommentDoc;
    case 4:
        return StyleRole::Number;
    case 5:
        return StyleRole::Keyword;
    case 6:
        return StyleRole::Keyword2;
    case 7:
    case 13:
        return StyleRole::String;
    case 8:
        return StyleRole::Character;
    case 9:
        return StyleRole::Preprocessor;
    case 10:
        return StyleRole::Operator;
    case 11:
        return StyleRole::Identifier;
    case 14:
        return StyleRole::Variable;
    case 15:
        return StyleRole::String;
    default:
        return StyleRole::Default;
    }
}

Theme buildCommon(const QString& id, const QString& name, bool dark)
{
    Theme t;
    t.id = id;
    t.name = name;
    t.dark = dark;
    return t;
}

Theme lightTheme()
{
    Theme t = buildCommon("light", "Light", false);
    t.background = QColor("#ffffff");
    t.foreground = QColor("#202020");
    t.caret = QColor("#1f1f1f");
    t.selectionBg = QColor("#add6ff");
    t.selectionFg = QColor("#000000");
    t.currentLine = QColor("#f0f0f0");
    t.lineNumberFg = QColor("#9a9a9a");
    t.lineNumberBg = QColor("#f2f2f2");
    t.foldFg = QColor("#909090");
    t.matchedBraceBg = QColor("#c8e6c9");
    t.bookmarkFg = QColor("#4a86e8");
    t.indentGuide = QColor("#e0e0e0");
    t.edgeLine = QColor("#d0d0d0");
    return t;
}

Theme darkTheme()
{
    Theme t = buildCommon("dark", "Dark", true);
    t.background = QColor("#1e2127");
    t.foreground = QColor("#d7dae0");
    t.caret = QColor("#d7dae0");
    t.selectionBg = QColor("#3e4451");
    t.selectionFg = QColor("#ffffff");
    t.currentLine = QColor("#272b33");
    t.lineNumberFg = QColor("#7f848e");
    t.lineNumberBg = QColor("#181b20");
    t.foldFg = QColor("#9aa0a6");
    t.matchedBraceBg = QColor("#2f4f4f");
    t.bookmarkFg = QColor("#61afef");
    t.indentGuide = QColor("#2c313a");
    t.edgeLine = QColor("#3e4451");
    return t;
}

} // namespace

StyleRole styleRole(const QString& lexer, int style)
{
    if (isCppFamily(lexer))
        return cppRole(style);
    if (lexer == "python")
        return pythonRole(style);
    if (lexer == "javascript")
        return javaScriptRole(style);
    if (lexer == "hypertext")
        return hypertextRole(style);
    if (lexer == "css")
        return cssRole(style);
    if (lexer == "xml")
        return xmlRole(style);
    if (lexer == "yaml")
        return yamlRole(style);
    return genericRole(style);
}

QColor Theme::roleColor(StyleRole role) const
{
    const bool d = dark;
    switch (role) {
    case StyleRole::Comment:
        return d ? QColor("#7f848e") : QColor("#008000");
    case StyleRole::CommentDoc:
        return d ? QColor("#8b949e") : QColor("#008080");
    case StyleRole::Keyword:
        return d ? QColor("#c678dd") : QColor("#0000c0");
    case StyleRole::Keyword2:
        return d ? QColor("#d2b0ff") : QColor("#0000ff");
    case StyleRole::Number:
        return d ? QColor("#d19a66") : QColor("#a05000");
    case StyleRole::String:
        return d ? QColor("#98c379") : QColor("#c03030");
    case StyleRole::Character:
        return d ? QColor("#98c379") : QColor("#c03030");
    case StyleRole::Preprocessor:
        return d ? QColor("#e5c07b") : QColor("#804000");
    case StyleRole::Operator:
        return d ? QColor("#56b6c2") : QColor("#206020");
    case StyleRole::Identifier:
        return foreground;
    case StyleRole::Function:
        return d ? QColor("#61afef") : QColor("#7a3e9d");
    case StyleRole::Type:
        return d ? QColor("#e5c07b") : QColor("#16607a");
    case StyleRole::Tag:
        return d ? QColor("#e06c75") : QColor("#a00000");
    case StyleRole::Attribute:
        return d ? QColor("#d19a66") : QColor("#a05000");
    case StyleRole::Value:
        return d ? QColor("#98c379") : QColor("#c03030");
    case StyleRole::Constant:
        return d ? QColor("#d19a66") : QColor("#a05000");
    case StyleRole::Variable:
        return d ? QColor("#e5c07b") : QColor("#804000");
    case StyleRole::Default:
        break;
    }
    return foreground;
}

QColor Theme::colorFor(const QString& lexer, int style) const
{
    return roleColor(styleRole(lexer, style));
}

bool Theme::boldFor(const QString& lexer, int style) const
{
    const StyleRole role = styleRole(lexer, style);
    return role == StyleRole::Keyword || role == StyleRole::Keyword2;
}

bool Theme::italicFor(const QString& lexer, int style) const
{
    const StyleRole role = styleRole(lexer, style);
    return role == StyleRole::Comment || role == StyleRole::CommentDoc;
}

QString Theme::styleSheet() const
{
    const QString bg = background.name();
    const QString fg = foreground.name();
    const QString sel = selectionBg.name();
    const QString panel = dark ? QStringLiteral("#252932") : QStringLiteral("#f5f5f5");
    const QString border = dark ? QStringLiteral("#3e4451") : QStringLiteral("#d0d0d0");
    const QString hover = dark ? QStringLiteral("#2c313a") : QStringLiteral("#e8e8e8");
    return QStringLiteral(
               "QWidget { background: %1; color: %2; }"
               "QMenuBar, QMenu, QToolBar, QStatusBar, QTabBar::tab { background: %3; color: %2; }"
               "QMenu::item:selected, QTabBar::tab:selected { background: %4; }"
               "QTabWidget::pane { border: 1px solid %5; }"
               "QLineEdit, QPlainTextEdit, QTreeView, QListView, QComboBox, QSpinBox {"
               "  background: %1; color: %2; border: 1px solid %5; }"
               "QPushButton { background: %3; color: %2; border: 1px solid %5; padding: 3px 10px; }"
               "QPushButton:hover { background: %4; }"
               "QHeaderView::section { background: %3; color: %2; border: 1px solid %5; }"
               "QToolTip { background: %3; color: %2; border: 1px solid %5; }")
        .arg(bg, fg, panel, hover, border)
        + (dark ? QStringLiteral("QScrollBar { background: #181b20; }")
                : QString());
}

const QList<Theme>& ThemeManager::themes()
{
    static const QList<Theme> list{lightTheme(), darkTheme()};
    return list;
}

const Theme& ThemeManager::byId(const QString& id)
{
    for (const Theme& t : themes())
        if (t.id.compare(id, Qt::CaseInsensitive) == 0)
            return t;
    return themes().first();
}

QString ThemeManager::s_current = QStringLiteral("light");

const Theme& ThemeManager::current()
{
    return byId(s_current);
}

void ThemeManager::setCurrent(const QString& id)
{
    s_current = id;
}

} // namespace PadForge
