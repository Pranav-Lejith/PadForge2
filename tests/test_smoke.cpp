#include "CharacterMapPanel.h"
#include "Diff.h"
#include "EditorWidget.h"
#include "Encoding.h"
#include "FunctionListPanel.h"
#include "MainWindow.h"
#include "SearchEngine.h"
#include "SearchResultsPanel.h"
#include "TextOps.h"
#include "UpdateChecker.h"

#include <QClipboard>
#include <QFile>
#include <QGuiApplication>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QtTest>
#include <Scintilla.h>
#include <ScintillaEdit.h>
#include <unistd.h>

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
    void exportAndInsertOperations();
    void readOnlyDetection();
    void functionListPanelBuildsOutline();
    void characterMapBuildsGrid();
    void backupOnSaveWritesDotBak();
    void saveCopyAsLeavesDocumentUntouched();
    void saveRejectsUnrepresentableTextAndPreservesDirtyState();
    void failedSavePreservesDirtyState();
    void setFirstVisibleLineScrolls();
    void compareMarkersSetAndClear();
    void searchResultsPanelBuildsGroups();
    void documentRenameMovesFile();
    void closeOtherAndAllDocuments();
    void sortTabsReordersByName();
    void tabTitleReflectsModification();
    void foldCommandsToggleVisibility();
    void matchingBraceNavigation();
    void updateVersionComparison();
};

void SmokeTest::updateVersionComparison() {
    QVERIFY(UpdateChecker::isNewerVersion(QStringLiteral("0.1.0"), QStringLiteral("v0.2.0")));
    QVERIFY(UpdateChecker::isNewerVersion(QStringLiteral("1.9.9"), QStringLiteral("1.10.0")));
    QVERIFY(!UpdateChecker::isNewerVersion(QStringLiteral("0.2.0"), QStringLiteral("v0.2.0")));
    QVERIFY(!UpdateChecker::isNewerVersion(QStringLiteral("0.3.0"), QStringLiteral("v0.2.0")));
    QVERIFY(
        !UpdateChecker::isNewerVersion(QStringLiteral("development"), QStringLiteral("v0.2.0")));
}

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

void SmokeTest::exportAndInsertOperations() {
    EditorWidget editor;
    editor.setDocumentText(QStringLiteral("int x = 1; // <tag>\n"));
    editor.setLanguage(QStringLiteral("cpp"));

    const QString html = editor.exportHtml();
    QVERIFY(html.contains(QStringLiteral("<html>")));
    QVERIFY(html.contains(QStringLiteral("&lt;tag&gt;")));
    QVERIFY(html.contains(QStringLiteral("background-color:")));
    QVERIFY(html.contains(QStringLiteral("<span")));

    const QString rtf = editor.exportRtf();
    QVERIFY(rtf.startsWith(QStringLiteral("{\\rtf1")));
    QVERIFY(rtf.contains(QStringLiteral("\\par")));

    QCOMPARE(editor.editor()->send(SCI_GETCHANGEHISTORY), SC_CHANGE_HISTORY_INDICATORS);
    editor.setChangeHistoryEnabled(false);
    QCOMPARE(editor.editor()->send(SCI_GETCHANGEHISTORY), SC_CHANGE_HISTORY_DISABLED);
    editor.setChangeHistoryEnabled(true);

    editor.setDocumentText(QStringLiteral("|"));
    editor.gotoLine(1);
    editor.insertDateTime(QStringLiteral("yyyy-MM-dd"));
    QVERIFY(QRegularExpression(QStringLiteral("\\d{4}-\\d{2}-\\d{2}\\|"))
                .match(editor.documentText())
                .hasMatch());

    editor.setDocumentText(QString());
    editor.insertGuid(GuidFormat::Upper);
    QCOMPARE(editor.documentText().size(), 36);
    QCOMPARE(editor.documentText(), editor.documentText().toUpper());

    editor.setDocumentText(QString());
    editor.insertGuid(GuidFormat::BracesUpper);
    QVERIFY(editor.documentText().startsWith(QLatin1Char('{')));
    QVERIFY(editor.documentText().endsWith(QLatin1Char('}')));

    editor.setDocumentText(QString());
    editor.insertGuid(GuidFormat::Compact);
    QCOMPARE(editor.documentText().size(), 32);
}

void SmokeTest::readOnlyDetection() {
    if (::geteuid() == 0)
        QSKIP("File permission bits are not enforced for root");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("read-only.txt"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("hello\n");
    file.close();

    QVERIFY(QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::ReadGroup |
                                            QFileDevice::ReadOther));
    EditorWidget editor;
    QString error;
    QVERIFY(editor.openFile(path, &error));
    QVERIFY(editor.isReadOnly());

    QVERIFY(QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner));
    editor.setReadOnly(false);
    editor.setDocumentText(QStringLiteral("changed\n"));
    QVERIFY(editor.saveFileAs(path, &error));
    QVERIFY(!editor.isReadOnly());
    QCOMPARE(editor.documentText(), QStringLiteral("changed\n"));
}

void SmokeTest::functionListPanelBuildsOutline() {
    EditorWidget editor;
    editor.setLanguage(QStringLiteral("cpp"));
    editor.setDocumentText(QStringLiteral("class Foo {\n"
                                          "    void bar();\n"
                                          "};\n"
                                          "int baz(int a) {\n"
                                          "    return a;\n"
                                          "}\n"));

    FunctionListPanel panel;
    panel.attachEditor(&editor);
    panel.refresh();
    QCOMPARE(panel.topLevelItemCount(), 2);
    QTreeWidgetItem* top = panel.topLevelItem(0);
    QVERIFY(top->text(0).contains(QStringLiteral("Foo")));
    QCOMPARE(top->childCount(), 1);
    QVERIFY(top->child(0)->text(0).contains(QStringLiteral("bar")));
    QCOMPARE(top->child(0)->data(0, Qt::UserRole).toInt(), 2);
    QVERIFY(panel.topLevelItem(1)->text(0).contains(QStringLiteral("baz")));

    EditorWidget other;
    other.setLanguage(QStringLiteral("text"));
    panel.attachEditor(&other);
    panel.refresh();
    QCOMPARE(panel.topLevelItemCount(), 0);
}

void SmokeTest::characterMapBuildsGrid() {
    CharacterMapPanel panel;
    auto* grid = panel.findChild<QTableWidget*>();
    QVERIFY(grid != nullptr);
    QCOMPARE(grid->columnCount(), 16);
    QVERIFY(grid->rowCount() >= 6);
    QVERIFY(grid->item(0, 0) != nullptr);
    QCOMPARE(grid->item(0, 0)->text(), QStringLiteral(" ")); // U+0020 SPACE
}

void SmokeTest::backupOnSaveWritesDotBak() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("doc.txt"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("original\n");
    file.close();

    EditorWidget editor;
    QString error;
    QVERIFY(editor.openFile(path, &error));
    editor.setBackupOnSave(true);
    editor.setDocumentText(QStringLiteral("updated\n"));
    QVERIFY(editor.saveFileAs(path, &error));

    const QString bak = path + QStringLiteral(".bak");
    QVERIFY(QFileInfo::exists(bak));
    QFile bakFile(bak);
    QVERIFY(bakFile.open(QIODevice::ReadOnly));
    // The .bak holds the previous on-disk contents shunted aside by the save.
    QCOMPARE(QString::fromUtf8(bakFile.readAll()), QStringLiteral("original\n"));

    bakFile.close();
    editor.setDocumentText(QStringLiteral("final\n"));
    QVERIFY(editor.saveFileAs(path, &error));
    QVERIFY(bakFile.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(bakFile.readAll()), QStringLiteral("updated\n"));

    const QString savedAs = dir.filePath(QStringLiteral("second.txt"));
    QVERIFY(editor.saveFileAs(savedAs, &error));
    QVERIFY(!QFileInfo::exists(savedAs + QStringLiteral(".bak")));
}

void SmokeTest::saveCopyAsLeavesDocumentUntouched() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("orig.txt"));
    const QString copy = dir.filePath(QStringLiteral("copy.txt"));

    EditorWidget editor;
    editor.setDocumentText(QStringLiteral("a\nb\n"));
    QString error;
    QVERIFY(editor.saveFileAs(path, &error));

    editor.setDocumentText(QStringLiteral("a\nb\nc\n"));
    QVERIFY(editor.saveCopyAs(copy, &error));
    QCOMPARE(editor.filePath(), path);
    QVERIFY(editor.isModified());

    QFile copyFile(copy);
    QVERIFY(copyFile.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(copyFile.readAll()), QStringLiteral("a\nb\nc\n"));
}

void SmokeTest::saveRejectsUnrepresentableTextAndPreservesDirtyState() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    EditorWidget editor;
    editor.setDocumentText(QStringLiteral("snowman \u2603"));
    editor.setEncoding({Charset::Latin1, false}, false);

    const QString path = dir.filePath(QStringLiteral("latin1.txt"));
    QString error;
    QVERIFY(!editor.saveFileAs(path, &error));
    QVERIFY(error.contains(QStringLiteral("cannot be saved as Latin-1"), Qt::CaseInsensitive));
    QVERIFY(!QFileInfo::exists(path));
    QVERIFY(editor.isModified());
}

void SmokeTest::failedSavePreservesDirtyState() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    EditorWidget editor;
    editor.setDocumentText(QStringLiteral("keep this"));
    QString error;
    QVERIFY(!editor.saveFileAs(dir.filePath(QStringLiteral("missing/out.txt")), &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(editor.isModified());
}

void SmokeTest::setFirstVisibleLineScrolls() {
    EditorWidget editor;
    QString text;
    for (int i = 0; i < 100; ++i)
        text += QStringLiteral("line %1\n").arg(i);
    editor.setDocumentText(text);

    QCOMPARE(editor.firstVisibleLine(), 0);
    editor.setFirstVisibleLine(10);
    QCOMPARE(editor.firstVisibleLine(), 10);
    editor.setFirstVisibleLine(0);
    QCOMPARE(editor.firstVisibleLine(), 0);
}

void SmokeTest::compareMarkersSetAndClear() {
    const QString a = QStringLiteral("one\ntwo\nthree\nfour\nfive\n");
    const QString b = QStringLiteral("one\nTWO\nthree\nfour\nFIVE\nsix\n");
    EditorWidget editor;
    editor.setDocumentText(a);
    editor.setCompareMarkers(Diff::removedLines(a, b), Diff::addedLines(a, b));
    QCOMPARE(editor.documentText(), a);
    editor.clearCompareMarkers();
    QCOMPARE(editor.documentText(), a);

    // Out-of-range and edge lines must be harmless.
    editor.setCompareMarkers({-1, 0, 1, 2, 99}, {3, 1000});
    editor.clearCompareMarkers();
    QCOMPARE(editor.documentText(), a);
}

void SmokeTest::searchResultsPanelBuildsGroups() {
    EditorWidget one;
    one.setDocumentText(QStringLiteral("foo here\nnothing\n"));
    EditorWidget two;
    two.setDocumentText(QStringLiteral("and foo\ntwo foo lines\n"));

    SearchResultsPanel panel;
    panel.begin(QStringLiteral("foo"));
    panel.addHit(&one, 1, 1, QStringLiteral("foo here"));
    panel.addHit(&two, 1, 5, QStringLiteral("and foo"));
    panel.addHit(&two, 2, 5, QStringLiteral("two foo lines"));
    panel.finish(3);

    auto* tree = panel.findChild<QTreeWidget*>();
    QVERIFY(tree != nullptr);
    QCOMPARE(tree->topLevelItemCount(), 2);
    QTreeWidgetItem* second = tree->topLevelItem(1);
    QCOMPARE(second->childCount(), 2);
    QCOMPARE(second->child(0)->text(1), QStringLiteral("1"));
    QCOMPARE(second->child(0)->text(2), QStringLiteral("5"));
    QCOMPARE(second->child(1)->data(0, Qt::UserRole + 1).toInt(), 2);
}

void SmokeTest::documentRenameMovesFile() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("old.txt"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("hello\n");
    file.close();

    MainWindow window;
    EditorWidget* editor = window.openPath(path);
    QVERIFY(editor != nullptr);
    QVERIFY(window.documentCount() >= 1);

    const QString renamed = dir.filePath(QStringLiteral("new.txt"));
    window.renameCurrentDocument(renamed);
    QCOMPARE(editor->filePath(), renamed);
    QVERIFY(QFileInfo::exists(renamed));
    QVERIFY(!QFileInfo::exists(path));
    QVERIFY(window.documentTabTitles().contains(QStringLiteral("new.txt")));
}

void SmokeTest::closeOtherAndAllDocuments() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QStringList paths;
    for (int i = 0; i < 3; ++i) {
        const QString path = dir.filePath(QStringLiteral("f%1.txt").arg(i));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("data\n");
        file.close();
        paths << path;
    }

    MainWindow window;
    window.openPaths(paths);
    // Three opened files plus the initial untitled document.
    QCOMPARE(window.documentCount(), 4);

    window.closeOtherDocuments();
    QCOMPARE(window.documentCount(), 1);

    window.closeAllDocuments();
    QCOMPARE(window.documentCount(), 0);
}

void SmokeTest::sortTabsReordersByName() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QStringList names = {QStringLiteral("b.txt"), QStringLiteral("a.txt"),
                               QStringLiteral("c.txt")};
    QStringList paths;
    for (const QString& name : names) {
        const QString path = dir.filePath(name);
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("x\n");
        file.close();
        paths << path;
    }

    MainWindow window;
    window.openPaths(paths);
    window.sortTabs(false);

    const QStringList titles = window.documentTabTitles();
    const int a = titles.indexOf(QStringLiteral("a.txt"));
    const int b = titles.indexOf(QStringLiteral("b.txt"));
    const int c = titles.indexOf(QStringLiteral("c.txt"));
    QVERIFY(a >= 0 && b >= 0 && c >= 0);
    QVERIFY(a < b);
    QVERIFY(b < c);
}

void SmokeTest::tabTitleReflectsModification() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("mod.txt"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("original\n");
    file.close();

    MainWindow window;
    EditorWidget* editor = window.openPath(path);
    QVERIFY(editor != nullptr);
    QVERIFY(window.documentTabTitles().contains(QStringLiteral("mod.txt")));

    editor->setDocumentText(QStringLiteral("changed\n"));
    QVERIFY(editor->isModified());
    QVERIFY(window.documentTabTitles().contains(QStringLiteral("mod.txt *")));
}

void SmokeTest::foldCommandsToggleVisibility() {
    EditorWidget editor;
    editor.setLanguage(QStringLiteral("cpp"));
    editor.setDocumentText(QStringLiteral("void f() {\n"
                                          "    int x = 1;\n"
                                          "    int y = 2;\n"
                                          "}\n"));
    editor.editor()->send(SCI_COLOURISE, 0, -1);

    QCOMPARE(editor.editor()->send(SCI_GETLINEVISIBLE, 1), 1);
    editor.foldAll(true);
    QCOMPARE(editor.editor()->send(SCI_GETLINEVISIBLE, 1), 0);
    editor.unfoldAll();
    QCOMPARE(editor.editor()->send(SCI_GETLINEVISIBLE, 1), 1);

    editor.gotoLine(1);
    editor.foldCurrent(true);
    QCOMPARE(editor.editor()->send(SCI_GETLINEVISIBLE, 1), 0);
    editor.toggleCurrentFold();
    QCOMPARE(editor.editor()->send(SCI_GETLINEVISIBLE, 1), 1);
}

void SmokeTest::matchingBraceNavigation() {
    EditorWidget editor;
    editor.setDocumentText(QStringLiteral("foo(bar) baz\n"));

    // Caret on the opening parenthesis jumps to the closing one.
    editor.editor()->send(SCI_GOTOPOS, 3);
    editor.gotoMatchingBrace();
    QCOMPARE(int(editor.editor()->send(SCI_GETCURRENTPOS)), 7);

    // Selecting from the closing parenthesis includes both braces.
    editor.editor()->send(SCI_GOTOPOS, 7);
    editor.selectToMatchingBrace();
    QCOMPARE(int(editor.editor()->send(SCI_GETSELECTIONSTART)), 3);
    QCOMPARE(int(editor.editor()->send(SCI_GETSELECTIONEND)), 8);

    // A caret not next to a brace does nothing.
    editor.editor()->send(SCI_GOTOPOS, 0);
    editor.gotoMatchingBrace();
    QCOMPARE(int(editor.editor()->send(SCI_GETCURRENTPOS)), 0);
}

QTEST_MAIN(SmokeTest)
#include "test_smoke.moc"
