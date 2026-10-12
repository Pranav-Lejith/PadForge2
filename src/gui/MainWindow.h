#pragma once

#include "Encoding.h"
#include "Macro.h"
#include "PluginHost.h"
#include "Session.h"
#include "Settings.h"

#include <QCryptographicHash>
#include <QList>
#include <QMainWindow>
#include <QTimer>

class QAction;
class QDockWidget;
class QLabel;
class QMenu;
class QSplitter;
class QTabWidget;

namespace PadForge {

class EditorWidget;
class FindInFilesPanel;
class FindReplaceDialog;
class PreferencesDialog;
class ColumnEditorDialog;
class GotoLineDialog;
class DocumentSummaryDialog;
class RunDialog;
class ExplorerPanel;
class DocumentMap;
class FloatingWindow;
class ConsolePanel;
class FunctionListPanel;
class CharacterMapPanel;
class SearchResultsPanel;

class MainWindow : public QMainWindow, public PluginContext {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    EditorWidget* openPath(const QString& path, bool switchTo = true);
    void openPaths(const QStringList& paths);
    EditorWidget* newDocument(const QString& text = QString(), const QString& path = QString());

    // PluginContext
    void insertText(const QString& text) override;
    void setDocumentText(const QString& text) override;
    QString documentText() const override;
    int lineCount() const override;
    int currentLine() const override;
    void gotoLine(int oneBasedLine) override;
    void setLanguage(const QString& lexerId) override;
    void showMessage(const QString& message) override;
    void addMenuItem(const QString& name, std::function<void()> callback) override;

    // Called by FloatingWindow to move a detached editor back to the main tabs.
    void reattachToMain(EditorWidget* editor);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    // Construction helpers
    void createActions();
    void createMenus();
    void createToolbar();
    void createStatusBar();
    void createDocks();
    void createPluginMenu();

    // Documents
    EditorWidget* createEditor();
    EditorWidget* currentEditor() const;
    void addEditor(EditorWidget* editor, QTabWidget* view);
    void registerEditor(EditorWidget* editor);
    void closeEditor(EditorWidget* editor);
    void updateTabTitle(EditorWidget* editor);
    void updateWindowTitle();
    void updateStatus();
    void applyThemeToAll();

    // File actions
    void newDocumentDialog();
    void openFilesDialog();
    void saveCurrent();
    void saveCurrentAs();
    void saveCurrentCopyAs();
    void saveAll();
    void reloadCurrent();
    void setEncodingForCurrent(const EncodingInfo& encoding, bool reencode);
    void setEolForCurrent(Eol eol);

    // Search
    void showFindDialog();
    void findAllInOpen(bool currentOnly);
    void goToSearchResult(EditorWidget* editor, int line);
    void findNext(bool up);
    void replaceOne();
    void replaceAll();
    void countMatches();
    void markAll();
    void gotoResult(const QString& file, int line);

    // View
    void toggleSplitView(bool enabled);
    void moveToOtherView();
    void setTheme(const QString& id);
    void setWordWrap(bool enabled);
    void showPreferences();
    void showColumnEditor();
    void showGotoLineDialog();
    void showDocumentSummary();
    void showRunDialog();
    void toggleFullScreen();
    void toggleAlwaysOnTop();
    void setSplitOrientation(int orientation);
    void nextTab(bool forward);
    void selectTabByNumber(int number);
    void updateWindowMenu();
    void applySettingsToAll();
    void updateLanguageMenu();

    // Explorer / document map / floating windows
    void openFolderDialog();
    void updateDocumentMapSource();
    void updateFunctionListSource();
    void detachCurrentToFloating();
    void mergeFloatingWindows();

    // Split-view synchronized scrolling
    void syncScrollFrom(EditorWidget* source);
    EditorWidget* otherViewEditor(EditorWidget* editor) const;

    // Compare documents
    void compareDocuments();
    void startCompare(EditorWidget* first, EditorWidget* second);
    void clearCompare();
    void gotoNextDifference(bool forward);
    void moveEditorTo(EditorWidget* editor, QTabWidget* view);
    void ensureSplitView();

    // Backup and crash recovery
    void snapshotModifiedDocuments();
    void offerRestoreBackups();

    // Export and printing
    void exportCurrentHtml();
    void exportCurrentRtf();
    void printCurrent(bool preview);

    // Tools: conversions, digests, script console, read-only
    void applyTextConversion(const std::function<QString(const QString&)>& transform,
                             const QString& label);
    void showDigest(QCryptographicHash::Algorithm algorithm, const QString& name);
    void showScriptConsole();
    void toggleReadOnlyMode(bool readOnly);
    void copyFileName();
    void copyFilePath();

    // Session / recents / macros / plugins
    void loadSession();
    void saveSession();
    void updateRecentMenu();
    void addRecent(const QString& path);
    void loadMacros();
    void saveMacros();
    void loadPlugins();
    void startMacroRecording();
    void stopMacroRecording();
    void playMacro();
    void showAbout();

    QTabWidget* m_mainTabs = nullptr;
    QTabWidget* m_subTabs = nullptr;
    QSplitter* m_splitter = nullptr;
    QTabWidget* m_activeTabs = nullptr;
    QList<EditorWidget*> m_editors;

    FindReplaceDialog* m_findDialog = nullptr;
    FindInFilesPanel* m_findInFilesPanel = nullptr;
    QDockWidget* m_findDock = nullptr;
    ExplorerPanel* m_explorerPanel = nullptr;
    QDockWidget* m_explorerDock = nullptr;
    DocumentMap* m_documentMap = nullptr;
    QDockWidget* m_mapDock = nullptr;
    ConsolePanel* m_consolePanel = nullptr;
    QDockWidget* m_consoleDock = nullptr;
    FunctionListPanel* m_functionList = nullptr;
    QDockWidget* m_functionListDock = nullptr;
    CharacterMapPanel* m_characterMap = nullptr;
    QDockWidget* m_characterMapDock = nullptr;
    QList<FloatingWindow*> m_floatingWindows;

    QLabel* m_statusPosition = nullptr;
    QLabel* m_statusEncoding = nullptr;
    QLabel* m_statusEol = nullptr;
    QLabel* m_statusLanguage = nullptr;
    QLabel* m_statusReadOnly = nullptr;

    SearchResultsPanel* m_searchResultsPanel = nullptr;
    QDockWidget* m_searchResultsDock = nullptr;

    QTimer m_backupTimer;
    bool m_syncScroll = false;
    bool m_syncingScroll = false;
    QAction* m_actSyncScroll = nullptr;

    EditorWidget* m_compareA = nullptr;
    EditorWidget* m_compareB = nullptr;
    QList<int> m_compareLinesA;
    QList<int> m_compareLinesB;
    bool m_compareActive = false;
    bool m_syncScrollBeforeCompare = false;

    QMenu* m_recentMenu = nullptr;
    QMenu* m_languageMenu = nullptr;
    QMenu* m_encodingMenu = nullptr;
    QMenu* m_eolMenu = nullptr;
    QMenu* m_macroMenu = nullptr;
    QMenu* m_pluginsMenu = nullptr;
    QMenu* m_themeMenu = nullptr;
    QMenu* m_windowMenu = nullptr;
    QList<QAction*> m_languageActions;

    QAction* m_actWordWrap = nullptr;
    QAction* m_actSplitView = nullptr;
    QAction* m_actRecordMacro = nullptr;
    QAction* m_actShowLineNumbers = nullptr;
    QAction* m_actShowFoldMargin = nullptr;
    QAction* m_actShowBookmarkMargin = nullptr;
    QAction* m_actShowAllChars = nullptr;
    QAction* m_actShowEol = nullptr;
    QAction* m_actShowIndentGuides = nullptr;
    QAction* m_actHighlightLine = nullptr;
    QAction* m_actAlwaysOnTop = nullptr;
    QAction* m_actSplitHorizontal = nullptr;
    QAction* m_actSplitVertical = nullptr;
    QAction* m_actReadOnly = nullptr;

    RecentFiles m_recent{15};
    PluginHost* m_pluginHost = nullptr;
    MacroRecorder m_macroRecorder;
    QList<Macro> m_macros;
    QString m_themeId = QStringLiteral("light");
    AppSettings m_settings;
    int m_untitledCounter = 0;
    int m_pluginMenuCount = 0;
};

} // namespace PadForge
