#include <QtTest>

#include <ScintillaEdit.h>

#include <QFile>
#include <QTemporaryDir>

#include "EditorWidget.h"
#include "Encoding.h"
#include "MainWindow.h"
#include "SearchEngine.h"

using namespace PadForge;

class SmokeTest : public QObject {
    Q_OBJECT

private slots:
    void newDocumentHoldsText();
    void openFileDetectsBom();
    void editorFindAndReplace();
    void mainWindowOpensPath();
};

void SmokeTest::newDocumentHoldsText()
{
    MainWindow window;
    EditorWidget* editor = window.newDocument(QStringLiteral("hello\nworld"));
    QVERIFY(editor != nullptr);
    QCOMPARE(editor->documentText(), QStringLiteral("hello\nworld"));
    QCOMPARE(static_cast<int>(editor->editor()->lineCount()), 2);
}

void SmokeTest::openFileDetectsBom()
{
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

void SmokeTest::editorFindAndReplace()
{
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

void SmokeTest::mainWindowOpensPath()
{
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

QTEST_MAIN(SmokeTest)
#include "test_smoke.moc"
