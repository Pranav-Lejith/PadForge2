#include "TextOps.h"

#include <QtTest>

using namespace PadForge;

class TextOpsTest : public QObject {
    Q_OBJECT

private slots:
    void sortAscendingDescending();
    void sortWithDuplicates();
    void removeDuplicates();
    void removeEmptyLinesOps();
    void caseConversions();
    void trimWhitespace();
    void indentAndUnindent();
    void tabifyUntabify();
    void lineCommentToggle();
    void blockCommentToggle();
    void duplicateMoveJoinSplit();
    void columnInsert();
    void statistics();
};

void TextOpsTest::sortAscendingDescending() {
    const QString text = QStringLiteral("banana\napple\ncherry\n");
    QCOMPARE(TextOps::sortLines(text, true, true, false),
             QStringLiteral("apple\nbanana\ncherry\n"));
    QCOMPARE(TextOps::sortLines(text, false, true, false),
             QStringLiteral("cherry\nbanana\napple\n"));
}

void TextOpsTest::sortWithDuplicates() {
    const QString text = QStringLiteral("b\na\nb\na\n");
    QCOMPARE(TextOps::sortLines(text, true, true, true), QStringLiteral("a\nb\n"));
}

void TextOpsTest::removeDuplicates() {
    QCOMPARE(TextOps::removeDuplicateLines(QStringLiteral("a\nb\na\nc\nb\n")),
             QStringLiteral("a\nb\nc\n"));
    QCOMPARE(TextOps::removeConsecutiveDuplicateLines(QStringLiteral("a\na\nb\nb\na\n")),
             QStringLiteral("a\nb\na\n"));
}

void TextOpsTest::removeEmptyLinesOps() {
    const QString messy = QStringLiteral("\n\na\n\n\nb\n\n\n");
    QCOMPARE(TextOps::removeEmptyLines(messy), QStringLiteral("a\nb\n"));
    QCOMPARE(TextOps::removeEmptyLinesAtStart(QStringLiteral("\n\na\nb\n")),
             QStringLiteral("a\nb\n"));
    QCOMPARE(TextOps::removeEmptyLinesAtEnd(QStringLiteral("a\nb\n\n\n")),
             QStringLiteral("a\nb\n"));
    QCOMPARE(TextOps::removeEmptyLinesInside(QStringLiteral("\n\na\nb\n\n\n")),
             QStringLiteral("a\nb\n"));
    QCOMPARE(TextOps::removeEmptyLinesInside(QStringLiteral("a\n\nb\n")),
             QStringLiteral("a\n\nb\n"));
}

void TextOpsTest::caseConversions() {
    QCOMPARE(TextOps::convertCase(QStringLiteral("Hello World"), CaseMode::Upper),
             QStringLiteral("HELLO WORLD"));
    QCOMPARE(TextOps::convertCase(QStringLiteral("Hello World"), CaseMode::Lower),
             QStringLiteral("hello world"));
    QCOMPARE(TextOps::convertCase(QStringLiteral("hello world"), CaseMode::Title),
             QStringLiteral("Hello World"));
    QCOMPARE(TextOps::convertCase(QStringLiteral("hello. world"), CaseMode::Sentence),
             QStringLiteral("Hello. World"));
    QCOMPARE(TextOps::convertCase(QStringLiteral("HELLO world"), CaseMode::Invert),
             QStringLiteral("hello WORLD"));
}

void TextOpsTest::trimWhitespace() {
    QCOMPARE(TextOps::trimTrailing(QStringLiteral("a  \nb\t\n")), QStringLiteral("a\nb\n"));
    QCOMPARE(TextOps::trimLeading(QStringLiteral("  a\n\tb\n")), QStringLiteral("a\nb\n"));
    QCOMPARE(TextOps::trimBoth(QStringLiteral("  a  \n b \n")), QStringLiteral("a\nb\n"));
}

void TextOpsTest::indentAndUnindent() {
    QCOMPARE(TextOps::indent(QStringLiteral("a\nb\n"), 4), QStringLiteral("    a\n    b\n"));
    QCOMPARE(TextOps::unindent(QStringLiteral("    a\n  b\n"), 4), QStringLiteral("a\nb\n"));
    QCOMPARE(TextOps::unindent(QStringLiteral("\ta\n"), 4), QStringLiteral("a\n"));
}

void TextOpsTest::tabifyUntabify() {
    QCOMPARE(TextOps::untabify(QStringLiteral("\tx\n"), 4), QStringLiteral("    x\n"));
    QCOMPARE(TextOps::tabify(QStringLiteral("    x\n"), 4), QStringLiteral("\tx\n"));
}

void TextOpsTest::lineCommentToggle() {
    const QString token = QStringLiteral("//");
    const QString commented = TextOps::toggleLineComment(QStringLiteral("a\nb\n"), token);
    QCOMPARE(commented, QStringLiteral("// a\n// b\n"));
    QCOMPARE(TextOps::toggleLineComment(commented, token), QStringLiteral("a\nb\n"));
    QCOMPARE(TextOps::toggleLineComment(QStringLiteral("  a\n"), token),
             QStringLiteral("  // a\n"));
}

void TextOpsTest::blockCommentToggle() {
    const QString wrapped = TextOps::toggleBlockComment(QStringLiteral("code"),
                                                        QStringLiteral("/*"), QStringLiteral("*/"));
    QCOMPARE(wrapped, QStringLiteral("/* code */"));
    QCOMPARE(TextOps::toggleBlockComment(wrapped, QStringLiteral("/*"), QStringLiteral("*/")),
             QStringLiteral("code"));
}

void TextOpsTest::duplicateMoveJoinSplit() {
    QCOMPARE(TextOps::duplicateLines(QStringLiteral("a\nb\n")), QStringLiteral("a\na\nb\nb\n"));
    QCOMPARE(TextOps::joinLinesOperation(QStringLiteral("a\nb\nc")), QStringLiteral("a b c"));
    QCOMPARE(TextOps::splitLinesOperation(QStringLiteral("a b c")), QStringLiteral("a\nb\nc"));
}

void TextOpsTest::columnInsert() {
    const QString text = QStringLiteral("x\nyyy\n");
    QCOMPARE(TextOps::columnInsertText(text, 3, QStringLiteral("*"), 0, -1, true, false, 4),
             QStringLiteral("x *\nyy*y\n"));
    QCOMPARE(
        TextOps::columnInsertNumbers(QStringLiteral("a\nb\n"), 1, 1, 1, 2, 0, -1, true, false, 4),
        QStringLiteral("01a\n02b\n"));
    QCOMPARE(TextOps::columnInsertText(QStringLiteral("a\n\nb\n"), 1, QStringLiteral("*"), 0, -1,
                                       false, false, 4),
             QStringLiteral("*a\n\n*b\n"));
}

void TextOpsTest::statistics() {
    const QString text = QStringLiteral("one two three\nfour\n");
    QCOMPARE(TextOps::wordCount(text), 4);
    QCOMPARE(TextOps::lineCount(text), 2);
    QCOMPARE(TextOps::characterCount(text), text.size());
}

QTEST_APPLESS_MAIN(TextOpsTest)
#include "test_textops.moc"
