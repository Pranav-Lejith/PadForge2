#pragma once

#include <QString>

namespace PadForge {

// User-visible application preferences, persisted with QSettings.
struct AppSettings {
    QString theme = QStringLiteral("light");
    QString fontFamily = QStringLiteral("Monospace");
    int fontSize = 11;
    QString tabMode = QStringLiteral("spaces"); // "spaces" or "tabs"
    int tabWidth = 4;

    bool wordWrap = false;
    bool showLineNumbers = true;
    bool showFoldMargin = true;
    bool showBookmarkMargin = true;
    bool showWhitespace = false;
    bool showEol = false;
    bool showIndentGuides = false;
    bool highlightCurrentLine = true;
    bool autoCompletion = true;
    bool autoCloseBrackets = true;
    bool autoCloseTags = true;
    bool calltips = true;
    bool showChangeHistory = true;

    int edgeColumn = 0;       // 0 disables the edge line
    int splitOrientation = 0; // 0 = horizontal, 1 = vertical
    bool alwaysOnTop = false;

    void load();
    void save() const;
};

} // namespace PadForge
