#pragma once

#include <QList>
#include <QString>

namespace PadForge {

// One contiguous run of differing lines between two texts. Line numbers are
// 1-based. A pure insertion has oldLineCount == 0, a pure deletion has
// newLineCount == 0; replacement regions touch both sides.
struct DiffHunk {
    int oldLine = 0;      // first differing 1-based line in the old text
    int newLine = 0;      // first differing 1-based line in the new text
    int oldLineCount = 0; // number of differing old lines (0 on pure insert)
    int newLineCount = 0; // number of differing new lines (0 on pure delete)
};

// Line-oriented longest-common-subsequence diff between two documents.
// Line numbers are 1-based.
class Diff {
public:
    // The hunks describing how "old" grows into "new" (equal regions skipped).
    static QList<DiffHunk> hunks(const QString& oldText, const QString& newText);

    // 1-based line numbers in the old text that were deleted or replaced.
    static QList<int> removedLines(const QString& oldText, const QString& newText);

    // 1-based line numbers in the new text that were inserted or replaced.
    static QList<int> addedLines(const QString& oldText, const QString& newText);

    // Cheap line split (drops a single trailing newline).
    static QStringList splitLines(const QString& text);
};

} // namespace PadForge