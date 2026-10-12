#pragma once

#include <QList>
#include <QString>

namespace PadForge {

// A single entry of the Function List / document outline.
struct OutlineItem {
    QString name;
    QString kind;      // "class", "function", "section", ...
    QString signature; // e.g. "foo(int a, int b)" for functions
    int line = 1;      // 1-based line number
    int level = 0;     // nesting depth derived from indentation
};

// Best-effort, regex-based outline extraction for the current document.
// This intentionally favours robustness (never throwing, tolerating broken
// code) over exhaustive parsing.
class OutlineParser {
public:
    static QList<OutlineItem> parse(const QString& text, const QString& languageId);

    // True when `languageId` has a dedicated set of outline rules.
    static bool supports(const QString& languageId);
};

} // namespace PadForge
