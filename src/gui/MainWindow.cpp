#include "MainWindow.h"

#include "Backup.h"
#include "CharacterMapPanel.h"
#include "ColumnEditorDialog.h"
#include "CompareDocumentsDialog.h"
#include "ConsolePanel.h"
#include "Diff.h"
#include "DocumentMap.h"
#include "DocumentSummaryDialog.h"
#include "EditorWidget.h"
#include "Encoding.h"
#include "ExplorerPanel.h"
#include "FindInFilesPanel.h"
#include "FindReplaceDialog.h"
#include "FloatingWindow.h"
#include "FunctionListPanel.h"
#include "GotoLineDialog.h"
#include "LanguageManager.h"
#include "PreferencesDialog.h"
#include "RunDialog.h"
#include "SearchEngine.h"
#include "SearchResultsPanel.h"
#include "TextOps.h"
#include "Theme.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPair>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QSettings>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QTextDocument>
#include <QToolBar>
#include <QUrl>
#include <Scintilla.h>
#include <ScintillaEdit.h>
#include <algorithm>

namespace PadForge {

namespace {

QString recentPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) +
           QStringLiteral("/recent.json");
}

QString pluginsPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) +
           QStringLiteral("/plugins");
}

} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    m_settings.load();
    m_themeId = m_settings.theme;

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
        updateWindowMenu();
        updateLanguageMenu();
        updateDocumentMapSource();
        updateFunctionListSource();
    });
    connect(m_subTabs, &QTabWidget::currentChanged, this, [this](int) {
        if (m_subTabs->currentWidget())
            m_activeTabs = m_subTabs;
        updateWindowTitle();
        updateStatus();
        updateWindowMenu();
        updateLanguageMenu();
        updateDocumentMapSource();
        updateFunctionListSource();
    });
    connect(m_mainTabs, &QTabWidget::tabCloseRequested, this, [this](int index) {
        closeEditor(qobject_cast<EditorWidget*>(m_mainTabs->widget(index)));
    });
    connect(m_subTabs, &QTabWidget::tabCloseRequested, this, [this](int index) {
        closeEditor(qobject_cast<EditorWidget*>(m_subTabs->widget(index)));
    });

    for (QTabWidget* tabs : {m_mainTabs, m_subTabs}) {
        tabs->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
        tabs->tabBar()->installEventFilter(this);
        connect(tabs->tabBar(), &QTabBar::customContextMenuRequested, this,
                [this, tabs](const QPoint& pos) {
                    const int index = tabs->tabBar()->tabAt(pos);
                    if (index >= 0)
                        m_activeTabs = tabs;
                    showTabContextMenu(tabs, index, tabs->tabBar()->mapToGlobal(pos));
                });
    }

    m_findDialog = new FindReplaceDialog(this);
    connect(m_findDialog, &FindReplaceDialog::findNextRequested, this,
            [this]() { findNext(false); });
    connect(m_findDialog, &FindReplaceDialog::findPreviousRequested, this,
            [this]() { findNext(true); });
    connect(m_findDialog, &FindReplaceDialog::replaceRequested, this, &MainWindow::replaceOne);
    connect(m_findDialog, &FindReplaceDialog::replaceAllRequested, this, &MainWindow::replaceAll);
    connect(m_findDialog, &FindReplaceDialog::countRequested, this, &MainWindow::countMatches);
    connect(m_findDialog, &FindReplaceDialog::markAllRequested, this, &MainWindow::markAll);
    connect(m_findDialog, &FindReplaceDialog::findAllInCurrentRequested, this,
            [this]() { findAllInOpen(false); });
    connect(m_findDialog, &FindReplaceDialog::findAllInOpenRequested, this,
            [this]() { findAllInOpen(true); });

    m_pluginHost = new PluginHost(this);

    createActions();
    createDocks();
    createMenus();
    createToolbar();
    createStatusBar();
    createPluginMenu();

    m_syncScroll = m_settings.syncScroll;

    m_backupTimer.setInterval(20 * 1000);
    connect(&m_backupTimer, &QTimer::timeout, this, &MainWindow::snapshotModifiedDocuments);

    loadMacros();
    setTheme(m_themeId);
    loadPlugins();
    loadSession();

    QSettings crashed("PadForge", "PadForge");
    const bool cleanShutdown = crashed.value(QStringLiteral("backup/cleanShutdown"), true).toBool();
    crashed.setValue(QStringLiteral("backup/cleanShutdown"), false);
    if (m_settings.backupEnabled) {
        m_backupTimer.start();
        if (!cleanShutdown)
            offerRestoreBackups();
    }

    if (m_editors.isEmpty())
        newDocument();
}

MainWindow::~MainWindow() {
    delete m_pluginHost;
}

EditorWidget* MainWindow::createEditor() {
    auto* editor = new EditorWidget();
    editor->applySettings(m_settings);
    return editor;
}

EditorWidget* MainWindow::currentEditor() const {
    if (!m_activeTabs)
        return nullptr;
    return qobject_cast<EditorWidget*>(m_activeTabs->currentWidget());
}

QTabWidget* MainWindow::tabWidgetFor(EditorWidget* editor) const {
    if (!editor)
        return nullptr;
    // A tab page's parentWidget() is the QTabWidget's internal QStackedWidget,
    // not the QTabWidget itself, so resolve the owner by index instead.
    if (m_mainTabs && m_mainTabs->indexOf(editor) >= 0)
        return m_mainTabs;
    if (m_subTabs && m_subTabs->indexOf(editor) >= 0)
        return m_subTabs;
    return nullptr;
}

void MainWindow::addEditor(EditorWidget* editor, QTabWidget* view) {
    view->addTab(editor, tr("Untitled"));
    view->setCurrentWidget(editor);
    m_activeTabs = view;
    registerEditor(editor);
    updateTabTitle(editor);
    updateWindowMenu();
}

void MainWindow::registerEditor(EditorWidget* editor) {
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
        if (m_explorerPanel && m_explorerPanel->isFollowing())
            m_explorerPanel->revealPath(path);
    });
    connect(editor, &EditorWidget::cursorPositionChanged, this, [this, editor](int, int) {
        if (editor == currentEditor())
            updateStatus();
    });
    connect(editor, &EditorWidget::scrolled, this, [this, editor]() { syncScrollFrom(editor); });
    connect(editor, &EditorWidget::languageChanged, this, [this, editor](const QString&) {
        if (editor == currentEditor())
            updateStatus();
        updateLanguageMenu();
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

EditorWidget* MainWindow::newDocument(const QString& text, const QString& path) {
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

EditorWidget* MainWindow::openPath(const QString& path, bool switchTo) {
    const QString clean = QDir::cleanPath(path);
    for (EditorWidget* editor : m_editors) {
        if (!editor->filePath().isEmpty() && QDir::cleanPath(editor->filePath()) == clean) {
            if (switchTo) {
                auto* tabs = tabWidgetFor(editor);
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

void MainWindow::openPaths(const QStringList& paths) {
    for (const QString& path : paths)
        openPath(path);
}

void MainWindow::closeEditor(EditorWidget* editor) {
    if (!editor)
        return;
    if (editor == m_compareA || editor == m_compareB)
        clearCompare();
    auto* tabs = tabWidgetFor(editor);
    if (editor->isModified()) {
        const QString name = editor->filePath().isEmpty()
                                 ? tr("Untitled")
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
    updateWindowMenu();
    updateWindowTitle();
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseButtonRelease) {
        auto* mouse = static_cast<QMouseEvent*>(event);
        if (mouse->button() == Qt::MiddleButton) {
            for (QTabWidget* tabs : {m_mainTabs, m_subTabs}) {
                if (watched != tabs->tabBar())
                    continue;
                const int index = tabs->tabBar()->tabAt(mouse->position().toPoint());
                if (index >= 0) {
                    closeEditor(qobject_cast<EditorWidget*>(tabs->widget(index)));
                    return true;
                }
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

QStringList MainWindow::documentTabTitles() const {
    QStringList titles;
    if (!m_activeTabs)
        return titles;
    for (int i = 0; i < m_activeTabs->count(); ++i)
        titles.append(m_activeTabs->tabText(i));
    return titles;
}

void MainWindow::showTabContextMenu(QTabWidget* tabs, int index, const QPoint& globalPos) {
    QMenu menu(this);
    auto* editor = index >= 0 ? qobject_cast<EditorWidget*>(tabs->widget(index)) : nullptr;
    if (editor) {
        menu.addAction(tr("Close"), this, [this, editor]() { closeEditor(editor); });
        menu.addAction(tr("Close All But This"), this, [this, editor]() {
            m_activeTabs = tabWidgetFor(editor);
            if (m_activeTabs)
                m_activeTabs->setCurrentWidget(editor);
            closeOtherDocuments();
        });
        menu.addAction(tr("Close All to the Left"), this,
                       [this, tabs, index]() { closeDocumentRange(tabs, 0, index - 1); });
        menu.addAction(tr("Close All to the Right"), this, [this, tabs, index]() {
            closeDocumentRange(tabs, index + 1, tabs->count() - 1);
        });
        menu.addSeparator();
        menu.addAction(tr("Save"), this, [this, editor]() { saveEditor(editor); });
        menu.addAction(tr("Save As..."), this, [this, editor]() { saveEditorAs(editor); });
        if (!editor->filePath().isEmpty())
            menu.addAction(tr("Reload from Disk"), this,
                           [this, editor]() { reloadEditor(editor); });
        menu.addSeparator();
        menu.addAction(tr("Rename..."), this, [this, editor]() { renameEditor(editor); });
        if (!editor->filePath().isEmpty()) {
            menu.addAction(tr("Copy Full Path"), this,
                           [this, editor]() { copyEditorPath(editor); });
            menu.addAction(tr("Open Containing Folder"), this,
                           [this, editor]() { openContainingFolder(editor); });
        }
        menu.addSeparator();
        menu.addAction(tr("Move to Other View"), this,
                       [this, editor]() { moveEditorToOtherView(editor); });
        menu.addSeparator();
    }
    menu.addAction(tr("Close All"), this, &MainWindow::closeAllDocuments);
    menu.exec(globalPos);
}

void MainWindow::saveEditor(EditorWidget* editor) {
    if (!editor)
        return;
    if (editor->filePath().isEmpty()) {
        saveEditorAs(editor);
        return;
    }
    QString error;
    if (!editor->saveFile(&error))
        QMessageBox::warning(this, tr("PadForge"), tr("Cannot save file:\n%1").arg(error));
}

void MainWindow::saveEditorAs(EditorWidget* editor) {
    if (!editor)
        return;
    const QString path = QFileDialog::getSaveFileName(this, tr("Save As"), editor->filePath());
    if (path.isEmpty())
        return;
    QString error;
    if (!editor->saveFileAs(path, &error))
        QMessageBox::warning(this, tr("PadForge"), tr("Cannot save file:\n%1").arg(error));
}

void MainWindow::reloadEditor(EditorWidget* editor) {
    if (!editor || editor->filePath().isEmpty())
        return;
    if (editor->isModified() &&
        QMessageBox::question(this, tr("Reload from Disk"),
                              tr("Discard unsaved changes to \"%1\" and reload from disk?")
                                  .arg(QFileInfo(editor->filePath()).fileName()),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) != QMessageBox::Yes)
        return;
    const int line = editor->currentLine();
    QString error;
    if (!editor->openFile(editor->filePath(), &error))
        QMessageBox::warning(this, tr("PadForge"), tr("Cannot reload file:\n%1").arg(error));
    else
        editor->gotoLine(line);
}

void MainWindow::renameEditor(EditorWidget* editor, const QString& newPath) {
    if (!editor)
        return;
    QString target = newPath;
    if (target.isEmpty()) {
        const QString initial =
            editor->filePath().isEmpty() ? QString() : QFileInfo(editor->filePath()).fileName();
        target = QInputDialog::getText(this, tr("Rename"), tr("New file name:"), QLineEdit::Normal,
                                       initial);
        if (target.isEmpty())
            return;
        if (!editor->filePath().isEmpty() && QFileInfo(target).isRelative())
            target = QFileInfo(editor->filePath()).absoluteDir().filePath(target);
    }
    if (editor->filePath().isEmpty()) {
        // An untitled document has nothing to rename on disk; save it instead.
        QString error;
        if (!editor->saveFileAs(target, &error))
            QMessageBox::warning(this, tr("PadForge"), tr("Cannot save file:\n%1").arg(error));
        return;
    }
    if (QDir::cleanPath(target) == QDir::cleanPath(editor->filePath()))
        return;
    if (QFileInfo::exists(target)) {
        QMessageBox::warning(this, tr("Rename"),
                             tr("A file named \"%1\" already exists.").arg(target));
        return;
    }
    const QString oldPath = editor->filePath();
    if (!QFile::rename(oldPath, target)) {
        QMessageBox::warning(this, tr("Rename"), tr("Cannot rename the file to:\n%1").arg(target));
        return;
    }
    editor->setFilePath(target);
    updateTabTitle(editor);
    addRecent(target);
    if (editor == currentEditor())
        updateWindowTitle();
    showMessage(tr("Renamed to \"%1\".").arg(QFileInfo(target).fileName()));
}

void MainWindow::openContainingFolder(EditorWidget* editor) {
    if (!editor || editor->filePath().isEmpty())
        return;
    const QString dir = QFileInfo(editor->filePath()).absolutePath();
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(dir)))
        QMessageBox::warning(this, tr("PadForge"),
                             tr("Cannot open the containing folder:\n%1").arg(dir));
}

void MainWindow::copyEditorPath(EditorWidget* editor) {
    if (!editor || editor->filePath().isEmpty())
        return;
    QApplication::clipboard()->setText(editor->filePath());
    showMessage(tr("Full path copied to clipboard."));
}

void MainWindow::closeDocumentRange(QTabWidget* tabs, int first, int last) {
    if (!tabs)
        return;
    QList<EditorWidget*> doomed;
    for (int i = first; i <= last; ++i) {
        if (i < 0 || i >= tabs->count())
            continue;
        if (auto* editor = qobject_cast<EditorWidget*>(tabs->widget(i)))
            doomed.append(editor);
    }
    for (EditorWidget* editor : doomed) {
        if (tabWidgetFor(editor) != tabs)
            continue;
        const int before = m_editors.size();
        closeEditor(editor);
        if (m_editors.size() == before)
            break; // the user cancelled
    }
}

void MainWindow::closeOtherDocuments() {
    EditorWidget* keep = currentEditor();
    if (!keep)
        return;
    for (EditorWidget* editor : QList<EditorWidget*>(m_editors)) {
        if (editor == keep)
            continue;
        const int before = m_editors.size();
        closeEditor(editor);
        if (m_editors.size() == before)
            break; // the user cancelled
    }
}

void MainWindow::closeAllDocuments() {
    for (EditorWidget* editor : QList<EditorWidget*>(m_editors)) {
        const int before = m_editors.size();
        closeEditor(editor);
        if (m_editors.size() == before)
            break; // the user cancelled
    }
}

void MainWindow::renameCurrentDocument(const QString& newPath) {
    renameEditor(currentEditor(), newPath);
}

void MainWindow::showCurrentInFileManager() {
    openContainingFolder(currentEditor());
}

void MainWindow::sortTabs(bool byPath) {
    QTabWidget* tabs = m_activeTabs;
    if (!tabs || tabs->count() < 2)
        return;
    EditorWidget* current = currentEditor();
    QList<QPair<QString, EditorWidget*>> ordered;
    for (int i = 0; i < tabs->count(); ++i) {
        auto* editor = qobject_cast<EditorWidget*>(tabs->widget(i));
        if (!editor)
            continue;
        const QString key = byPath ? editor->filePath() : tabs->tabText(i);
        ordered.append({key.toLower(), editor});
    }
    std::stable_sort(ordered.begin(), ordered.end(),
                     [](const QPair<QString, EditorWidget*>& a,
                        const QPair<QString, EditorWidget*>& b) { return a.first < b.first; });
    for (int target = 0; target < ordered.size(); ++target) {
        const int from = tabs->indexOf(ordered.at(target).second);
        if (from != target)
            tabs->tabBar()->moveTab(from, target);
    }
    if (current)
        tabs->setCurrentWidget(current);
}

void MainWindow::updateTabTitle(EditorWidget* editor) {
    auto* tabs = tabWidgetFor(editor);
    if (!tabs)
        return;
    const int index = tabs->indexOf(editor);
    if (index < 0)
        return;
    QString name =
        editor->filePath().isEmpty() ? tr("Untitled") : QFileInfo(editor->filePath()).fileName();
    if (editor->isModified())
        name += QStringLiteral(" *");
    tabs->setTabText(index, name);
    tabs->setTabToolTip(index, editor->filePath());
}

void MainWindow::updateWindowTitle() {
    EditorWidget* editor = currentEditor();
    if (!editor) {
        setWindowTitle(QStringLiteral("PadForge"));
        return;
    }
    QString name =
        editor->filePath().isEmpty() ? tr("Untitled") : QFileInfo(editor->filePath()).fileName();
    if (editor->isModified())
        name += QStringLiteral(" *");
    setWindowTitle(QStringLiteral("%1 - PadForge").arg(name));
}

void MainWindow::updateStatus() {
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    m_statusPosition->setText(
        tr("Ln %1, Col %2").arg(editor->currentLine()).arg(editor->currentColumn()));
    m_statusEncoding->setText(editor->encoding().displayName());
    m_statusEol->setText(EncodingDetector::eolName(editor->eol()));
    const Language* language = LanguageManager::byId(editor->languageId());
    m_statusLanguage->setText(language ? language->name : QStringLiteral("Normal Text"));
    if (m_statusReadOnly)
        m_statusReadOnly->setText(editor->isReadOnly() ? tr("R/O") : QString());
    if (m_actReadOnly)
        m_actReadOnly->setChecked(editor->isReadOnly());
}

void MainWindow::closeEvent(QCloseEvent* event) {
    for (FloatingWindow* window : QList<FloatingWindow*>(m_floatingWindows))
        window->close();
    for (EditorWidget* editor : QList<EditorWidget*>(m_editors)) {
        auto* tabs = tabWidgetFor(editor);
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
    m_settings.save();
    if (m_settings.backupEnabled)
        BackupManager::clear();
    QSettings("PadForge", "PadForge").setValue(QStringLiteral("backup/cleanShutdown"), true);
    event->accept();
}

void MainWindow::createActions() {
    m_actWordWrap = new QAction(tr("Word wrap"), this);
    m_actWordWrap->setCheckable(true);
    m_actWordWrap->setChecked(m_settings.wordWrap);
    connect(m_actWordWrap, &QAction::toggled, this, [this](bool on) {
        m_settings.wordWrap = on;
        applySettingsToAll();
        m_settings.save();
    });

    m_actSplitView = new QAction(tr("Split view"), this);
    m_actSplitView->setCheckable(true);
    connect(m_actSplitView, &QAction::toggled, this, &MainWindow::toggleSplitView);

    m_actShowLineNumbers = new QAction(tr("Line numbers"), this);
    m_actShowLineNumbers->setCheckable(true);
    m_actShowLineNumbers->setChecked(m_settings.showLineNumbers);
    connect(m_actShowLineNumbers, &QAction::toggled, this, [this](bool on) {
        m_settings.showLineNumbers = on;
        applySettingsToAll();
        m_settings.save();
    });

    m_actShowFoldMargin = new QAction(tr("Folding margin"), this);
    m_actShowFoldMargin->setCheckable(true);
    m_actShowFoldMargin->setChecked(m_settings.showFoldMargin);
    connect(m_actShowFoldMargin, &QAction::toggled, this, [this](bool on) {
        m_settings.showFoldMargin = on;
        applySettingsToAll();
        m_settings.save();
    });

    m_actShowBookmarkMargin = new QAction(tr("Bookmark margin"), this);
    m_actShowBookmarkMargin->setCheckable(true);
    m_actShowBookmarkMargin->setChecked(m_settings.showBookmarkMargin);
    connect(m_actShowBookmarkMargin, &QAction::toggled, this, [this](bool on) {
        m_settings.showBookmarkMargin = on;
        applySettingsToAll();
        m_settings.save();
    });

    m_actShowAllChars = new QAction(tr("Show all characters"), this);
    m_actShowAllChars->setCheckable(true);
    m_actShowAllChars->setChecked(m_settings.showWhitespace);
    connect(m_actShowAllChars, &QAction::toggled, this, [this](bool on) {
        m_settings.showWhitespace = on;
        applySettingsToAll();
        m_settings.save();
    });

    m_actShowEol = new QAction(tr("Show line endings"), this);
    m_actShowEol->setCheckable(true);
    m_actShowEol->setChecked(m_settings.showEol);
    connect(m_actShowEol, &QAction::toggled, this, [this](bool on) {
        m_settings.showEol = on;
        applySettingsToAll();
        m_settings.save();
    });

    m_actShowIndentGuides = new QAction(tr("Indentation guides"), this);
    m_actShowIndentGuides->setCheckable(true);
    m_actShowIndentGuides->setChecked(m_settings.showIndentGuides);
    connect(m_actShowIndentGuides, &QAction::toggled, this, [this](bool on) {
        m_settings.showIndentGuides = on;
        applySettingsToAll();
        m_settings.save();
    });

    m_actHighlightLine = new QAction(tr("Highlight current line"), this);
    m_actHighlightLine->setCheckable(true);
    m_actHighlightLine->setChecked(m_settings.highlightCurrentLine);
    connect(m_actHighlightLine, &QAction::toggled, this, [this](bool on) {
        m_settings.highlightCurrentLine = on;
        applySettingsToAll();
        m_settings.save();
    });

    m_actAlwaysOnTop = new QAction(tr("Always on top"), this);
    m_actAlwaysOnTop->setCheckable(true);
    m_actAlwaysOnTop->setChecked(m_settings.alwaysOnTop);
    connect(m_actAlwaysOnTop, &QAction::toggled, this, &MainWindow::toggleAlwaysOnTop);

    m_actSplitHorizontal = new QAction(tr("Split horizontally"), this);
    m_actSplitHorizontal->setCheckable(true);
    m_actSplitHorizontal->setChecked(m_settings.splitOrientation == 0);
    connect(m_actSplitHorizontal, &QAction::toggled, this, [this](bool on) {
        if (on)
            setSplitOrientation(0);
    });

    m_actSplitVertical = new QAction(tr("Split vertically"), this);
    m_actSplitVertical->setCheckable(true);
    m_actSplitVertical->setChecked(m_settings.splitOrientation == 1);
    connect(m_actSplitVertical, &QAction::toggled, this, [this](bool on) {
        if (on)
            setSplitOrientation(1);
    });

    m_actRecordMacro = new QAction(tr("Start recording"), this);
    connect(m_actRecordMacro, &QAction::triggered, this, [this]() {
        if (m_macroRecorder.isRecording())
            stopMacroRecording();
        else
            startMacroRecording();
    });
}

void MainWindow::createMenus() {
    QMenuBar* bar = menuBar();

    QMenu* fileMenu = bar->addMenu(tr("&File"));
    QAction* aNew = fileMenu->addAction(tr("&New"));
    aNew->setShortcut(QKeySequence::New);
    connect(aNew, &QAction::triggered, this, &MainWindow::newDocumentDialog);
    QAction* aOpen = fileMenu->addAction(tr("&Open..."));
    aOpen->setShortcut(QKeySequence::Open);
    connect(aOpen, &QAction::triggered, this, &MainWindow::openFilesDialog);
    QAction* aOpenFolder = fileMenu->addAction(tr("Open &Folder..."));
    connect(aOpenFolder, &QAction::triggered, this, &MainWindow::openFolderDialog);

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
    QAction* aSaveCopy = fileMenu->addAction(tr("Save a Copy A&s..."));
    aSaveCopy->setShortcut(QKeySequence(QStringLiteral("Ctrl+Alt+S")));
    connect(aSaveCopy, &QAction::triggered, this, &MainWindow::saveCurrentCopyAs);
    QAction* aReload = fileMenu->addAction(tr("Re&load from Disk"));
    connect(aReload, &QAction::triggered, this, &MainWindow::reloadCurrent);

    fileMenu->addSeparator();
    m_actReadOnly = fileMenu->addAction(tr("&Read-Only Mode"));
    m_actReadOnly->setCheckable(true);
    connect(m_actReadOnly, &QAction::toggled, this, &MainWindow::toggleReadOnlyMode);
    QAction* aCopyName = fileMenu->addAction(tr("Copy File &Name"));
    connect(aCopyName, &QAction::triggered, this, &MainWindow::copyFileName);
    QAction* aCopyPath = fileMenu->addAction(tr("Copy Full Pat&h"));
    connect(aCopyPath, &QAction::triggered, this, &MainWindow::copyFilePath);
    QAction* aRename = fileMenu->addAction(tr("Rena&me..."));
    connect(aRename, &QAction::triggered, this, [this]() { renameCurrentDocument(); });
    QAction* aContaining = fileMenu->addAction(tr("Open Containing &Folder"));
    connect(aContaining, &QAction::triggered, this, &MainWindow::showCurrentInFileManager);
    fileMenu->addSeparator();

    QMenu* exportMenu = fileMenu->addMenu(tr("&Export"));
    exportMenu->addAction(tr("&HTML..."), this, &MainWindow::exportCurrentHtml);
    exportMenu->addAction(tr("&RTF..."), this, &MainWindow::exportCurrentRtf);
    QAction* aPrint = fileMenu->addAction(tr("&Print..."));
    aPrint->setShortcut(QKeySequence::Print);
    connect(aPrint, &QAction::triggered, this, [this]() { printCurrent(false); });
    QAction* aPrintPreview = fileMenu->addAction(tr("Print Pre&view..."));
    connect(aPrintPreview, &QAction::triggered, this, [this]() { printCurrent(true); });

    QAction* aClose = fileMenu->addAction(tr("&Close"));
    aClose->setShortcut(QKeySequence::Close);
    connect(aClose, &QAction::triggered, this, [this]() { closeEditor(currentEditor()); });
    QAction* aCloseOthers = fileMenu->addAction(tr("Close All But &This"));
    connect(aCloseOthers, &QAction::triggered, this, &MainWindow::closeOtherDocuments);
    QAction* aCloseAll = fileMenu->addAction(tr("Clos&e All"));
    connect(aCloseAll, &QAction::triggered, this, &MainWindow::closeAllDocuments);
    fileMenu->addSeparator();

    m_encodingMenu = fileMenu->addMenu(tr("&Encoding"));
    struct EncodingChoice {
        const char* label;
        Charset charset;
        bool bom;
    };
    const EncodingChoice encodings[] = {
        {"UTF-8", Charset::Utf8, false},       {"UTF-8 with BOM", Charset::Utf8Bom, true},
        {"UTF-16 LE", Charset::Utf16LE, true}, {"UTF-16 BE", Charset::Utf16BE, true},
        {"UTF-32 LE", Charset::Utf32LE, true}, {"UTF-32 BE", Charset::Utf32BE, true},
        {"Latin-1", Charset::Latin1, false},
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
    addEditorAction(editMenu, tr("Select &All"), QKeySequence::SelectAll,
                    &ScintillaEdit::selectAll);
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
    QAction* aCopyBookmarked = editMenu->addAction(tr("Copy Bookmarked Lines"));
    connect(aCopyBookmarked, &QAction::triggered, this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->copyBookmarkedLines();
    });
    QAction* aCutBookmarked = editMenu->addAction(tr("Cut Bookmarked Lines"));
    connect(aCutBookmarked, &QAction::triggered, this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->cutBookmarkedLines();
    });
    QAction* aDeleteBookmarked = editMenu->addAction(tr("Delete Bookmarked Lines"));
    connect(aDeleteBookmarked, &QAction::triggered, this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->deleteBookmarkedLines();
    });
    QAction* aPasteBookmarked = editMenu->addAction(tr("Paste to Every Bookmarked Line"));
    connect(aPasteBookmarked, &QAction::triggered, this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->pasteToBookmarkedLines();
    });
    QAction* aInvertBookmarks = editMenu->addAction(tr("Invert Bookmarks"));
    connect(aInvertBookmarks, &QAction::triggered, this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->invertBookmarks();
    });

    editMenu->addSeparator();
    QAction* aNextDiff = editMenu->addAction(tr("Next Difference"));
    aNextDiff->setShortcut(QKeySequence(QStringLiteral("F7")));
    connect(aNextDiff, &QAction::triggered, this, [this]() { gotoNextDifference(true); });
    QAction* aPrevDiff = editMenu->addAction(tr("Previous Difference"));
    aPrevDiff->setShortcut(QKeySequence(QStringLiteral("Shift+F7")));
    connect(aPrevDiff, &QAction::triggered, this, [this]() { gotoNextDifference(false); });

    editMenu->addSeparator();
    auto* commentMenu = editMenu->addMenu(tr("Co&mment/Uncomment"));
    QAction* aLineComment = commentMenu->addAction(tr("Toggle Line Comment"));
    aLineComment->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+C")));
    connect(aLineComment, &QAction::triggered, this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->toggleLineComment();
    });
    QAction* aBlockComment = commentMenu->addAction(tr("Toggle Block Comment"));
    aBlockComment->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+B")));
    connect(aBlockComment, &QAction::triggered, this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->toggleBlockComment();
    });

    auto addTransform = [this, editMenu](const QString& label, void (EditorWidget::*fn)()) {
        QAction* action = editMenu->addAction(label);
        connect(action, &QAction::triggered, this, [this, fn]() {
            if (EditorWidget* editor = currentEditor())
                (editor->*fn)();
        });
    };
    editMenu->addSeparator();
    QAction* aDuplicateLine = editMenu->addAction(tr("Duplicate Current Line"));
    aDuplicateLine->setShortcut(QKeySequence(QStringLiteral("Ctrl+D")));
    connect(aDuplicateLine, &QAction::triggered, this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->duplicateCurrentLine();
    });
    addTransform(tr("Delete Current Line"), &EditorWidget::deleteCurrentLine);
    addTransform(tr("Move Current Line Up"), &EditorWidget::moveLineUp);
    addTransform(tr("Move Current Line Down"), &EditorWidget::moveLineDown);
    addTransform(tr("Join Lines"), &EditorWidget::joinSelectedLines);
    addTransform(tr("Split Lines"), &EditorWidget::splitSelectedLines);

    auto* sortMenu = editMenu->addMenu(tr("S&ort Lines"));
    sortMenu->addAction(tr("Ascending (case sensitive)"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->sortLinesAscending();
    });
    sortMenu->addAction(tr("Ascending (case insensitive)"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->sortLinesAscendingCaseInsensitive();
    });
    sortMenu->addAction(tr("Descending"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->sortLinesDescending();
    });

    auto* removeMenu = editMenu->addMenu(tr("Remove Lines"));
    removeMenu->addAction(tr("Remove Duplicate Lines"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->removeDuplicateLines();
    });
    removeMenu->addAction(tr("Remove Consecutive Duplicate Lines"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->removeConsecutiveDuplicateLines();
    });
    removeMenu->addAction(tr("Remove Empty Lines"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->removeEmptyLines();
    });

    auto* caseMenu = editMenu->addMenu(tr("Change Case"));
    caseMenu->addAction(tr("UPPER CASE"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->convertCaseUpper();
    });
    caseMenu->addAction(tr("lower case"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->convertCaseLower();
    });
    caseMenu->addAction(tr("Title Case"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->convertCaseTitle();
    });
    caseMenu->addAction(tr("Sentence case"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->convertCaseSentence();
    });

    auto* trimMenu = editMenu->addMenu(tr("Trim Trailing/Leading Space"));
    trimMenu->addAction(tr("Trim Trailing Space"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->trimTrailingWhitespace();
    });
    trimMenu->addAction(tr("Trim Leading Space"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->trimLeadingWhitespace();
    });
    trimMenu->addAction(tr("Trim Both"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->trimWhitespace();
    });

    addTransform(tr("Tabify"), &EditorWidget::tabifySelection);
    addTransform(tr("Untabify"), &EditorWidget::untabifySelection);

    QAction* aColumnEditor = editMenu->addAction(tr("Column Editor..."));
    aColumnEditor->setShortcut(QKeySequence(QStringLiteral("Ctrl+Alt+C")));
    connect(aColumnEditor, &QAction::triggered, this, &MainWindow::showColumnEditor);

    editMenu->addSeparator();
    QAction* aGoto = editMenu->addAction(tr("Go to &Line..."));
    aGoto->setShortcut(QKeySequence(QStringLiteral("Ctrl+G")));
    connect(aGoto, &QAction::triggered, this, &MainWindow::showGotoLineDialog);
    QAction* aMatchBrace = editMenu->addAction(tr("Go to Matching &Brace"));
    aMatchBrace->setShortcut(QKeySequence(QStringLiteral("Ctrl+B")));
    connect(aMatchBrace, &QAction::triggered, this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->gotoMatchingBrace();
    });
    QAction* aSelectBrace = editMenu->addAction(tr("Select to Matching Brace"));
    connect(aSelectBrace, &QAction::triggered, this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->selectToMatchingBrace();
    });
    QAction* aSummary = editMenu->addAction(tr("Document &Summary..."));
    connect(aSummary, &QAction::triggered, this, &MainWindow::showDocumentSummary);
    QAction* aWordCompletion = editMenu->addAction(tr("Word Completion"));
    aWordCompletion->setShortcut(QKeySequence(QStringLiteral("Ctrl+Space")));
    connect(aWordCompletion, &QAction::triggered, this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->triggerCompletion();
    });
    QAction* aFunctionCompletion = editMenu->addAction(tr("Function &Completion"));
    aFunctionCompletion->setShortcut(QKeySequence(QStringLiteral("Ctrl+Enter")));
    connect(aFunctionCompletion, &QAction::triggered, this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->triggerSymbolCompletion();
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
    viewMenu->addAction(m_actShowLineNumbers);
    viewMenu->addAction(m_actShowFoldMargin);
    viewMenu->addAction(m_actShowBookmarkMargin);
    viewMenu->addAction(m_actShowAllChars);
    viewMenu->addAction(m_actShowEol);
    viewMenu->addAction(m_actShowIndentGuides);
    viewMenu->addAction(m_actHighlightLine);
    QMenu* foldingMenu = viewMenu->addMenu(tr("&Folding"));
    foldingMenu->addAction(tr("Fold All"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->foldAll(true);
    });
    foldingMenu->addAction(tr("Unfold All"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->unfoldAll();
    });
    foldingMenu->addAction(tr("Toggle All Folds"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->toggleAllFolds();
    });
    foldingMenu->addSeparator();
    foldingMenu->addAction(tr("Fold Current"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->foldCurrent(true);
    });
    foldingMenu->addAction(tr("Unfold Current"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->foldCurrent(false);
    });
    foldingMenu->addAction(tr("Toggle Current Fold"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->toggleCurrentFold();
    });
    viewMenu->addSeparator();
    QAction* aExplorer = viewMenu->addAction(tr("E&xplorer"));
    aExplorer->setCheckable(true);
    connect(aExplorer, &QAction::toggled, this, [this](bool on) {
        if (on) {
            m_explorerDock->show();
            m_explorerDock->raise();
        } else {
            m_explorerDock->hide();
        }
    });
    connect(m_explorerDock, &QDockWidget::visibilityChanged, aExplorer, &QAction::setChecked);
    QAction* aDocMap = viewMenu->addAction(tr("Document &Map"));
    aDocMap->setCheckable(true);
    connect(aDocMap, &QAction::toggled, this, [this](bool on) {
        if (on) {
            m_mapDock->show();
            m_mapDock->raise();
        } else {
            m_mapDock->hide();
        }
    });
    connect(m_mapDock, &QDockWidget::visibilityChanged, aDocMap, &QAction::setChecked);
    QAction* aConsole = viewMenu->addAction(tr("Script &Console"));
    aConsole->setCheckable(true);
    connect(aConsole, &QAction::toggled, this, [this](bool on) {
        if (on) {
            m_consoleDock->show();
            m_consoleDock->raise();
        } else {
            m_consoleDock->hide();
        }
    });
    connect(m_consoleDock, &QDockWidget::visibilityChanged, aConsole, &QAction::setChecked);
    QAction* aFunctionList = viewMenu->addAction(tr("&Function List"));
    aFunctionList->setCheckable(true);
    connect(aFunctionList, &QAction::toggled, this, [this](bool on) {
        if (on) {
            m_functionListDock->show();
            m_functionListDock->raise();
        } else {
            m_functionListDock->hide();
        }
    });
    connect(m_functionListDock, &QDockWidget::visibilityChanged, aFunctionList,
            &QAction::setChecked);
    QAction* aCharacterMap = viewMenu->addAction(tr("Character Ma&p"));
    aCharacterMap->setCheckable(true);
    connect(aCharacterMap, &QAction::toggled, this, [this](bool on) {
        if (on) {
            m_characterMapDock->show();
            m_characterMapDock->raise();
        } else {
            m_characterMapDock->hide();
        }
    });
    connect(m_characterMapDock, &QDockWidget::visibilityChanged, aCharacterMap,
            &QAction::setChecked);
    QAction* aSearchResults = viewMenu->addAction(tr("Search R&esults"));
    aSearchResults->setCheckable(true);
    connect(aSearchResults, &QAction::triggered, this, [this](bool checked) {
        if (checked) {
            m_searchResultsDock->show();
            m_searchResultsDock->raise();
        } else {
            m_searchResultsDock->hide();
        }
    });
    connect(m_searchResultsDock, &QDockWidget::visibilityChanged, aSearchResults,
            &QAction::setChecked);
    viewMenu->addSeparator();
    QAction* aMoveView = viewMenu->addAction(tr("Move to Other View"));
    connect(aMoveView, &QAction::triggered, this, &MainWindow::moveToOtherView);
    viewMenu->addSeparator();
    QMenu* orientationMenu = viewMenu->addMenu(tr("Split Orientation"));
    orientationMenu->addAction(m_actSplitHorizontal);
    orientationMenu->addAction(m_actSplitVertical);
    viewMenu->addSeparator();
    m_actSyncScroll = viewMenu->addAction(tr("Synchronized Scrolling"));
    m_actSyncScroll->setCheckable(true);
    m_actSyncScroll->setChecked(m_syncScroll);
    connect(m_actSyncScroll, &QAction::toggled, this, [this](bool on) {
        m_syncScroll = on;
        m_settings.syncScroll = on;
        m_settings.save();
        if (on)
            syncScrollFrom(currentEditor());
    });
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
    viewMenu->addSeparator();
    QAction* aAlwaysOnTop = viewMenu->addAction(m_actAlwaysOnTop->text());
    aAlwaysOnTop->setCheckable(true);
    aAlwaysOnTop->setChecked(m_actAlwaysOnTop->isChecked());
    connect(aAlwaysOnTop, &QAction::toggled, m_actAlwaysOnTop, &QAction::setChecked);
    QAction* aFullScreen = viewMenu->addAction(tr("Full Screen"));
    aFullScreen->setShortcut(QKeySequence(QStringLiteral("F11")));
    connect(aFullScreen, &QAction::triggered, this, &MainWindow::toggleFullScreen);

    m_languageMenu = bar->addMenu(tr("&Language"));
    QAction* aDetect = m_languageMenu->addAction(tr("Detect from Content"));
    connect(aDetect, &QAction::triggered, this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->detectLanguage();
    });
    m_languageMenu->addSeparator();
    for (const Language& language : LanguageManager::languages()) {
        const QString id = language.id;
        QAction* action = m_languageMenu->addAction(language.name);
        action->setCheckable(true);
        action->setData(id);
        connect(action, &QAction::triggered, this, [this, id]() {
            if (EditorWidget* editor = currentEditor())
                editor->setLanguage(id);
        });
        m_languageActions.append(action);
    }

    QMenu* insertMenu = bar->addMenu(tr("&Insert"));
    auto insertDateTimeFormat = [this](const QString& format) {
        if (EditorWidget* editor = currentEditor())
            editor->insertDateTime(format);
    };
    auto* dateTimeMenu = insertMenu->addMenu(tr("Date and &Time"));
    dateTimeMenu->addAction(
        tr("Date and Time (yyyy-MM-dd HH:mm:ss)"), this,
        [insertDateTimeFormat]() { insertDateTimeFormat(QStringLiteral("yyyy-MM-dd HH:mm:ss")); });
    dateTimeMenu->addAction(tr("Date (yyyy-MM-dd)"), this, [insertDateTimeFormat]() {
        insertDateTimeFormat(QStringLiteral("yyyy-MM-dd"));
    });
    dateTimeMenu->addAction(tr("Time (HH:mm:ss)"), this, [insertDateTimeFormat]() {
        insertDateTimeFormat(QStringLiteral("HH:mm:ss"));
    });
    auto* guidMenu = insertMenu->addMenu(tr("&GUID"));
    guidMenu->addAction(tr("GUID (lower case)"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->insertGuid(GuidFormat::Lower);
    });
    guidMenu->addAction(tr("GUID (upper case)"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->insertGuid(GuidFormat::Upper);
    });
    guidMenu->addAction(tr("GUID (braces, upper case)"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->insertGuid(GuidFormat::BracesUpper);
    });
    guidMenu->addAction(tr("GUID (plain, upper case)"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->insertGuid(GuidFormat::Plain);
    });
    guidMenu->addAction(tr("GUID (compact)"), this, [this]() {
        if (EditorWidget* editor = currentEditor())
            editor->insertGuid(GuidFormat::Compact);
    });
    insertMenu->addSeparator();
    insertMenu->addAction(tr("File &Name"), this, [this]() {
        if (EditorWidget* editor = currentEditor()) {
            if (editor->filePath().isEmpty()) {
                showMessage(tr("The current document has no file name."));
                return;
            }
            editor->insertTextAtCursor(QFileInfo(editor->filePath()).fileName());
        }
    });
    insertMenu->addAction(tr("Full &Path"), this, [this]() {
        if (EditorWidget* editor = currentEditor()) {
            if (editor->filePath().isEmpty()) {
                showMessage(tr("The current document has no file path."));
                return;
            }
            editor->insertTextAtCursor(editor->filePath());
        }
    });

    QMenu* toolsMenu = bar->addMenu(tr("&Tools"));
    QMenu* convertMenu = toolsMenu->addMenu(tr("&Convert"));
    convertMenu->addAction(tr("Base64 Encode"), this, [this]() {
        applyTextConversion(&TextOps::base64Encode, tr("Base64 encode"));
    });
    convertMenu->addAction(tr("Base64 Decode"), this, [this]() {
        applyTextConversion([](const QString& text) { return TextOps::base64Decode(text); },
                            tr("Base64 decode"));
    });
    convertMenu->addAction(tr("URL Encode"), this, [this]() {
        applyTextConversion(&TextOps::urlEncode, tr("URL encode"));
    });
    convertMenu->addAction(tr("URL Decode"), this, [this]() {
        applyTextConversion(&TextOps::urlDecode, tr("URL decode"));
    });
    convertMenu->addAction(tr("HTML Entities Encode"), this, [this]() {
        applyTextConversion(&TextOps::htmlEncode, tr("HTML encode"));
    });
    convertMenu->addAction(tr("HTML Entities Decode"), this, [this]() {
        applyTextConversion(&TextOps::htmlDecode, tr("HTML decode"));
    });
    convertMenu->addAction(tr("Unicode Escape"), this, [this]() {
        applyTextConversion(&TextOps::unicodeEscape, tr("Unicode escape"));
    });
    convertMenu->addAction(tr("Unicode Unescape"), this, [this]() {
        applyTextConversion(&TextOps::unicodeUnescape, tr("Unicode unescape"));
    });
    convertMenu->addAction(tr("Rot13"), this,
                           [this]() { applyTextConversion(&TextOps::rot13, tr("Rot13")); });
    convertMenu->addAction(tr("Text to Hex"), this,
                           [this]() { applyTextConversion(&TextOps::toHex, tr("Text to hex")); });
    convertMenu->addAction(tr("Hex to Text"), this, [this]() {
        applyTextConversion([](const QString& text) { return TextOps::fromHex(text); },
                            tr("Hex to text"));
    });

    QMenu* digestMenu = toolsMenu->addMenu(tr("&Hash / Digest"));
    struct DigestChoice {
        const char* label;
        QCryptographicHash::Algorithm algorithm;
    };
    const DigestChoice digests[] = {
        {"MD5", QCryptographicHash::Md5},        {"SHA-1", QCryptographicHash::Sha1},
        {"SHA-256", QCryptographicHash::Sha256}, {"SHA-384", QCryptographicHash::Sha384},
        {"SHA-512", QCryptographicHash::Sha512},
    };
    for (const DigestChoice& choice : digests) {
        const QCryptographicHash::Algorithm algorithm = choice.algorithm;
        const QString label = tr(choice.label);
        digestMenu->addAction(label, this,
                              [this, algorithm, label]() { showDigest(algorithm, label); });
    }

    toolsMenu->addSeparator();
    QAction* aCompare = toolsMenu->addAction(tr("&Compare Documents..."));
    connect(aCompare, &QAction::triggered, this, &MainWindow::compareDocuments);
    QAction* aClearCompare = toolsMenu->addAction(tr("&Clear Compare Highlight"));
    connect(aClearCompare, &QAction::triggered, this, &MainWindow::clearCompare);

    toolsMenu->addSeparator();
    QAction* aConsoleShow = toolsMenu->addAction(tr("Script &Console"));
    connect(aConsoleShow, &QAction::triggered, this, &MainWindow::showScriptConsole);

    QMenu* settingsMenu = bar->addMenu(tr("&Settings"));
    QAction* aRun = settingsMenu->addAction(tr("&Run..."));
    aRun->setShortcut(QKeySequence(QStringLiteral("F5")));
    connect(aRun, &QAction::triggered, this, &MainWindow::showRunDialog);
    settingsMenu->addSeparator();
    QAction* aPreferences = settingsMenu->addAction(tr("&Preferences..."));
    aPreferences->setShortcut(QKeySequence::Preferences);
    connect(aPreferences, &QAction::triggered, this, &MainWindow::showPreferences);

    m_windowMenu = bar->addMenu(tr("&Window"));
    QAction* aNextTab = m_windowMenu->addAction(tr("Next Tab"));
    aNextTab->setShortcut(QKeySequence(QStringLiteral("Ctrl+Tab")));
    connect(aNextTab, &QAction::triggered, this, [this]() { nextTab(true); });
    QAction* aPrevTab = m_windowMenu->addAction(tr("Previous Tab"));
    aPrevTab->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+Tab")));
    connect(aPrevTab, &QAction::triggered, this, [this]() { nextTab(false); });
    QMenu* sortTabsMenu = m_windowMenu->addMenu(tr("Sort Tabs"));
    sortTabsMenu->addAction(tr("By Name"), this, [this]() { sortTabs(false); });
    sortTabsMenu->addAction(tr("By Path"), this, [this]() { sortTabs(true); });
    m_windowMenu->addAction(m_actSplitView);
    m_windowMenu->addAction(aMoveView);
    m_windowMenu->addSeparator();
    QAction* aDetach = m_windowMenu->addAction(tr("Detach Current Tab to Floating Window"));
    aDetach->setShortcut(QKeySequence(QStringLiteral("Ctrl+Alt+D")));
    connect(aDetach, &QAction::triggered, this, &MainWindow::detachCurrentToFloating);
    QAction* aMergeWindows = m_windowMenu->addAction(tr("Merge All Floating Windows"));
    connect(aMergeWindows, &QAction::triggered, this, &MainWindow::mergeFloatingWindows);
    m_windowMenu->addSeparator();
    updateWindowMenu();
    updateLanguageMenu();

    QMenu* helpMenu = bar->addMenu(tr("&Help"));
    QAction* aAbout = helpMenu->addAction(tr("&About PadForge"));
    connect(aAbout, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::createToolbar() {
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

void MainWindow::createStatusBar() {
    m_statusLanguage = new QLabel(this);
    m_statusEncoding = new QLabel(this);
    m_statusEol = new QLabel(this);
    m_statusPosition = new QLabel(this);
    m_statusReadOnly = new QLabel(this);
    statusBar()->addWidget(m_statusLanguage, 1);
    statusBar()->addPermanentWidget(m_statusReadOnly);
    statusBar()->addPermanentWidget(m_statusEncoding);
    statusBar()->addPermanentWidget(m_statusEol);
    statusBar()->addPermanentWidget(m_statusPosition);
}

void MainWindow::createDocks() {
    m_explorerPanel = new ExplorerPanel(this);
    m_explorerDock = new QDockWidget(tr("Explorer"), this);
    m_explorerDock->setObjectName(QStringLiteral("explorerDock"));
    m_explorerDock->setWidget(m_explorerPanel);
    addDockWidget(Qt::LeftDockWidgetArea, m_explorerDock);
    m_explorerDock->hide();
    connect(m_explorerPanel, &ExplorerPanel::fileActivated, this,
            [this](const QString& path) { openPath(path); });

    m_findInFilesPanel = new FindInFilesPanel(this);
    m_findDock = new QDockWidget(tr("Find in Files"), this);
    m_findDock->setObjectName(QStringLiteral("findInFilesDock"));
    m_findDock->setWidget(m_findInFilesPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_findDock);
    m_findDock->hide();
    connect(m_findInFilesPanel, &FindInFilesPanel::resultActivated, this, &MainWindow::gotoResult);

    m_documentMap = new DocumentMap(this);
    m_mapDock = new QDockWidget(tr("Document Map"), this);
    m_mapDock->setObjectName(QStringLiteral("documentMapDock"));
    m_mapDock->setWidget(m_documentMap);
    addDockWidget(Qt::RightDockWidgetArea, m_mapDock);
    m_mapDock->hide();
    connect(m_mapDock, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (visible)
            updateDocumentMapSource();
        else
            m_documentMap->detachEditor();
    });

    m_consolePanel = new ConsolePanel(m_pluginHost, this);
    m_consoleDock = new QDockWidget(tr("Script Console"), this);
    m_consoleDock->setObjectName(QStringLiteral("consoleDock"));
    m_consoleDock->setWidget(m_consolePanel);
    addDockWidget(Qt::BottomDockWidgetArea, m_consoleDock);
    m_consoleDock->hide();

    m_functionList = new FunctionListPanel(this);
    m_functionListDock = new QDockWidget(tr("Function List"), this);
    m_functionListDock->setObjectName(QStringLiteral("functionListDock"));
    m_functionListDock->setWidget(m_functionList);
    addDockWidget(Qt::RightDockWidgetArea, m_functionListDock);
    m_functionListDock->hide();
    connect(m_functionListDock, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (visible)
            updateFunctionListSource();
        else
            m_functionList->detachEditor();
    });

    m_characterMap = new CharacterMapPanel(this);
    m_characterMapDock = new QDockWidget(tr("Character Map"), this);
    m_characterMapDock->setObjectName(QStringLiteral("characterMapDock"));
    m_characterMapDock->setWidget(m_characterMap);
    addDockWidget(Qt::RightDockWidgetArea, m_characterMapDock);
    m_characterMapDock->hide();
    connect(m_characterMap, &CharacterMapPanel::characterSelected, this,
            [this](const QString& text) { insertText(text); });

    m_searchResultsPanel = new SearchResultsPanel(this);
    m_searchResultsDock = new QDockWidget(tr("Search Results"), this);
    m_searchResultsDock->setObjectName(QStringLiteral("searchResultsDock"));
    m_searchResultsDock->setWidget(m_searchResultsPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_searchResultsDock);
    m_searchResultsDock->hide();
    connect(m_searchResultsPanel, &SearchResultsPanel::resultActivated, this,
            &MainWindow::goToSearchResult);
}

void MainWindow::createPluginMenu() {
    m_pluginsMenu = menuBar()->addMenu(tr("&Plugins"));
    QAction* reload = m_pluginsMenu->addAction(tr("Reload Plugins"));
    connect(reload, &QAction::triggered, this, &MainWindow::loadPlugins);
    m_pluginsMenu->addSeparator();
}

void MainWindow::newDocumentDialog() {
    newDocument();
}

void MainWindow::openFilesDialog() {
    const QStringList files = QFileDialog::getOpenFileNames(this, tr("Open File"));
    openPaths(files);
}

void MainWindow::saveCurrent() {
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

void MainWindow::saveCurrentAs() {
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

void MainWindow::saveCurrentCopyAs() {
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    const QString path =
        QFileDialog::getSaveFileName(this, tr("Save a Copy As"), editor->filePath());
    if (path.isEmpty())
        return;
    QString error;
    if (!editor->saveCopyAs(path, &error))
        QMessageBox::warning(this, tr("PadForge"), tr("Cannot save copy:\n%1").arg(error));
    else
        showMessage(tr("Saved a copy."));
}

void MainWindow::saveAll() {
    for (EditorWidget* editor : m_editors) {
        if (!editor->isModified())
            continue;
        if (editor->filePath().isEmpty()) {
            auto* tabs = tabWidgetFor(editor);
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

void MainWindow::reloadCurrent() {
    reloadEditor(currentEditor());
}

void MainWindow::setEncodingForCurrent(const EncodingInfo& encoding, bool reencode) {
    if (EditorWidget* editor = currentEditor())
        editor->setEncoding(encoding, reencode);
}

void MainWindow::setEolForCurrent(Eol eol) {
    if (EditorWidget* editor = currentEditor())
        editor->setEol(eol);
}

void MainWindow::showFindDialog() {
    m_findDialog->show();
}

void MainWindow::findNext(bool up) {
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    SearchOptions options = m_findDialog->options();
    options.searchUp = up;
    editor->findNextFromCursor(m_findDialog->term(), options);
}

void MainWindow::replaceOne() {
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    editor->replaceCurrent(m_findDialog->term(), m_findDialog->replacement(),
                           m_findDialog->options());
}

void MainWindow::replaceAll() {
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    const QString term = m_findDialog->term();
    if (term.isEmpty())
        return;
    int count = 0;
    const QString replaced = SearchEngine::replaceAll(
        editor->documentText(), term, m_findDialog->replacement(), m_findDialog->options(), &count);
    editor->setDocumentText(replaced);
    showMessage(tr("Replaced %1 occurrence(s).").arg(count));
}

void MainWindow::countMatches() {
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    const int count =
        SearchEngine::count(editor->documentText(), m_findDialog->term(), m_findDialog->options());
    showMessage(tr("%1 match(es) found.").arg(count));
}

void MainWindow::markAll() {
    if (EditorWidget* editor = currentEditor())
        editor->highlightMatches(m_findDialog->term(), m_findDialog->options());
}

void MainWindow::findAllInOpen(bool allDocuments) {
    EditorWidget* current = currentEditor();
    if (!current)
        return;
    const QString term = m_findDialog->term();
    if (term.isEmpty()) {
        showMessage(tr("Nothing to search for."));
        return;
    }
    const SearchOptions options = m_findDialog->options();
    m_searchResultsPanel->begin(term);
    int total = 0;
    const QList<EditorWidget*> targets = allDocuments ? m_editors : QList<EditorWidget*>({current});
    for (EditorWidget* editor : targets) {
        if (!editor || editor->isLargeFile())
            continue;
        const QString text = editor->documentText();
        const QList<SearchMatch> matches = SearchEngine::findAll(text, term, options, 100000);
        for (const SearchMatch& match : matches) {
            const int line = TextOps::lineNumberAt(text, int(match.start));
            const int column = int(match.start) - TextOps::lineStartAt(text, line) + 1;
            m_searchResultsPanel->addHit(editor, line, column,
                                         TextOps::lineContent(text, line).trimmed());
        }
        total += matches.size();
    }
    m_searchResultsPanel->finish(total);
    m_searchResultsDock->show();
    m_searchResultsDock->raise();
    showMessage(total > 0 ? tr("%1 match(es) found in open document(s).").arg(total)
                          : tr("No matches found."));
}

void MainWindow::goToSearchResult(EditorWidget* editor, int line) {
    if (!editor)
        return;
    auto* tabs = tabWidgetFor(editor);
    if (tabs) {
        tabs->setCurrentWidget(editor);
        m_activeTabs = tabs;
    }
    editor->gotoLine(line);
    editor->setFocus();
}

void MainWindow::gotoResult(const QString& file, int line) {
    EditorWidget* editor = openPath(file);
    if (editor)
        editor->gotoLine(line);
}

void MainWindow::toggleSplitView(bool enabled) {
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
        EditorWidget* editor = createEditor();
        m_subTabs->addTab(editor, tr("Untitled"));
        m_subTabs->setCurrentWidget(editor);
        registerEditor(editor);
        ++m_untitledCounter;
        updateTabTitle(editor);
    }
    m_actSplitView->setChecked(enabled);
}

void MainWindow::moveToOtherView() {
    moveEditorToOtherView(currentEditor());
}

void MainWindow::moveEditorToOtherView(EditorWidget* editor) {
    if (!editor)
        return;
    auto* from = tabWidgetFor(editor);
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

void MainWindow::setTheme(const QString& id) {
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

void MainWindow::applyThemeToAll() {
    const Theme& theme = ThemeManager::current();
    for (EditorWidget* editor : m_editors)
        editor->applyTheme(theme);
}

void MainWindow::setWordWrap(bool enabled) {
    for (EditorWidget* editor : m_editors)
        editor->editor()->send(SCI_SETWRAPMODE, enabled ? SC_WRAP_WORD : SC_WRAP_NONE);
}

void MainWindow::applySettingsToAll() {
    for (EditorWidget* editor : m_editors)
        editor->applySettings(m_settings);
}

void MainWindow::showPreferences() {
    PreferencesDialog dialog(m_settings, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_settings = dialog.resultSettings();
    m_settings.save();
    setTheme(m_settings.theme);
    m_actWordWrap->setChecked(m_settings.wordWrap);
    m_actShowLineNumbers->setChecked(m_settings.showLineNumbers);
    m_actShowFoldMargin->setChecked(m_settings.showFoldMargin);
    m_actShowBookmarkMargin->setChecked(m_settings.showBookmarkMargin);
    m_actShowAllChars->setChecked(m_settings.showWhitespace);
    m_actShowEol->setChecked(m_settings.showEol);
    m_actShowIndentGuides->setChecked(m_settings.showIndentGuides);
    m_actHighlightLine->setChecked(m_settings.highlightCurrentLine);
    applySettingsToAll();
    showMessage(tr("Preferences saved."));
}

void MainWindow::showColumnEditor() {
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    ColumnEditorDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const QString text = editor->documentText();
    const int totalLines = TextOps::lineCount(text);
    const int startLine = qMax(0, dialog.fromLine() - 1);
    const int endLine =
        (dialog.toLine() < 0) ? totalLines - 1 : qMin(totalLines - 1, dialog.toLine() - 1);
    if (startLine >= totalLines || startLine > endLine) {
        showMessage(tr("No lines to edit."));
        return;
    }
    const bool useTabs = m_settings.tabMode == QStringLiteral("tabs");
    const QString result =
        dialog.insertTextMode()
            ? TextOps::columnInsertText(text, dialog.column(), dialog.text(), startLine, endLine,
                                        dialog.repeatIntoEmptyLines(), useTabs, m_settings.tabWidth)
            : TextOps::columnInsertNumbers(text, dialog.column(), dialog.initialNumber(),
                                           dialog.step(), dialog.paddingWidth(), startLine, endLine,
                                           dialog.repeatIntoEmptyLines(), useTabs,
                                           m_settings.tabWidth);
    editor->setDocumentText(result);
    showMessage(tr("Column editor applied."));
}

void MainWindow::showGotoLineDialog() {
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    const int maxLine = static_cast<int>(editor->editor()->lineCount());
    GotoLineDialog dialog(maxLine, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    editor->gotoLine(dialog.lineNumber());
    editor->setFocus();
}

void MainWindow::showDocumentSummary() {
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    const QString text = editor->documentText();
    const int words = TextOps::wordCount(text);
    const int lines = TextOps::lineCount(text);
    const int chars = TextOps::characterCount(text);
    const int ascii =
        int(std::count_if(text.begin(), text.end(), [](QChar c) { return c.unicode() < 128; }));
    const QString summary =
        tr("Document: %1\nWords: %2\nLines: %3\nCharacters: %4\n"
           "ASCII characters: %5\nBytes (UTF-8): %6")
            .arg(editor->filePath().isEmpty() ? tr("Untitled") : editor->filePath())
            .arg(words)
            .arg(lines)
            .arg(chars)
            .arg(ascii)
            .arg(text.toUtf8().size());
    DocumentSummaryDialog dialog(summary, this);
    dialog.exec();
}

void MainWindow::showRunDialog() {
    EditorWidget* editor = currentEditor();
    const QString path = editor ? editor->filePath() : QString();
    RunDialog dialog(path, this);
    dialog.exec();
}

void MainWindow::toggleFullScreen() {
    if (isFullScreen())
        showNormal();
    else
        showFullScreen();
    applySettingsToAll();
}

void MainWindow::toggleAlwaysOnTop() {
    const bool on = m_actAlwaysOnTop->isChecked();
    m_settings.alwaysOnTop = on;
    setWindowFlag(Qt::WindowStaysOnTopHint, on);
    show();
    m_settings.save();
}

void MainWindow::setSplitOrientation(int orientation) {
    m_settings.splitOrientation = orientation;
    m_splitter->setOrientation(orientation == 1 ? Qt::Vertical : Qt::Horizontal);
    m_actSplitHorizontal->setChecked(orientation == 0);
    m_actSplitVertical->setChecked(orientation == 1);
    m_settings.save();
    updateWindowTitle();
}

void MainWindow::nextTab(bool forward) {
    if (!m_activeTabs || m_activeTabs->count() < 2)
        return;
    const int count = m_activeTabs->count();
    const int current = m_activeTabs->currentIndex();
    int next = forward ? (current + 1) % count : (current - 1 + count) % count;
    m_activeTabs->setCurrentIndex(next);
}

void MainWindow::selectTabByNumber(int number) {
    if (number < 0 || number >= m_editors.size())
        return;
    EditorWidget* editor = m_editors.at(number);
    auto* tabs = tabWidgetFor(editor);
    if (!tabs)
        return;
    tabs->setCurrentWidget(editor);
    m_activeTabs = tabs;
    editor->setFocus();
}

void MainWindow::updateWindowMenu() {
    if (!m_windowMenu)
        return;
    for (int i = m_windowMenu->actions().size() - 1; i >= 0; --i) {
        QAction* action = m_windowMenu->actions().at(i);
        if (action->data().toString() == QStringLiteral("docmenu"))
            m_windowMenu->removeAction(action);
    }
    for (int i = 0; i < m_editors.size(); ++i) {
        EditorWidget* editor = m_editors.at(i);
        const QString name = editor->filePath().isEmpty()
                                 ? tr("Untitled")
                                 : QFileInfo(editor->filePath()).fileName();
        QAction* action = new QAction(tr("&%1. %2").arg((i + 1) % 10).arg(name), m_windowMenu);
        action->setData(QStringLiteral("docmenu"));
        if (i < 10)
            action->setShortcut(QKeySequence(QStringLiteral("Alt+%1").arg((i + 1) % 10)));
        action->setCheckable(true);
        action->setChecked(editor == currentEditor());
        const int index = i;
        connect(action, &QAction::triggered, this, [this, index]() { selectTabByNumber(index); });
        m_windowMenu->addAction(action);
    }
}

void MainWindow::openFolderDialog() {
    const QString dir =
        QFileDialog::getExistingDirectory(this, tr("Folder as Workspace"), m_explorerPanel->root());
    if (dir.isEmpty())
        return;
    m_explorerPanel->setRoot(dir);
    m_explorerDock->show();
    m_explorerDock->raise();
}

void MainWindow::updateDocumentMapSource() {
    EditorWidget* editor = currentEditor();
    if (!editor) {
        m_documentMap->detachEditor();
        return;
    }
    m_documentMap->attachEditor(editor);
    if (m_explorerPanel && m_explorerPanel->isFollowing())
        m_explorerPanel->revealPath(editor->filePath());
}

void MainWindow::updateFunctionListSource() {
    if (!m_functionList)
        return;
    if (EditorWidget* editor = currentEditor())
        m_functionList->attachEditor(editor);
    else
        m_functionList->detachEditor();
}

EditorWidget* MainWindow::otherViewEditor(EditorWidget* editor) const {
    auto* tabs = tabWidgetFor(editor);
    if (!tabs)
        return nullptr;
    QTabWidget* other = (tabs == m_mainTabs) ? m_subTabs : m_mainTabs;
    if (!other || other->isHidden() || other->count() == 0)
        return nullptr;
    return qobject_cast<EditorWidget*>(other->currentWidget());
}

void MainWindow::syncScrollFrom(EditorWidget* source) {
    if (!m_syncScroll || m_syncingScroll || !source)
        return;
    EditorWidget* other = otherViewEditor(source);
    if (!other || other == source)
        return;
    const int line = int(source->editor()->firstVisibleLine());
    if (int(other->editor()->firstVisibleLine()) == line)
        return;
    m_syncingScroll = true;
    other->setFirstVisibleLine(line);
    m_syncingScroll = false;
}

void MainWindow::ensureSplitView() {
    if (!m_subTabs->isVisible()) {
        m_subTabs->setVisible(true);
        m_actSplitView->setChecked(true);
    }
}

void MainWindow::moveEditorTo(EditorWidget* editor, QTabWidget* view) {
    auto* from = tabWidgetFor(editor);
    if (from == view) {
        view->setCurrentWidget(editor);
        m_activeTabs = view;
        return;
    }
    if (from)
        from->removeTab(from->indexOf(editor));
    view->addTab(editor, tr("Untitled"));
    view->setCurrentWidget(editor);
    m_activeTabs = view;
    updateTabTitle(editor);
}

void MainWindow::compareDocuments() {
    if (m_editors.size() < 2) {
        showMessage(tr("Compare needs at least two open documents."));
        return;
    }
    CompareDocumentsDialog dialog(m_editors, currentEditor(), this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    startCompare(dialog.firstDocument(), dialog.secondDocument());
}

void MainWindow::startCompare(EditorWidget* first, EditorWidget* second) {
    if (!first || !second || first == second)
        return;
    clearCompare();
    ensureSplitView();
    moveEditorTo(first, m_mainTabs);
    moveEditorTo(second, m_subTabs);

    const QList<int> removed = Diff::removedLines(first->documentText(), second->documentText());
    const QList<int> added = Diff::addedLines(first->documentText(), second->documentText());
    first->setCompareMarkers(removed, {});
    second->setCompareMarkers({}, added);

    m_compareA = first;
    m_compareB = second;
    m_compareLinesA = removed;
    m_compareLinesB = added;
    m_compareActive = true;

    m_syncScrollBeforeCompare = m_syncScroll;
    if (!m_syncScroll) {
        m_syncScroll = true;
        if (m_actSyncScroll) {
            const QSignalBlocker blocker(m_actSyncScroll);
            m_actSyncScroll->setChecked(true);
        }
    }
    showMessage(
        tr("Compare: %1 lines added, %2 lines removed.").arg(added.size()).arg(removed.size()));
}

void MainWindow::clearCompare() {
    if (m_compareA)
        m_compareA->clearCompareMarkers();
    if (m_compareB)
        m_compareB->clearCompareMarkers();
    if (m_compareActive && m_syncScroll != m_syncScrollBeforeCompare) {
        m_syncScroll = m_syncScrollBeforeCompare;
        if (m_actSyncScroll) {
            const QSignalBlocker blocker(m_actSyncScroll);
            m_actSyncScroll->setChecked(m_syncScroll);
        }
    }
    m_compareA = nullptr;
    m_compareB = nullptr;
    m_compareLinesA.clear();
    m_compareLinesB.clear();
    m_compareActive = false;
}

void MainWindow::gotoNextDifference(bool forward) {
    if (!m_compareActive)
        return;
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    const QList<int>* lines = nullptr;
    if (editor == m_compareA)
        lines = &m_compareLinesA;
    else if (editor == m_compareB)
        lines = &m_compareLinesB;
    if (!lines || lines->isEmpty())
        return;
    const int caret = editor->currentLine();
    int best = -1;
    if (forward) {
        for (int line : *lines) {
            if (line > caret) {
                best = line;
                break;
            }
        }
        if (best < 0)
            best = lines->first();
    } else {
        for (qsizetype i = lines->size() - 1; i >= 0; --i) {
            if (lines->at(i) < caret) {
                best = lines->at(i);
                break;
            }
        }
        if (best < 0)
            best = lines->last();
    }
    if (best > 0) {
        editor->gotoLine(best);
        editor->setFocus();
    }
}

void MainWindow::snapshotModifiedDocuments() {
    if (!m_settings.backupEnabled)
        return;
    BackupManager::clear();
    int untitledIndex = 0;
    for (EditorWidget* editor : m_editors) {
        if (editor->isLargeFile())
            continue;
        if (!editor->isModified() && !editor->isUntitled())
            continue;
        BackupEntry entry;
        if (editor->isUntitled()) {
            entry.key = QStringLiteral("untitled-%1").arg(untitledIndex++);
            entry.untitled = true;
        } else {
            entry.key = BackupManager::keyForPath(editor->filePath());
            entry.originalPath = editor->filePath();
        }
        entry.text = editor->documentText();
        BackupManager::write(entry);
    }
}

void MainWindow::offerRestoreBackups() {
    const QList<BackupEntry> entries = BackupManager::list();
    if (entries.isEmpty())
        return;
    const auto choice = QMessageBox::question(
        this, tr("Recover unsaved documents"),
        tr("PadForge did not shut down cleanly. Restore %1 unsaved document(s)?")
            .arg(entries.size()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (choice == QMessageBox::Yes) {
        for (const BackupEntry& entry : entries) {
            if (entry.untitled)
                newDocument(entry.text);
            else
                newDocument(entry.text, entry.originalPath);
        }
        showMessage(tr("Restored %1 unsaved document(s).").arg(entries.size()));
    }
    BackupManager::clear();
}

void MainWindow::reattachToMain(EditorWidget* editor) {
    if (!editor)
        return;
    auto* from = tabWidgetFor(editor);
    if (from)
        from->removeTab(from->indexOf(editor));
    m_mainTabs->addTab(editor, tr("Untitled"));
    m_mainTabs->setCurrentWidget(editor);
    m_activeTabs = m_mainTabs;
    updateTabTitle(editor);
    editor->setFocus();
    updateWindowMenu();
    updateWindowTitle();
}

void MainWindow::detachCurrentToFloating() {
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    auto* from = tabWidgetFor(editor);
    if (!from)
        return;
    auto* window = new FloatingWindow;
    window->setAttribute(Qt::WA_DeleteOnClose);
    m_floatingWindows.append(window);
    connect(window, &QObject::destroyed, this,
            [this, window]() { m_floatingWindows.removeAll(window); });
    from->removeTab(from->indexOf(editor));
    window->addEditor(editor, this);
    window->show();
    window->raise();
    updateWindowMenu();
    updateWindowTitle();
}

void MainWindow::mergeFloatingWindows() {
    for (FloatingWindow* window : QList<FloatingWindow*>(m_floatingWindows))
        window->close();
}

void MainWindow::updateLanguageMenu() {
    if (m_languageActions.isEmpty())
        return;
    EditorWidget* editor = currentEditor();
    const QString current = editor ? editor->languageId() : QString();
    for (QAction* action : m_languageActions)
        action->setChecked(action->data().toString() == current);
}

void MainWindow::exportCurrentHtml() {
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    const QFileInfo info(editor->filePath());
    const QString defaultName =
        info.isFile() ? info.absolutePath() + QLatin1Char('/') + info.completeBaseName() + ".html"
                      : QDir::homePath() + QStringLiteral("/Untitled.html");
    const QString path = QFileDialog::getSaveFileName(this, tr("Export as HTML"), defaultName,
                                                      tr("HTML files (*.html *.htm)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, tr("PadForge"),
                             tr("Cannot write %1:\n%2").arg(path, file.errorString()));
        return;
    }
    file.write(editor->exportHtml().toUtf8());
    file.close();
    showMessage(tr("Exported to %1").arg(path));
}

void MainWindow::exportCurrentRtf() {
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    const QFileInfo info(editor->filePath());
    const QString defaultName =
        info.isFile() ? info.absolutePath() + QLatin1Char('/') + info.completeBaseName() + ".rtf"
                      : QDir::homePath() + QStringLiteral("/Untitled.rtf");
    const QString path = QFileDialog::getSaveFileName(this, tr("Export as RTF"), defaultName,
                                                      tr("RTF files (*.rtf)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, tr("PadForge"),
                             tr("Cannot write %1:\n%2").arg(path, file.errorString()));
        return;
    }
    file.write(editor->exportRtf().toUtf8());
    file.close();
    showMessage(tr("Exported to %1").arg(path));
}

void MainWindow::printCurrent(bool preview) {
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    QTextDocument* document = editor->printDocument();
    if (preview) {
        QPrinter printer(QPrinter::HighResolution);
        QPrintPreviewDialog dialog(&printer, this);
        connect(&dialog, &QPrintPreviewDialog::paintRequested, this,
                [document](QPrinter* printer) { document->print(printer); });
        dialog.exec();
    } else {
        QPrinter printer(QPrinter::HighResolution);
        QPrintDialog dialog(&printer, this);
        if (dialog.exec() == QDialog::Accepted)
            document->print(&printer);
    }
    delete document;
}

void MainWindow::applyTextConversion(const std::function<QString(const QString&)>& transform,
                                     const QString& label) {
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    editor->transformSelectionOrDocument(transform);
    showMessage(tr("%1 applied").arg(label));
}

void MainWindow::showDigest(QCryptographicHash::Algorithm algorithm, const QString& name) {
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    const QString hash = TextOps::digest(editor->selectedTextOrDocument(), algorithm);
    QApplication::clipboard()->setText(hash);
    QMessageBox::information(this, name, tr("%1 (copied to clipboard):\n%2").arg(name, hash));
}

void MainWindow::showScriptConsole() {
    m_consoleDock->show();
    m_consoleDock->raise();
}

void MainWindow::toggleReadOnlyMode(bool readOnly) {
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    editor->setReadOnly(readOnly);
    updateStatus();
}

void MainWindow::copyFileName() {
    EditorWidget* editor = currentEditor();
    if (!editor || editor->filePath().isEmpty()) {
        showMessage(tr("The current document has no file path."));
        return;
    }
    QApplication::clipboard()->setText(QFileInfo(editor->filePath()).fileName());
    showMessage(tr("Copied file name"));
}

void MainWindow::copyFilePath() {
    EditorWidget* editor = currentEditor();
    if (!editor || editor->filePath().isEmpty()) {
        showMessage(tr("The current document has no file path."));
        return;
    }
    QApplication::clipboard()->setText(editor->filePath());
    showMessage(tr("Copied full path"));
}

void MainWindow::loadSession() {
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
        if (file.readOnly)
            editor->setReadOnly(true);
        editor->applySavedState(file.cursor, file.anchor, file.firstVisibleLine,
                                file.bookmarkLines);
    }

    if (session.splitView && !session.subViewFiles.isEmpty()) {
        toggleSplitView(true);
        m_subTabs->clear();
        for (const SessionFile& file : session.subViewFiles) {
            EditorWidget* editor =
                file.untitled ? newDocument(file.untitledText) : openPath(file.path, false);
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

void MainWindow::saveSession() {
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
        file.readOnly = editor->isReadOnly();
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

void MainWindow::updateRecentMenu() {
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

void MainWindow::addRecent(const QString& path) {
    if (path.isEmpty())
        return;
    m_recent.add(path);
    m_recent.save(recentPath());
    updateRecentMenu();
}

void MainWindow::loadMacros() {
    bool ok = false;
    m_macros = MacroLibrary::load(MacroLibrary::defaultPath(), &ok);
    if (!ok)
        m_macros.clear();
}

void MainWindow::saveMacros() {
    MacroLibrary::save(m_macros, MacroLibrary::defaultPath());
}

void MainWindow::loadPlugins() {
    const QString userDir = pluginsPath();
    QDir().mkpath(userDir);
    m_pluginHost->setPluginDirectory(userDir);

    QStringList dirs;
    dirs << userDir;
    for (const QString& base :
         QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation))
        dirs << base + QStringLiteral("/PadForge/plugins");
    dirs << QCoreApplication::applicationDirPath() + QStringLiteral("/plugins");

    for (const QString& dir : dirs) {
        QDir directory(dir);
        if (!directory.exists())
            continue;
        m_pluginHost->loadLuaPluginsIn(dir);
        for (const QString& entry :
             directory.entryList({QStringLiteral("*.so"), QStringLiteral("*.so.*")}, QDir::Files)) {
            m_pluginHost->loadNativePlugin(directory.absoluteFilePath(entry));
        }
    }
    showMessage(tr("Loaded %1 plugin(s).").arg(m_pluginHost->loadedPlugins().size()));
}

void MainWindow::startMacroRecording() {
    bool ok = false;
    const QString name =
        QInputDialog::getText(this, tr("Record Macro"), tr("Macro name:"), QLineEdit::Normal,
                              tr("Macro %1").arg(m_macros.size() + 1), &ok);
    if (!ok || name.isEmpty())
        return;
    m_macroRecorder.start(name);
    m_actRecordMacro->setText(tr("Stop recording"));
    showMessage(tr("Recording macro \"%1\"...").arg(name));
}

void MainWindow::stopMacroRecording() {
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

void MainWindow::playMacro() {
    if (m_macros.isEmpty()) {
        showMessage(tr("No macros recorded."));
        return;
    }
    QStringList names;
    for (const Macro& macro : m_macros)
        names << macro.name;
    bool ok = false;
    const QString chosen =
        QInputDialog::getItem(this, tr("Play Macro"), tr("Macro:"), names, 0, false, &ok);
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

void MainWindow::showAbout() {
    QMessageBox::about(this, tr("About PadForge"),
                       tr("<h3>PadForge</h3>"
                          "<p>A fast, native Linux text editor inspired by the Notepad++ "
                          "workflow. Built with Qt 6, Scintilla and Lexilla.</p>"
                          "<p>PadForge is free software released under the GNU GPL v3. "
                          "Notepad++ is an independent project and is not affiliated with "
                          "PadForge.</p>"));
}

void MainWindow::insertText(const QString& text) {
    EditorWidget* editor = currentEditor();
    if (!editor)
        return;
    const QByteArray utf8 = text.toUtf8();
    editor->editor()->send(SCI_REPLACESEL, 0, reinterpret_cast<sptr_t>(utf8.constData()));
}

void MainWindow::setDocumentText(const QString& text) {
    if (EditorWidget* editor = currentEditor())
        editor->setDocumentText(text);
}

QString MainWindow::documentText() const {
    EditorWidget* editor = currentEditor();
    return editor ? editor->documentText() : QString();
}

int MainWindow::lineCount() const {
    EditorWidget* editor = currentEditor();
    return editor ? static_cast<int>(editor->editor()->lineCount()) : 0;
}

int MainWindow::currentLine() const {
    EditorWidget* editor = currentEditor();
    return editor ? editor->currentLine() : 0;
}

void MainWindow::gotoLine(int oneBasedLine) {
    if (EditorWidget* editor = currentEditor())
        editor->gotoLine(oneBasedLine);
}

void MainWindow::setLanguage(const QString& lexerId) {
    if (EditorWidget* editor = currentEditor())
        editor->setLanguage(lexerId);
}

void MainWindow::showMessage(const QString& message) {
    statusBar()->showMessage(message, 8000);
}

void MainWindow::addMenuItem(const QString& name, std::function<void()> callback) {
    if (!m_pluginsMenu)
        return;
    QAction* action = m_pluginsMenu->addAction(name);
    connect(action, &QAction::triggered, this, [callback]() { callback(); });
    ++m_pluginMenuCount;
}

} // namespace PadForge
