#include "EditorWidget.h"
#include "Encoding.h"
#include "MainWindow.h"
#include "SearchEngine.h"
#include "TextOps.h"

#include <QClipboard>
#include <QFile>
#include <QGuiApplication>
#include <QTemporaryDir>
#include <QtTest>
#include <ScintillaEdit.h>

using namespace PadForge;

class SmokeTest : public QObject {
    Q_OBJECT

private slots:
    void newDocumentHoldsText();
    void openFileDetectsBom();
    void editorFindAndReplace();
    void mainWindowOpensPath();
    void editorTransformations();
    void bookmarkOperations();
};

void SmokeTest::newDocumentHoldsText() {
    MainWindow window;
    EditorWidget* editor = window.newDocument(QStringLiteral("hello\nworld"));
    QVERIFY(editor != nullptr);
    QCOMPARE(editor->documentText(), QStringLiteral("hello\nworld"));
    QCOMPARE(static_cast<int>(editor->editor()->lineCount()), 2);
}

void SmokeTest::openFileDetectsBom() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("bom.txt"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QByteArray("\xEF\xBB\xBF") + QByteArray("abc"));
    file.close();

    MainWindow window;
    EditorWidget* editor = window.openPath(path);
    QVERIFY(editor != nullptr);
    QCOMPARE(editor->documentText(), QStringLiteral("abc"));
    QVERIFY(editor->encoding().bom);
    QCOMPARE(editor->encoding().charset, Charset::Utf8Bom);
}

void SmokeTest::editorFindAndReplace() {
    EditorWidget editor;
    editor.setDocumentText(QStringLiteral("foo bar foo"));

    SearchOptions options;
    editor.highlightMatches(QStringLiteral("foo"), options);
    int count = SearchEngine::count(editor.documentText(), QStringLiteral("foo"), options);
    QCOMPARE(count, 2);

    int replaced = 0;
    const QString result = SearchEngine::replaceAll(editor.documentText(), QStringLiteral("foo"),
                                                    QStringLiteral("baz"), options, &replaced);
    QCOMPARE(replaced, 2);
    editor.setDocumentText(result);
    QCOMPARE(editor.documentText(), QStringLiteral("baz bar baz"));
}

void SmokeTest::mainWindowOpensPath() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("sample.cpp"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("int main() { return 0; }\n");
    file.close();

    MainWindow window;
    EditorWidget* editor = window.openPath(path);
    QVERIFY(editor != nullptr);
    QCOMPARE(editor->languageId(), QStringLiteral("cpp"));
}

void SmokeTest::editorTransformations() {
    EditorWidget editor;
    editor.setLanguage(QStringLiteral("cpp"));
    editor.setDocumentText(QStringLiteral("b\na\n"));
    editor.gotoLine(1);
    editor.toggleLineComment();
    QCOMPARE(editor.documentText(), QStringLiteral("// b\na\n"));

    editor.setDocumentText(QStringLiteral("a\nb\n"));
    editor.gotoLine(1);
    editor.duplicateCurrentLine();
    QCOMPARE(editor.documentText(), QStringLiteral("a\na\nb\n"));

    editor.setDocumentText(QStringLiteral("banana\napple\n"));
    editor.editor()->selectAll();
    editor.sortLinesAscending();
    QCOMPARE(editor.documentText(), QStringLiteral("apple\nbanana\n"));

    editor.setDocumentText(QStringLiteral("  hello\n"));
    editor.gotoLine(1);
    editor.trimTrailingWhitespace();
    editor.trimLeadingWhitespace();
    QCOMPARE(editor.documentText(), QStringLiteral("hello\n"));

    const QString numbered =
        TextOps::columnInsertNumbers(QStringLiteral("a\nb\n"), 1, 1, 1, 2, 0, -1, true, false, 4);
    QCOMPARE(numbered, QStringLiteral("01a\n02b\n"));
}

void SmokeTest::bookmarkOperations() {
    EditorWidget editor;
    editor.setDocumentText(QStringLiteral("x\ny\nz\n"));
    editor.gotoLine(2);
    editor.toggleBookmark();
    editor.gotoLine(3);
    editor.toggleBookmark();
    QCOMPARE(editor.bookmarkLines(), QList<int>({1, 2}));
    editor.copyBookmarkedLines();
    QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("y\nz\n"));
    editor.invertBookmarks();
    QCOMPARE(editor.bookmarkLines(), QList<int>({0}));
}

QTEST_MAIN(SmokeTest)
#include "test_smoke.moc"
