#include "MainWindow.h"

#include "EditorWidget.h"
#include "Encoding.h"
#include "FindInFilesPanel.h"
#include "FindReplaceDialog.h"
#include "LanguageManager.h"
#include "SearchEngine.h"
#include "Theme.h"

#include <ScintillaEdit.h>
#include <Scintilla.h>

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>

namespace PadForge {

namespace {

QString recentPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
        + QStringLiteral("/recent.json");
}

QString pluginsPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
        + QStringLiteral("/plugins");
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("PadForge"));
    resize(1100, 720);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_mainTabs = new QTabWidget(m_splitter);
    m_subTabs = new QTabWidget(m_splitter);
    m_mainTabs->setTabsClosable(true);
    m_mainTabs->setMovable(true);
    m_mainTabs->setDocumentMode(true);
    m_subTabs->setTabsClosable(true);
    m_subTabs->setMovable(true);
    m_subTabs->setDocumentMode(true);
    m_subTabs->hide();
    m_splitter->addWidget(m_mainTabs);
    m_splitter->addWidget(m_subTabs);
    setCentralWidget(m_splitter);
    m_activeTabs = m_mainTabs;

    connect(m_mainTabs, &QTabWidget::currentChanged, this, [this](int) {
        if (m_mainTabs->currentWidget())
            m_activeTabs = m_mainTabs;
        updateWindowTitle();
        updateStatus();
    });
    connect(m_subTabs, &QTabWidget::currentChanged, this, [this](int) {
        if (m_subTabs->currentWidget())
            m_activeTabs = m_subTabs;
        updateWindowTitle();
        updateStatus();
    });
    connect(m_mainTabs, &QTabWidget::tabCloseRequested, this,
            [this](int index) { closeEditor(qobject_cast<EditorWidget*>(m_mainTabs->widget(index))); });
    connect(m_subTabs, &QTabWidget::tabCloseRequested, this,
            [this](int index) { closeEditor(qobject_cast<EditorWidget*>(m_subTabs->widget(index))); });

    m_findDialog = new FindReplaceDialog(this);
    connect(m_findDialog, &FindReplaceDialog::findNextRequested, this, [this]() { findNext(false); });
    connect(m_findDialog, &FindReplaceDialog::findPreviousRequested, this, [this]() { findNext(true); });
    connect(m_findDialog, &FindReplaceDialog::replaceRequested, this, &MainWindow::replaceOne);
    connect(m_findDialog, &FindReplaceDialog::replaceAllRequested, this, &MainWindow::replaceAll);
    connect(m_findDialog, &FindReplaceDialog::countRequested, this, &MainWindow::countMatches);
    connect(m_findDialog, &FindReplaceDialog::markAllRequested, this, &MainWindow::markAll);

    m_pluginHost = new PluginHost(this);

    createActions();
    createMenus();
    createToolbar();
    createStatusBar();
    createDocks();
    createPluginMenu();

    loadMacros();
    setTheme(m_themeId);
    loadPlugins();
    loadSession();

    if (m_editors.isEmpty())
        newDocument();
}

MainWindow::~MainWindow()
{
    delete m_pluginHost;
}

EditorWidget* MainWindow::createEditor()
{
    auto* editor = new EditorWidget();
    return editor;
}

EditorWidget* MainWindow::currentEditor() const
{
    if (!m_activeTabs)
        return nullptr;
    return qobject_cast<EditorWidget*>(m_activeTabs->currentWidget());
}

void MainWindow::addEditor(EditorWidget* editor, QTabWidget* view)
{
    view->addTab(editor, tr("Untitled"));
    view->setCurrentWidget(editor);
    m_activeTabs = view;
    registerEditor(editor);
    updateTabTitle(editor);
}

void MainWindow::registerEditor(EditorWidget* editor)
{
    m_editors.append(editor);
    connect(editor, &EditorWidget::modificationChanged, this, [this, editor](bool) {
        updateTabTitle(editor);
        if (editor == currentEditor())
            updateWindowTitle();
    });
    connect(editor, &EditorWidget::filePathChanged, this, [this, editor](const QString& path) {
        updateTabTitle(editor);
        if (!path.isEmpty())
            addRecent(path);
        if (editor == currentEditor())
            updateWindowTitle();
    });
    connect(editor, &EditorWidget::cursorPositionChanged, this, [this, editor](int, int) {
        if (editor == currentEditor())
            updateStatus();
    });
    connect(editor, &EditorWidget::languageChanged, this, [this, editor](const QString&) {
        if (editor == currentEditor())
            updateStatus();
    });
    connect(editor, &EditorWidget::encodingChanged, this, [this, editor]() {
        if (editor == currentEditor())
            updateStatus();
    });
    connect(editor, &EditorWidget::eolChanged, this, [this, editor]() {
        if (editor == currentEditor())
            updateStatus();
    });
}

EditorWidget* MainWindow::newDocument(const QString& text, const QString& path)
{
    EditorWidget* editor = createEditor();
    if (!text.isEmpty())
        editor->setDocumentText(text);
    if (!path.isEmpty()) {
        editor->setFilePath(path);
        editor->detectLanguage();
    }
    addEditor(editor, m_mainTabs);
    if (path.isEmpty())
        ++m_untitledCounter;
    updateWindowTitle();
    return editor;
}

EditorWidget* MainWindow::openPath(const QString& path, bool switchTo)
{
    const QString clean = QDir::cleanPath(path);
    for (EditorWidget* editor : m_editors) {
        if (!editor->filePath().isEmpty()
            && QDir::cleanPath(editor->filePath()) == clean) {
            if (switchTo) {
                auto* tabs = qobject_cast<QTabWidget*>(editor->parentWidget());
                if (tabs) {
                    tabs->setCurrentWidget(editor);
                    m_activeTabs = tabs;
                }
            }
            return editor;
        }
    }

    EditorWidget* editor = createEditor();
    QString error;
    if (!editor->openFile(clean, &error)) {
        QMessageBox::warning(this, tr("PadForge"), tr("Cannot open %1:\n%2").arg(clean, error));
        editor->deleteLater();
        return nullptr;
    }
    addEditor(editor, m_mainTabs);
    addRecent(clean);
    editor->setFocus();
    updateWindowTitle();
    return editor;
}

void MainWindow::openPaths(const QStringList& paths)
{
    for (const QString& path : paths)
        openPath(path);
}

void MainWindow::closeEditor(EditorWidget* editor)
{
    if (!editor)
        return;
    auto* tabs = qobject_cast<QTabWidget*>(editor->parentWidget());
    if (editor->isModified()) {
        const QString name = editor->filePath().isEmpty() ? tr("Untitled")
                                                          : QFileInfo(editor->filePath()).fileName();
        const auto choice = QMessageBox::question(
            this, tr("Save changes"),
            tr("Do you want to save the changes you made to \"%1\"?").arg(name),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (choice == QMessageBox::Cancel)
            return;
        if (choice == QMessageBox::Save) {
            if (editor->filePath().isEmpty()) {
                const QString path = QFileDialog::getSaveFileName(this, tr("Save As"));
                if (path.isEmpty())
                    return;
                editor->saveFileAs(path);
            } else {
                editor->saveFile();
            }
        }
    }
    if (tabs)
        tabs->removeTab(tabs->indexOf(editor));
    m_editors.removeAll(editor);
    editor->deleteLater();
    updateWindowTitle();
}

void MainWindow::updateTabTitle(EditorWidget* editor)
{
    auto* tabs = qobject_cast<QTabWidget*>(editor->parentWidget());
    if (!tabs)
        return;
    const int index = tabs->indexOf(editor);
    if (index < 0)
        return;
    QString name = editor->filePath().isEmpty() ? tr("Untitled")
                                                : QFileInfo(editor->filePath()).fileName();
    if (editor->isModified())
        name += QStringLiteral(" *");
    tabs->setTabText(index, name);
    tabs->setTabToolTip(index, editor->filePath());
}

void MainWindow::updateWindowTitle()
{
    EditorWidget* editor = currentEditor();
    if (!editor) {
        setWindowTitle(QStringLiteral("PadForge"));
        return;
    }
    QString name = editor->filePath().isEmpty() ? tr("Untitled")
                                               : QFileInfo(editor->filePath()).fileName();
    if (editor->isModified())
        name += QStringLiteral(" *");
    setWindowTitle(QStringLiteral("%1 - PadForge").arg(name));
}

void MainWindow::updateStatus()
{
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    m_statusPosition->setText(tr("Ln %1, Col %2").arg(editor->currentLine()).arg(editor->currentColumn()));
    m_statusEncoding->setText(editor->encoding().displayName());
    m_statusEol->setText(EncodingDetector::eolName(editor->eol()));
    const Language* language = LanguageManager::byId(editor->languageId());
    m_statusLanguage->setText(language ? language->name : QStringLiteral("Normal Text"));
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    for (EditorWidget* editor : QList<EditorWidget*>(m_editors)) {
        auto* tabs = qobject_cast<QTabWidget*>(editor->parentWidget());
        if (editor->isModified()) {
            if (tabs)
                tabs->setCurrentWidget(editor);
            const auto choice = QMessageBox::question(
                this, tr("Save changes"),
                tr("Save changes to \"%1\" before closing?")
                    .arg(editor->filePath().isEmpty() ? tr("Untitled")
                                                      : QFileInfo(editor->filePath()).fileName()),
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            if (choice == QMessageBox::Cancel) {
                event->ignore();
                return;
            }
            if (choice == QMessageBox::Save) {
                if (editor->filePath().isEmpty()) {
                    const QString path = QFileDialog::getSaveFileName(this, tr("Save As"));
                    if (path.isEmpty()) {
                        event->ignore();
                        return;
                    }
                    editor->saveFileAs(path);
                } else {
                    editor->saveFile();
                }
            }
        }
    }
    saveSession();
    saveMacros();
    event->accept();
}

void MainWindow::createActions()
{
    m_actWordWrap = new QAction(tr("Word wrap"), this);
    m_actWordWrap->setCheckable(true);
    connect(m_actWordWrap, &QAction::toggled, this, &MainWindow::setWordWrap);

    m_actSplitView = new QAction(tr("Split view"), this);
    m_actSplitView->setCheckable(true);
    connect(m_actSplitView, &QAction::toggled, this, &MainWindow::toggleSplitView);

    m_actRecordMacro = new QAction(tr("Start recording"), this);
    connect(m_actRecordMacro, &QAction::triggered, this, [this]() {
        if (m_macroRecorder.isRecording())
            stopMacroRecording();
        else
            startMacroRecording();
    });
}

void MainWindow::createMenus()
{
    QMenuBar* bar = menuBar();

    QMenu* fileMenu = bar->addMenu(tr("&File"));
    QAction* aNew = fileMenu->addAction(tr("&New"));
    aNew->setShortcut(QKeySequence::New);
    connect(aNew, &QAction::triggered, this, &MainWindow::newDocumentDialog);
    QAction* aOpen = fileMenu->addAction(tr("&Open..."));
    aOpen->setShortcut(QKeySequence::Open);
    connect(aOpen, &QAction::triggered, this, &MainWindow::openFilesDialog);

    m_recentMenu = fileMenu->addMenu(tr("Open &Recent"));
    m_recentMenu->setObjectName(QStringLiteral("recentMenu"));

    QAction* aSave = fileMenu->addAction(tr("&Save"));
    aSave->setShortcut(QKeySequence::Save);
    connect(aSave, &QAction::triggered, this, &MainWindow::saveCurrent);
    QAction* aSaveAs = fileMenu->addAction(tr("Save &As..."));
    aSaveAs->setShortcut(QKeySequence::SaveAs);
    connect(aSaveAs, &QAction::triggered, this, &MainWindow::saveCurrentAs);
    QAction* aSaveAll = fileMenu->addAction(tr("Save A&ll"));
    connect(aSaveAll, &QAction::triggered, this, &MainWindow::saveAll);
    QAction* aReload = fileMenu->addAction(tr("Re&load from Disk"));
    connect(aReload, &QAction::triggered, this, &MainWindow::reloadCurrent);
    QAction* aClose = fileMenu->addAction(tr("&Close"));
    aClose->setShortcut(QKeySequence::Close);
    connect(aClose, &QAction::triggered, this, [this]() { closeEditor(currentEditor()); });
    fileMenu->addSeparator();

    m_encodingMenu = fileMenu->addMenu(tr("&Encoding"));
    struct EncodingChoice {
        const char* label;
        Charset charset;
        bool bom;
    };
    const EncodingChoice encodings[] = {
        { "UTF-8", Charset::Utf8, false },
        { "UTF-8 with BOM", Charset::Utf8Bom, true },
        { "UTF-16 LE", Charset::Utf16LE, true },
        { "UTF-16 BE", Charset::Utf16BE, true },
        { "UTF-32 LE", Charset::Utf32LE, true },
        { "UTF-32 BE", Charset::Utf32BE, true },
        { "Latin-1", Charset::Latin1, false },
    };
    for (const EncodingChoice& choice : encodings) {
        const Charset charset = choice.charset;
        const bool bom = choice.bom;
        m_encodingMenu->addAction(tr(choice.label), this, [this, charset, bom]() {
            EncodingInfo info;
            info.charset = charset;
            info.bom = bom;
            setEncodingForCurrent(info, false);
        });
    }

    m_eolMenu = fileMenu->addMenu(tr("&Line Endings"));
    m_eolMenu->addAction(tr("Unix (LF)"), this, [this]() { setEolForCurrent(Eol::Lf); });
    m_eolMenu->addAction(tr("Windows (CRLF)"), this, [this]() { setEolForCurrent(Eol::CrLf); });
    m_eolMenu->addAction(tr("Macintosh (CR)"), this, [this]() { setEolForCurrent(Eol::Cr); });

    fileMenu->addSeparator();
    QAction* aQuit = fileMenu->addAction(tr("E&xit"));
    aQuit->setShortcut(QKeySequence::Quit);
    connect(aQuit, &QAction::triggered, this, &QWidget::close);

    QMenu* editMenu = bar->addMenu(tr("&Edit"));
    auto addEditorAction = [this](QMenu* menu, const QString& label, QKeySequence shortcut,
                                  void (ScintillaEdit::*fn)()) {
        QAction* action = menu->addAction(label);
        if (!shortcut.isEmpty())
            action->setShortcut(shortcut);
        connect(action, &QAction::triggered, this, [this, fn]() {
            EditorWidget* editor = currentEditor();
            if (editor)
                (editor->editor()->*fn)();
        });
    };
    addEditorAction(editMenu, tr("&Undo"), QKeySequence::Undo, &ScintillaEdit::undo);
    addEditorAction(editMenu, tr("&Redo"), QKeySequence::Redo, &ScintillaEdit::redo);
    editMenu->addSeparator();
    addEditorAction(editMenu, tr("Cu&t"), QKeySequence::Cut, &ScintillaEdit::cut);
    addEditorAction(editMenu, tr("&Copy"), QKeySequence::Copy, &ScintillaEdit::copy);
    addEditorAction(editMenu, tr("&Paste"), QKeySequence::Paste, &ScintillaEdit::paste);
    addEditorAction(editMenu, tr("Select &All"), QKeySequence::SelectAll, &ScintillaEdit::selectAll);
    editMenu->addSeparator();
    QAction* aFind = editMenu->addAction(tr("&Find..."));
    aFind->setShortcut(QKeySequence::Find);
    connect(aFind, &QAction::triggered, this, [this]() {
        m_findDialog->showForReplaceMode(false);
        m_findDialog->show();
        m_findDialog->raise();
        m_findDialog->activateWindow();
    });
    QAction* aReplace = editMenu->addAction(tr("&Replace..."));
    aReplace->setShortcut(QKeySequence(QStringLiteral("Ctrl+H")));
    connect(aReplace, &QAction::triggered, this, [this]() {
        m_findDialog->showForReplaceMode(true);
        m_findDialog->show();
        m_findDialog->raise();
        m_findDialog->activateWindow();
    });
    QAction* aFindInFiles = editMenu->addAction(tr("Find in &Files..."));
    aFindInFiles->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+F")));
    connect(aFindInFiles, &QAction::triggered, this, [this]() {
        m_findDock->show();
        m_findDock->raise();
        if (currentEditor() && !currentEditor()->filePath().isEmpty())
            m_findInFilesPanel->setDirectory(QFileInfo(currentEditor()->filePath()).absolutePath());
    });
    editMenu->addSeparator();
    QAction* aToggleBookmark = editMenu->addAction(tr("Toggle &Bookmark"));
    aToggleBookmark->setShortcut(QKeySequence(QStringLiteral("Ctrl+F2")));
    connect(aToggleBookmark, &QAction::triggered, this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->toggleBookmark();
    });
    QAction* aNextBookmark = editMenu->addAction(tr("&Next Bookmark"));
    aNextBookmark->setShortcut(QKeySequence(QStringLiteral("F2")));
    connect(aNextBookmark, &QAction::triggered, this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->nextBookmark(true);
    });
    QAction* aPrevBookmark = editMenu->addAction(tr("&Previous Bookmark"));
    aPrevBookmark->setShortcut(QKeySequence(QStringLiteral("Shift+F2")));
    connect(aPrevBookmark, &QAction::triggered, this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->nextBookmark(false);
    });
    QAction* aClearBookmarks = editMenu->addAction(tr("Clear All Bookmarks"));
    connect(aClearBookmarks, &QAction::triggered, this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->clearBookmarks();
    });

    m_macroMenu = editMenu->addMenu(tr("&Macro"));
    m_macroMenu->addAction(m_actRecordMacro);
    QAction* aPlayMacro = m_macroMenu->addAction(tr("Play..."));
    connect(aPlayMacro, &QAction::triggered, this, &MainWindow::playMacro);

    QMenu* viewMenu = bar->addMenu(tr("&View"));
    m_themeMenu = viewMenu->addMenu(tr("&Theme"));
    for (const Theme& theme : ThemeManager::themes()) {
        QAction* action = m_themeMenu->addAction(theme.name);
        action->setCheckable(true);
        action->setData(theme.id);
        action->setChecked(theme.id == m_themeId);
        const QString id = theme.id;
        connect(action, &QAction::triggered, this, [this, id]() { setTheme(id); });
    }
    viewMenu->addSeparator();
    viewMenu->addAction(m_actWordWrap);
    viewMenu->addAction(m_actSplitView);
    QAction* aMoveView = viewMenu->addAction(tr("Move to Other View"));
    connect(aMoveView, &QAction::triggered, this, &MainWindow::moveToOtherView);
    viewMenu->addSeparator();
    QAction* aZoomIn = viewMenu->addAction(tr("Zoom &In"));
    aZoomIn->setShortcut(QKeySequence::ZoomIn);
    connect(aZoomIn, &QAction::triggered, this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->editor()->zoomIn();
    });
    QAction* aZoomOut = viewMenu->addAction(tr("Zoom &Out"));
    aZoomOut->setShortcut(QKeySequence::ZoomOut);
    connect(aZoomOut, &QAction::triggered, this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->editor()->zoomOut();
    });
    QAction* aZoomReset = viewMenu->addAction(tr("&Reset Zoom"));
    connect(aZoomReset, &QAction::triggered, this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->setZoom(0);
    });
    QAction* aWhitespace = viewMenu->addAction(tr("Show All Characters"));
    aWhitespace->setCheckable(true);
    connect(aWhitespace, &QAction::toggled, this, [this](bool on) {
        if (EditorWidget* editor = currentEditor())
            editor->editor()->send(SCI_SETVIEWWS, on ? SCWS_VISIBLEALWAYS : SCWS_INVISIBLE);
    });

    m_languageMenu = bar->addMenu(tr("&Language"));
    QAction* aDetect = m_languageMenu->addAction(tr("Detect from Content"));
    connect(aDetect, &QAction::triggered, this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->detectLanguage();
    });
    m_languageMenu->addSeparator();
    for (const Language& language : LanguageManager::languages()) {
        const QString id = language.id;
        m_languageMenu->addAction(language.name, this, [this, id]() {
            if (EditorWidget* editor = currentEditor())
                editor->setLanguage(id);
        });
    }

    QMenu* helpMenu = bar->addMenu(tr("&Help"));
    QAction* aAbout = helpMenu->addAction(tr("&About PadForge"));
    connect(aAbout, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::createToolbar()
{
    QToolBar* toolbar = addToolBar(tr("Main"));
    toolbar->setObjectName(QStringLiteral("mainToolbar"));
    toolbar->setMovable(false);
    toolbar->addAction(tr("New"), this, &MainWindow::newDocumentDialog);
    toolbar->addAction(tr("Open"), this, &MainWindow::openFilesDialog);
    toolbar->addAction(tr("Save"), this, &MainWindow::saveCurrent);
    toolbar->addSeparator();
    toolbar->addAction(tr("Find"), this, [this]() {
        m_findDialog->showForReplaceMode(false);
        m_findDialog->show();
        m_findDialog->raise();
    });
    toolbar->addAction(tr("Find in Files"), this, [this]() { m_findDock->show(); });
}

void MainWindow::createStatusBar()
{
    m_statusLanguage = new QLabel(this);
    m_statusEncoding = new QLabel(this);
    m_statusEol = new QLabel(this);
    m_statusPosition = new QLabel(this);
    statusBar()->addWidget(m_statusLanguage, 1);
    statusBar()->addPermanentWidget(m_statusEncoding);
    statusBar()->addPermanentWidget(m_statusEol);
    statusBar()->addPermanentWidget(m_statusPosition);
}

void MainWindow::createDocks()
{
    m_findInFilesPanel = new FindInFilesPanel(this);
    m_findDock = new QDockWidget(tr("Find in Files"), this);
    m_findDock->setObjectName(QStringLiteral("findInFilesDock"));
    m_findDock->setWidget(m_findInFilesPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_findDock);
    m_findDock->hide();
    connect(m_findInFilesPanel, &FindInFilesPanel::resultActivated, this, &MainWindow::gotoResult);
}

void MainWindow::createPluginMenu()
{
    m_pluginsMenu = menuBar()->addMenu(tr("&Plugins"));
    QAction* reload = m_pluginsMenu->addAction(tr("Reload Plugins"));
    connect(reload, &QAction::triggered, this, &MainWindow::loadPlugins);
    m_pluginsMenu->addSeparator();
}

void MainWindow::newDocumentDialog()
{
    newDocument();
}

void MainWindow::openFilesDialog()
{
    const QStringList files = QFileDialog::getOpenFileNames(this, tr("Open File"));
    openPaths(files);
}

void MainWindow::saveCurrent()
{
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    if (editor->filePath().isEmpty()) {
        saveCurrentAs();
        return;
    }
    QString error;
    if (!editor->saveFile(&error))
        QMessageBox::warning(this, tr("PadForge"), tr("Cannot save file:\n%1").arg(error));
}

void MainWindow::saveCurrentAs()
{
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    const QString path = QFileDialog::getSaveFileName(this, tr("Save As"), editor->filePath());
    if (path.isEmpty())
        return;
    QString error;
    if (!editor->saveFileAs(path, &error))
        QMessageBox::warning(this, tr("PadForge"), tr("Cannot save file:\n%1").arg(error));
}

void MainWindow::saveAll()
{
    for (EditorWidget* editor : m_editors) {
        if (!editor->isModified())
            continue;
        if (editor->filePath().isEmpty()) {
            auto* tabs = qobject_cast<QTabWidget*>(editor->parentWidget());
            if (tabs)
                tabs->setCurrentWidget(editor);
            const QString path = QFileDialog::getSaveFileName(this, tr("Save As"));
            if (path.isEmpty())
                continue;
            editor->saveFileAs(path);
        } else {
            editor->saveFile();
        }
    }
}

void MainWindow::reloadCurrent()
{
    EditorWidget* editor = currentEditor();
    if (!editor || editor->filePath().isEmpty())
        return;
    const int line = editor->currentLine();
    QString error;
    editor->openFile(editor->filePath(), &error);
    editor->gotoLine(line);
}

void MainWindow::setEncodingForCurrent(const EncodingInfo& encoding, bool reencode)
{
    if (EditorWidget* editor = currentEditor())
        editor->setEncoding(encoding, reencode);
}

void MainWindow::setEolForCurrent(Eol eol)
{
    if (EditorWidget* editor = currentEditor())
        editor->setEol(eol);
}

void MainWindow::showFindDialog()
{
    m_findDialog->show();
}

void MainWindow::findNext(bool up)
{
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    SearchOptions options = m_findDialog->options();
    options.searchUp = up;
    editor->findNextFromCursor(m_findDialog->term(), options);
}

void MainWindow::replaceOne()
{
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    editor->replaceCurrent(m_findDialog->term(), m_findDialog->replacement(), m_findDialog->options());
}

void MainWindow::replaceAll()
{
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    const QString term = m_findDialog->term();
    if (term.isEmpty())
        return;
    int count = 0;
    const QString replaced = SearchEngine::replaceAll(editor->documentText(), term,
                                                      m_findDialog->replacement(),
                                                      m_findDialog->options(), &count);
    editor->setDocumentText(replaced);
    showMessage(tr("Replaced %1 occurrence(s).").arg(count));
}

void MainWindow::countMatches()
{
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    const int count = SearchEngine::count(editor->documentText(), m_findDialog->term(),
                                          m_findDialog->options());
    showMessage(tr("%1 match(es) found.").arg(count));
}

void MainWindow::markAll()
{
    if (EditorWidget* editor = currentEditor())
        editor->highlightMatches(m_findDialog->term(), m_findDialog->options());
}

void MainWindow::gotoResult(const QString& file, int line)
{
    EditorWidget* editor = openPath(file);
    if (editor)
        editor->gotoLine(line);
}

void MainWindow::toggleSplitView(bool enabled)
{
    m_subTabs->setVisible(enabled);
    if (!enabled) {
        while (m_subTabs->count() > 0) {
            auto* editor = qobject_cast<EditorWidget*>(m_subTabs->widget(0));
            if (!editor)
                break;
            m_subTabs->removeTab(0);
            m_mainTabs->addTab(editor, tr("Untitled"));
            m_mainTabs->setCurrentWidget(editor);
            updateTabTitle(editor);
        }
        m_activeTabs = m_mainTabs;
    } else if (m_subTabs->count() == 0) {
        m_subTabs->addTab(new EditorWidget(), tr("Untitled"));
    }
    m_actSplitView->setChecked(enabled);
}

void MainWindow::moveToOtherView()
{
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    auto* from = qobject_cast<QTabWidget*>(editor->parentWidget());
    if (!from)
        return;
    QTabWidget* to = (from == m_mainTabs) ? m_subTabs : m_mainTabs;
    if (!m_actSplitView->isChecked())
        toggleSplitView(true);
    from->removeTab(from->indexOf(editor));
    to->addTab(editor, tr("Untitled"));
    to->setCurrentWidget(editor);
    m_activeTabs = to;
    updateTabTitle(editor);
}

void MainWindow::setTheme(const QString& id)
{
    m_themeId = id;
    ThemeManager::setCurrent(id);
    const Theme& theme = ThemeManager::byId(id);

    QPalette palette = qApp->palette();
    palette.setColor(QPalette::Window, theme.background);
    palette.setColor(QPalette::Base, theme.background);
    palette.setColor(QPalette::Text, theme.foreground);
    palette.setColor(QPalette::WindowText, theme.foreground);
    palette.setColor(QPalette::ButtonText, theme.foreground);
    palette.setColor(QPalette::ToolTipBase, theme.background);
    palette.setColor(QPalette::ToolTipText, theme.foreground);
    qApp->setPalette(palette);
    qApp->setStyleSheet(theme.styleSheet());

    if (m_themeMenu) {
        for (QAction* action : m_themeMenu->actions())
            action->setChecked(action->data().toString() == id);
    }
    applyThemeToAll();
}

void MainWindow::applyThemeToAll()
{
    const Theme& theme = ThemeManager::current();
    for (EditorWidget* editor : m_editors)
        editor->applyTheme(theme);
}

void MainWindow::setWordWrap(bool enabled)
{
    for (EditorWidget* editor : m_editors)
        editor->editor()->send(SCI_SETWRAPMODE, enabled ? SC_WRAP_WORD : SC_WRAP_NONE);
}

void MainWindow::loadSession()
{
    bool ok = false;
    const Session session = SessionManager::load(SessionManager::defaultPath(), &ok);
    if (!ok)
        return;

    for (const SessionFile& file : session.files) {
        EditorWidget* editor = nullptr;
        if (file.untitled)
            editor = newDocument(file.untitledText);
        else
            editor = openPath(file.path, false);
        if (!editor)
            continue;
        if (!file.language.isEmpty())
            editor->setLanguage(file.language);
        editor->applySavedState(file.cursor, file.anchor, file.firstVisibleLine, file.bookmarkLines);
    }

    if (session.splitView && !session.subViewFiles.isEmpty()) {
        toggleSplitView(true);
        m_subTabs->clear();
        for (const SessionFile& file : session.subViewFiles) {
            EditorWidget* editor = file.untitled ? newDocument(file.untitledText)
                                                 : openPath(file.path, false);
            if (!editor)
                continue;
            if (!file.language.isEmpty())
                editor->setLanguage(file.language);
            editor->applySavedState(file.cursor, file.anchor, file.firstVisibleLine,
                                    file.bookmarkLines);
        }
    }

    m_recent.setList(session.recentFiles);
    updateRecentMenu();
}

void MainWindow::saveSession()
{
    Session session;
    auto capture = [](EditorWidget* editor) {
        SessionFile file;
        file.path = editor->filePath();
        file.untitled = editor->filePath().isEmpty();
        file.untitledText = file.untitled ? editor->documentText() : QString();
        file.cursor = editor->cursorPosition();
        file.anchor = editor->cursorPosition();
        file.firstVisibleLine = static_cast<int>(editor->editor()->firstVisibleLine());
        file.language = editor->languageId();
        file.encoding = editor->encoding().canonicalName();
        file.eol = EncodingDetector::eolName(editor->eol());
        file.bookmarkLines = editor->bookmarkLines();
        return file;
    };

    for (int i = 0; i < m_mainTabs->count(); ++i) {
        auto* editor = qobject_cast<EditorWidget*>(m_mainTabs->widget(i));
        if (!editor)
            continue;
        if (editor == currentEditor())
            session.activeIndex = session.files.size();
        session.files.append(capture(editor));
    }
    if (!m_subTabs->isHidden()) {
        session.splitView = true;
        for (int i = 0; i < m_subTabs->count(); ++i) {
            auto* editor = qobject_cast<EditorWidget*>(m_subTabs->widget(i));
            if (editor && editor == currentEditor())
                session.activeView = 1;
            if (editor)
                session.subViewFiles.append(capture(editor));
        }
    }
    session.recentFiles = m_recent.list();
    SessionManager::save(session, SessionManager::defaultPath());
}

void MainWindow::updateRecentMenu()
{
    if (!m_recentMenu)
        return;
    m_recentMenu->clear();
    const QStringList files = m_recent.list();
    if (files.isEmpty()) {
        QAction* empty = m_recentMenu->addAction(tr("(no recent files)"));
        empty->setEnabled(false);
        return;
    }
    for (const QString& path : files) {
        m_recentMenu->addAction(path, this, [this, path]() { openPath(path); });
    }
    m_recentMenu->addSeparator();
    m_recentMenu->addAction(tr("Clear Recent Files"), this, [this]() {
        m_recent.clear();
        m_recent.save(recentPath());
        updateRecentMenu();
    });
}

void MainWindow::addRecent(const QString& path)
{
    if (path.isEmpty())
        return;
    m_recent.add(path);
    m_recent.save(recentPath());
    updateRecentMenu();
}

void MainWindow::loadMacros()
{
    bool ok = false;
    m_macros = MacroLibrary::load(MacroLibrary::defaultPath(), &ok);
    if (!ok)
        m_macros.clear();
}

void MainWindow::saveMacros()
{
    MacroLibrary::save(m_macros, MacroLibrary::defaultPath());
}

void MainWindow::loadPlugins()
{
    const QString dir = pluginsPath();
    QDir().mkpath(dir);
    m_pluginHost->setPluginDirectory(dir);
    m_pluginHost->loadLuaPluginsIn(dir);

    const QString nativeDir = QCoreApplication::applicationDirPath() + QStringLiteral("/plugins");
    QDir native(nativeDir);
    for (const QString& entry : native.entryList({ QStringLiteral("*.so"), QStringLiteral("*.so.*") },
                                                 QDir::Files)) {
        m_pluginHost->loadNativePlugin(native.absoluteFilePath(entry));
    }
    showMessage(tr("Loaded %1 plugin(s).").arg(m_pluginHost->loadedPlugins().size()));
}

void MainWindow::startMacroRecording()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Record Macro"), tr("Macro name:"),
                                               QLineEdit::Normal,
                                               tr("Macro %1").arg(m_macros.size() + 1), &ok);
    if (!ok || name.isEmpty())
        return;
    m_macroRecorder.start(name);
    m_actRecordMacro->setText(tr("Stop recording"));
    showMessage(tr("Recording macro \"%1\"...").arg(name));
}

void MainWindow::stopMacroRecording()
{
    Macro macro = m_macroRecorder.stop();
    m_actRecordMacro->setText(tr("Start recording"));
    if (macro.isEmpty()) {
        showMessage(tr("Macro recorded no steps."));
        return;
    }
    m_macros.append(macro);
    saveMacros();
    showMessage(tr("Saved macro \"%1\".").arg(macro.name));
}

void MainWindow::playMacro()
{
    if (m_macros.isEmpty()) {
        showMessage(tr("No macros recorded."));
        return;
    }
    QStringList names;
    for (const Macro& macro : m_macros)
        names << macro.name;
    bool ok = false;
    const QString chosen = QInputDialog::getItem(this, tr("Play Macro"), tr("Macro:"), names, 0,
                                                 false, &ok);
    if (!ok)
        return;
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    for (const Macro& macro : m_macros) {
        if (macro.name != chosen)
            continue;
        for (const MacroStep& step : macro.steps) {
            if (step.kind == MacroStep::Kind::InsertText) {
                const QByteArray utf8 = step.text.toUtf8();
                editor->editor()->send(SCI_REPLACESEL, 0,
                                       reinterpret_cast<sptr_t>(utf8.constData()));
            }
        }
    }
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, tr("About PadForge"),
                       tr("<h3>PadForge</h3>"
                          "<p>A fast, native Linux text editor inspired by the Notepad++ "
                          "workflow. Built with Qt 6, Scintilla and Lexilla.</p>"
                          "<p>PadForge is free software released under the GNU GPL v3. "
                          "Notepad++ is an independent project and is not affiliated with "
                          "PadForge.</p>"));
}

void MainWindow::insertText(const QString& text)
{
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    const QByteArray utf8 = text.toUtf8();
    editor->editor()->send(SCI_REPLACESEL, 0, reinterpret_cast<sptr_t>(utf8.constData()));
}

void MainWindow::setDocumentText(const QString& text)
{
    if (EditorWidget* editor = currentEditor())
        editor->setDocumentText(text);
}

QString MainWindow::documentText() const
{
    EditorWidget* editor = currentEditor();
    return editor ? editor->documentText() : QString();
}

int MainWindow::lineCount() const
{
    EditorWidget* editor = currentEditor();
    return editor ? static_cast<int>(editor->editor()->lineCount()) : 0;
}

int MainWindow::currentLine() const
{
    EditorWidget* editor = currentEditor();
    return editor ? editor->currentLine() : 0;
}

void MainWindow::gotoLine(int oneBasedLine)
{
    if (EditorWidget* editor = currentEditor())
        editor->gotoLine(oneBasedLine);
}

void MainWindow::setLanguage(const QString& lexerId)
{
    if (EditorWidget* editor = currentEditor())
        editor->setLanguage(lexerId);
}

void MainWindow::showMessage(const QString& message)
{
    statusBar()->showMessage(message, 8000);
}

void MainWindow::addMenuItem(const QString& name, std::function<void()> callback)
{
    if (!m_pluginsMenu)
        return;
    QAction* action = m_pluginsMenu->addAction(name);
    connect(action, &QAction::triggered, this, [callback]() { callback(); });
    ++m_pluginMenuCount;
}

} // namespace PadForge
