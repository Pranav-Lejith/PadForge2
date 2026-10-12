#include "Diff.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace PadForge {

QStringList Diff::splitLines(const QString& text) {
    QStringList out;
    int start = 0;
    for (int i = 0; i < text.size(); ++i) {
        if (text.at(i) == QLatin1Char('\n')) {
            out.append(text.mid(start, i - start));
            start = i + 1;
        }
    }
    if (start < text.size())
        out.append(text.mid(start));
    return out;
}

namespace {

// Bounding the DP grid keeps a single compare pass well under ~32 MB.
constexpr qsizetype kMaxCells = 8 * 1024 * 1024;
constexpr int kMaxLines = 200000;

struct Differ {
    const QStringList& a; // old lines
    const QStringList& b; // new lines
    int n = a.size();
    int m = b.size();

    QList<DiffHunk> run() {
        if (n == 0 && m == 0)
            return {};
        if (qsizetype(n) * qsizetype(m) > kMaxCells || n + m > kMaxLines)
            return {DiffHunk{1, 1, n, m}};
        return finish(backtrack(table()));
    }

private:
    std::vector<std::uint16_t> table() {
        std::vector<std::uint16_t> lcs((n + 1) * (m + 1), 0);
        for (int i = 1; i <= n; ++i) {
            const int row = i * (m + 1);
            const int prevRow = (i - 1) * (m + 1);
            for (int j = 1; j <= m; ++j) {
                if (a[i - 1] == b[j - 1])
                    lcs[row + j] = std::uint16_t(lcs[prevRow + j - 1] + 1);
                else
                    lcs[row + j] =
                        std::uint16_t(qMax(int(lcs[prevRow + j]), int(lcs[row + j - 1])));
            }
        }
        return lcs;
    }

    // Reconstruct the edit script backwards; ops are 0 equal, 1 insert (b),
    // 2 delete (a), in reverse document order.
    QList<int> backtrack(const std::vector<std::uint16_t>& lcs) {
        QList<int> ops;
        int i = n, j = m;
        while (i > 0 && j > 0) {
            if (a[i - 1] == b[j - 1]) {
                ops.append(0);
                --i;
                --j;
            } else if (lcs[(i - 1) * (m + 1) + j] > lcs[i * (m + 1) + j - 1]) {
                ops.append(2);
                --i;
            } else {
                ops.append(1);
                --j;
            }
        }
        while (i > 0) {
            ops.append(2);
            --i;
        }
        while (j > 0) {
            ops.append(1);
            --j;
        }
        return ops;
    }

    QList<DiffHunk> finish(const QList<int>& ops) {
        QList<DiffHunk> hunks;
        int oldCursor = 0, newCursor = 0;
        int runOld = 0, runNew = 0, runOldCount = 0, runNewCount = 0;
        for (qsizetype t = ops.size() - 1; t >= 0; --t) {
            const int op = ops.at(t);
            if (op == 0) {
                if (runOldCount > 0 || runNewCount > 0) {
                    hunks.append(DiffHunk{runOld + 1, runNew + 1, runOldCount, runNewCount});
                    runOldCount = runNewCount = 0;
                }
                ++oldCursor;
                ++newCursor;
            } else if (op == 2) {
                if (runOldCount == 0 && runNewCount == 0) {
                    runOld = oldCursor;
                    runNew = newCursor;
                }
                ++runOldCount;
                ++oldCursor;
            } else {
                if (runOldCount == 0 && runNewCount == 0) {
                    runOld = oldCursor;
                    runNew = newCursor;
                }
                ++runNewCount;
                ++newCursor;
            }
        }
        if (runOldCount > 0 || runNewCount > 0)
            hunks.append(DiffHunk{runOld + 1, runNew + 1, runOldCount, runNewCount});
        return hunks;
    }
};

QList<int> lineSet(const QList<DiffHunk>& hunks, bool oldSide) {
    QList<int> out;
    for (const DiffHunk& hunk : hunks) {
        const int start = oldSide ? hunk.oldLine : hunk.newLine;
        const int count = oldSide ? hunk.oldLineCount : hunk.newLineCount;
        for (int i = 0; i < count; ++i)
            out.append(start + i);
    }
    return out;
}

} // namespace

QList<DiffHunk> Diff::hunks(const QString& oldText, const QString& newText) {
    QStringList a = splitLines(oldText);
    QStringList b = splitLines(newText);
    return Differ{a, b}.run();
}

QList<int> Diff::removedLines(const QString& oldText, const QString& newText) {
    return lineSet(hunks(oldText, newText), true);
}

QList<int> Diff::addedLines(const QString& oldText, const QString& newText) {
    return lineSet(hunks(oldText, newText), false);
}

} // namespace PadForge