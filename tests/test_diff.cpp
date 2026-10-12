#include "Diff.h"

#include <QRandomGenerator>
#include <QSet>
#include <QtGlobal>
#include <QtTest>

using namespace PadForge;

namespace PadForge {
static bool operator==(const DiffHunk& lhs, const DiffHunk& rhs) {
    return lhs.oldLine == rhs.oldLine && lhs.newLine == rhs.newLine &&
           lhs.oldLineCount == rhs.oldLineCount && lhs.newLineCount == rhs.newLineCount;
}
} // namespace PadForge

class DiffTest : public QObject {
    Q_OBJECT

private slots:
    void identicalTexts();
    void pureInsertion();
    void pureDeletion();
    void replacement();
    void runOfChanges();
    void emptyInputs();
    void trailingNewlineIsIgnored();
    void reconstructsNewText(); // hand cases + randomized round-trips
};

QString joined(const QStringList& lines) {
    QString text;
    for (const QString& line : lines)
        text += line + QLatin1Char('\n');
    return text;
}

void DiffTest::identicalTexts() {
    const QString text = QStringLiteral("alpha\nbeta\ngamma\n");
    QCOMPARE(Diff::hunks(text, text), QList<DiffHunk>());
    QCOMPARE(Diff::removedLines(text, text), QList<int>());
    QCOMPARE(Diff::addedLines(text, text), QList<int>());
}

void DiffTest::pureInsertion() {
    const QString oldText = QStringLiteral("a\nb\nc\n");
    const QString newText = QStringLiteral("a\nx\nb\nc\n");
    const QList<DiffHunk> hunks = Diff::hunks(oldText, newText);
    QCOMPARE(hunks.size(), 1);
    QCOMPARE(hunks[0].oldLine, 2);
    QCOMPARE(hunks[0].newLine, 2);
    QCOMPARE(hunks[0].oldLineCount, 0);
    QCOMPARE(hunks[0].newLineCount, 1);
    QCOMPARE(Diff::removedLines(oldText, newText), QList<int>());
    QCOMPARE(Diff::addedLines(oldText, newText), QList<int>({2}));
}

void DiffTest::pureDeletion() {
    const QString oldText = QStringLiteral("a\nx\nb\nc\n");
    const QString newText = QStringLiteral("a\nb\nc\n");
    const QList<DiffHunk> hunks = Diff::hunks(oldText, newText);
    QCOMPARE(hunks.size(), 1);
    QCOMPARE(hunks[0].oldLine, 2);
    QCOMPARE(hunks[0].newLine, 2);
    QCOMPARE(hunks[0].oldLineCount, 1);
    QCOMPARE(hunks[0].newLineCount, 0);
    QCOMPARE(Diff::removedLines(oldText, newText), QList<int>({2}));
    QCOMPARE(Diff::addedLines(oldText, newText), QList<int>());
}

void DiffTest::replacement() {
    const QString oldText = QStringLiteral("a\nx\nb\n");
    const QString newText = QStringLiteral("a\ny\nb\n");
    const QList<DiffHunk> hunks = Diff::hunks(oldText, newText);
    QCOMPARE(hunks.size(), 1);
    QCOMPARE(hunks[0].oldLine, 2);
    QCOMPARE(hunks[0].newLine, 2);
    QCOMPARE(hunks[0].oldLineCount, 1);
    QCOMPARE(hunks[0].newLineCount, 1);
    QCOMPARE(Diff::removedLines(oldText, newText), QList<int>({2}));
    QCOMPARE(Diff::addedLines(oldText, newText), QList<int>({2}));
}

void DiffTest::runOfChanges() {
    const QString oldText = QStringLiteral("a\n1\n2\nz\n");
    const QString newText = QStringLiteral("a\nX\nY\nz\n");
    const QList<DiffHunk> hunks = Diff::hunks(oldText, newText);
    QCOMPARE(hunks.size(), 1);
    QCOMPARE(hunks[0].oldLine, 2);
    QCOMPARE(hunks[0].newLine, 2);
    QCOMPARE(hunks[0].oldLineCount, 2);
    QCOMPARE(hunks[0].newLineCount, 2);
    QCOMPARE(Diff::removedLines(oldText, newText), QList<int>({2, 3}));
    QCOMPARE(Diff::addedLines(oldText, newText), QList<int>({2, 3}));
}

void DiffTest::emptyInputs() {
    QCOMPARE(Diff::hunks(QString(), QString()), QList<DiffHunk>());
    QCOMPARE(Diff::hunks(QString(), QString()), QList<DiffHunk>());

    const QList<DiffHunk> insert = Diff::hunks(QString(), QStringLiteral("x\ny\n"));
    QCOMPARE(insert.size(), 1);
    QCOMPARE(insert[0].oldLine, 1);
    QCOMPARE(insert[0].oldLineCount, 0);
    QCOMPARE(insert[0].newLineCount, 2);
    QCOMPARE(Diff::addedLines(QString(), QStringLiteral("x\ny\n")), QList<int>({1, 2}));

    const QList<DiffHunk> del = Diff::hunks(QStringLiteral("x\ny\n"), QString());
    QCOMPARE(del.size(), 1);
    QCOMPARE(del[0].newLineCount, 0);
    QCOMPARE(del[0].oldLineCount, 2);
    QCOMPARE(Diff::removedLines(QStringLiteral("x\ny\n"), QString()), QList<int>({1, 2}));
}

void DiffTest::trailingNewlineIsIgnored() {
    QCOMPARE(Diff::hunks(QStringLiteral("a\n"), QStringLiteral("a")), QList<DiffHunk>());
    const QList<DiffHunk> hunks = Diff::hunks(QStringLiteral("a\n"), QStringLiteral("a\nb"));
    QCOMPARE(hunks.size(), 1);
    QCOMPARE(hunks[0].newLine, 2);
    QCOMPARE(hunks[0].newLineCount, 1);
}

// Rebuild the new text from the old text plus the reported hunks; also checks
// that hunk line numbers stay in bounds and never overlap.
static bool reconstruct(const QStringList& oldLines, const QStringList& newLines,
                        const QList<DiffHunk>& hunks, QString* failure) {
    QStringList out;
    int prevOldEnd = 0;
    int prevNewEnd = 0;
    for (const DiffHunk& hunk : hunks) {
        const int oldStart = hunk.oldLine - 1;
        const int newStart = hunk.newLine - 1;
        if (oldStart < prevOldEnd || newStart < prevNewEnd ||
            oldStart + hunk.oldLineCount > oldLines.size() ||
            newStart + hunk.newLineCount > newLines.size()) {
            *failure = QStringLiteral("hunk out of bounds/overlap");
            return false;
        }
        // Copy the equal region preceding this hunk.
        if (newStart - prevNewEnd != oldStart - prevOldEnd) {
            *failure = QStringLiteral("unequal gap lengths");
            return false;
        }
        for (int i = prevNewEnd; i < newStart; ++i)
            out.append(newLines[i]);
        for (int i = 0; i < hunk.newLineCount; ++i)
            out.append(newLines[newStart + i]);
        prevOldEnd = oldStart + hunk.oldLineCount;
        prevNewEnd = newStart + hunk.newLineCount;
        if (hunk.oldLineCount == 0 && hunk.newLineCount == 0) {
            *failure = QStringLiteral("hunk with no content");
            return false;
        }
    }
    for (int i = prevNewEnd; i < newLines.size(); ++i)
        out.append(newLines[i]);
    if (out != newLines) {
        *failure = QStringLiteral("reconstruction mismatch");
        return false;
    }
    return true;
}

void DiffTest::reconstructsNewText() {
    const QStringList a = {"a", "b", "c", "d", "e"};
    const QStringList b = {"a", "x", "c", "d", "z", "e"};
    QString failure;
    QVERIFY2(reconstruct(a, b, Diff::hunks(joined(a), joined(b)), &failure), qPrintable(failure));

    // Randomized round-trips: rebuilding "new" from "old" + hunks must be exact.
    QRandomGenerator rng(20261012);
    for (int trial = 0; trial < 200; ++trial) {
        const int n = int(rng.bounded(0, 12));
        const int m = int(rng.bounded(0, 12));
        QStringList oldLines;
        QStringList newLines;
        for (int i = 0; i < n; ++i)
            oldLines.append(QStringLiteral("l%1").arg(int(rng.bounded(0, 6))));
        for (int i = 0; i < m; ++i)
            newLines.append(QStringLiteral("l%1").arg(int(rng.bounded(0, 6))));
        const QList<DiffHunk> hunks = Diff::hunks(joined(oldLines), joined(newLines));
        if (oldLines.isEmpty() && newLines.isEmpty()) {
            QVERIFY(hunks.isEmpty());
            continue;
        }
        QVERIFY2(reconstruct(oldLines, newLines, hunks, &failure), qPrintable(failure));
    }
}

QTEST_APPLESS_MAIN(DiffTest)
#include "test_diff.moc"