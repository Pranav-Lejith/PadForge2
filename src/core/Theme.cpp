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

QHash<int, QColor> roles(const QColor& keyword, const QColor& keyword2, const QColor& stringColor,
                         const QColor& comment, const QColor& number, const QColor& function,
                         const QColor& type, const QColor& tag, const QColor& preprocessor,
                         const QColor& operatorColor, const QColor& value)
{
    return {
        { int(StyleRole::Keyword), keyword },
        { int(StyleRole::Keyword2), keyword2 },
        { int(StyleRole::String), stringColor },
        { int(StyleRole::Character), stringColor },
        { int(StyleRole::Comment), comment },
        { int(StyleRole::CommentDoc), comment },
        { int(StyleRole::Number), number },
        { int(StyleRole::Constant), number },
        { int(StyleRole::Function), function },
        { int(StyleRole::Type), type },
        { int(StyleRole::Tag), tag },
        { int(StyleRole::Attribute), type },
        { int(StyleRole::Preprocessor), preprocessor },
        { int(StyleRole::Operator), operatorColor },
        { int(StyleRole::Variable), value },
        { int(StyleRole::Value), value },
    };
}

Theme solarizedLight()
{
    Theme t = buildCommon("solarized-light", "Solarized Light", false);
    t.background = QColor("#fdf6e3");
    t.foreground = QColor("#657b83");
    t.caret = QColor("#657b83");
    t.selectionBg = QColor("#eee8d5");
    t.selectionFg = QColor("#586e75");
    t.currentLine = QColor("#eee8d5");
    t.lineNumberFg = QColor("#93a1a1");
    t.lineNumberBg = QColor("#eee8d5");
    t.foldFg = QColor("#93a1a1");
    t.matchedBraceBg = QColor("#eee8d5");
    t.bookmarkFg = QColor("#268bd2");
    t.indentGuide = QColor("#eee8d5");
    t.edgeLine = QColor("#eee8d5");
    t.roleOverrides = roles(QColor("#859900"), QColor("#859900"), QColor("#2aa198"),
                            QColor("#93a1a1"), QColor("#d33682"), QColor("#268bd2"),
                            QColor("#b58900"), QColor("#268bd2"), QColor("#cb4b16"),
                            QColor("#859900"), QColor("#cb4b16"));
    return t;
}

Theme solarizedDark()
{
    Theme t = buildCommon("solarized-dark", "Solarized Dark", true);
    t.background = QColor("#002b36");
    t.foreground = QColor("#839496");
    t.caret = QColor("#839496");
    t.selectionBg = QColor("#073642");
    t.selectionFg = QColor("#93a1a1");
    t.currentLine = QColor("#073642");
    t.lineNumberFg = QColor("#586e75");
    t.lineNumberBg = QColor("#00252e");
    t.foldFg = QColor("#586e75");
    t.matchedBraceBg = QColor("#073642");
    t.bookmarkFg = QColor("#268bd2");
    t.indentGuide = QColor("#073642");
    t.edgeLine = QColor("#073642");
    t.roleOverrides = roles(QColor("#859900"), QColor("#859900"), QColor("#2aa198"),
                            QColor("#586e75"), QColor("#d33682"), QColor("#268bd2"),
                            QColor("#b58900"), QColor("#268bd2"), QColor("#cb4b16"),
                            QColor("#859900"), QColor("#cb4b16"));
    return t;
}

Theme monokai()
{
    Theme t = buildCommon("monokai", "Monokai", true);
    t.background = QColor("#272822");
    t.foreground = QColor("#f8f8f2");
    t.caret = QColor("#f8f8f0");
    t.selectionBg = QColor("#49483e");
    t.selectionFg = QColor("#f8f8f2");
    t.currentLine = QColor("#3e3d32");
    t.lineNumberFg = QColor("#90908a");
    t.lineNumberBg = QColor("#1e1f1c");
    t.foldFg = QColor("#90908a");
    t.matchedBraceBg = QColor("#49483e");
    t.bookmarkFg = QColor("#a6e22e");
    t.indentGuide = QColor("#3b3a32");
    t.edgeLine = QColor("#49483e");
    t.roleOverrides = roles(QColor("#f92672"), QColor("#f92672"), QColor("#e6db74"),
                            QColor("#75715e"), QColor("#ae81ff"), QColor("#a6e22e"),
                            QColor("#66d9ef"), QColor("#f92672"), QColor("#f92672"),
                            QColor("#f92672"), QColor("#ae81ff"));
    return t;
}

Theme dracula()
{
    Theme t = buildCommon("dracula", "Dracula", true);
    t.background = QColor("#282a36");
    t.foreground = QColor("#f8f8f2");
    t.caret = QColor("#f8f8f2");
    t.selectionBg = QColor("#44475a");
    t.selectionFg = QColor("#ffffff");
    t.currentLine = QColor("#44475a");
    t.lineNumberFg = QColor("#6272a4");
    t.lineNumberBg = QColor("#21222c");
    t.foldFg = QColor("#6272a4");
    t.matchedBraceBg = QColor("#44475a");
    t.bookmarkFg = QColor("#8be9fd");
    t.indentGuide = QColor("#383a4a");
    t.edgeLine = QColor("#44475a");
    t.roleOverrides = roles(QColor("#ff79c6"), QColor("#ff79c6"), QColor("#f1fa8c"),
                            QColor("#6272a4"), QColor("#bd93f9"), QColor("#50fa7b"),
                            QColor("#8be9fd"), QColor("#ff79c6"), QColor("#ffb86c"),
                            QColor("#ff79c6"), QColor("#bd93f9"));
    return t;
}

Theme nord()
{
    Theme t = buildCommon("nord", "Nord", true);
    t.background = QColor("#2e3440");
    t.foreground = QColor("#d8dee9");
    t.caret = QColor("#d8dee9");
    t.selectionBg = QColor("#434c5e");
    t.selectionFg = QColor("#eceff4");
    t.currentLine = QColor("#3b4252");
    t.lineNumberFg = QColor("#4c566a");
    t.lineNumberBg = QColor("#2b303b");
    t.foldFg = QColor("#4c566a");
    t.matchedBraceBg = QColor("#3b4252");
    t.bookmarkFg = QColor("#88c0d0");
    t.indentGuide = QColor("#3b4252");
    t.edgeLine = QColor("#434c5e");
    t.roleOverrides = roles(QColor("#81a1c1"), QColor("#81a1c1"), QColor("#a3be8c"),
                            QColor("#616e88"), QColor("#b48ead"), QColor("#88c0d0"),
                            QColor("#8fbcbb"), QColor("#81a1c1"), QColor("#d08770"),
                            QColor("#81a1c1"), QColor("#ebcb8b"));
    return t;
}

Theme gruvboxDark()
{
    Theme t = buildCommon("gruvbox", "Gruvbox Dark", true);
    t.background = QColor("#282828");
    t.foreground = QColor("#ebdbb2");
    t.caret = QColor("#ebdbb2");
    t.selectionBg = QColor("#504945");
    t.selectionFg = QColor("#fbf1c7");
    t.currentLine = QColor("#3c3836");
    t.lineNumberFg = QColor("#928374");
    t.lineNumberBg = QColor("#1d2021");
    t.foldFg = QColor("#928374");
    t.matchedBraceBg = QColor("#3c3836");
    t.bookmarkFg = QColor("#8ec07c");
    t.indentGuide = QColor("#3c3836");
    t.edgeLine = QColor("#504945");
    t.roleOverrides = roles(QColor("#fb4934"), QColor("#fb4934"), QColor("#b8bb26"),
                            QColor("#928374"), QColor("#d3869b"), QColor("#8ec07c"),
                            QColor("#fabd2f"), QColor("#fb4934"), QColor("#fe8019"),
                            QColor("#fe8019"), QColor("#d3869b"));
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
    const auto override = roleOverrides.constFind(int(role));
    if (override != roleOverrides.constEnd())
        return override.value();

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
    static const QList<Theme> list{lightTheme(), darkTheme(), solarizedLight(), solarizedDark(),
                                   monokai(), dracula(), nord(), gruvboxDark()};
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
