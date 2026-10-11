#include "Settings.h"

#include <QSettings>

namespace PadForge {

namespace {

QSettings makeSettings() {
    return QSettings(QStringLiteral("PadForge"), QStringLiteral("PadForge"));
}

} // namespace

void AppSettings::load() {
    QSettings s = makeSettings();
    theme = s.value(QStringLiteral("editor/theme"), theme).toString();
    fontFamily = s.value(QStringLiteral("editor/fontFamily"), fontFamily).toString();
    fontSize = s.value(QStringLiteral("editor/fontSize"), fontSize).toInt();
    tabMode = s.value(QStringLiteral("editor/tabMode"), tabMode).toString();
    tabWidth = s.value(QStringLiteral("editor/tabWidth"), tabWidth).toInt();

    wordWrap = s.value(QStringLiteral("editor/wordWrap"), wordWrap).toBool();
    showLineNumbers = s.value(QStringLiteral("editor/showLineNumbers"), showLineNumbers).toBool();
    showFoldMargin = s.value(QStringLiteral("editor/showFoldMargin"), showFoldMargin).toBool();
    showBookmarkMargin =
        s.value(QStringLiteral("editor/showBookmarkMargin"), showBookmarkMargin).toBool();
    showWhitespace = s.value(QStringLiteral("editor/showWhitespace"), showWhitespace).toBool();
    showEol = s.value(QStringLiteral("editor/showEol"), showEol).toBool();
    showIndentGuides =
        s.value(QStringLiteral("editor/showIndentGuides"), showIndentGuides).toBool();
    highlightCurrentLine =
        s.value(QStringLiteral("editor/highlightCurrentLine"), highlightCurrentLine).toBool();
    autoCompletion = s.value(QStringLiteral("editor/autoCompletion"), autoCompletion).toBool();
    autoCloseBrackets =
        s.value(QStringLiteral("editor/autoCloseBrackets"), autoCloseBrackets).toBool();

    edgeColumn = s.value(QStringLiteral("editor/edgeColumn"), edgeColumn).toInt();
    splitOrientation = s.value(QStringLiteral("view/splitOrientation"), splitOrientation).toInt();
    alwaysOnTop = s.value(QStringLiteral("view/alwaysOnTop"), alwaysOnTop).toBool();
}

void AppSettings::save() const {
    QSettings s = makeSettings();
    s.setValue(QStringLiteral("editor/theme"), theme);
    s.setValue(QStringLiteral("editor/fontFamily"), fontFamily);
    s.setValue(QStringLiteral("editor/fontSize"), fontSize);
    s.setValue(QStringLiteral("editor/tabMode"), tabMode);
    s.setValue(QStringLiteral("editor/tabWidth"), tabWidth);

    s.setValue(QStringLiteral("editor/wordWrap"), wordWrap);
    s.setValue(QStringLiteral("editor/showLineNumbers"), showLineNumbers);
    s.setValue(QStringLiteral("editor/showFoldMargin"), showFoldMargin);
    s.setValue(QStringLiteral("editor/showBookmarkMargin"), showBookmarkMargin);
    s.setValue(QStringLiteral("editor/showWhitespace"), showWhitespace);
    s.setValue(QStringLiteral("editor/showEol"), showEol);
    s.setValue(QStringLiteral("editor/showIndentGuides"), showIndentGuides);
    s.setValue(QStringLiteral("editor/highlightCurrentLine"), highlightCurrentLine);
    s.setValue(QStringLiteral("editor/autoCompletion"), autoCompletion);
    s.setValue(QStringLiteral("editor/autoCloseBrackets"), autoCloseBrackets);

    s.setValue(QStringLiteral("editor/edgeColumn"), edgeColumn);
    s.setValue(QStringLiteral("view/splitOrientation"), splitOrientation);
    s.setValue(QStringLiteral("view/alwaysOnTop"), alwaysOnTop);
    s.sync();
}

} // namespace PadForge
