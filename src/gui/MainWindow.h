#pragma once

#include "Encoding.h"
#include "Macro.h"
#include "PluginHost.h"
#include "Session.h"
#include "Settings.h"

#include <QList>
#include <QMainWindow>

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
    void saveAll();
    void reloadCurrent();
    void setEncodingForCurrent(const EncodingInfo& encoding, bool reencode);
    void setEolForCurrent(Eol eol);

    // Search
    void showFindDialog();
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
    void detachCurrentToFloating();
    void mergeFloatingWindows();

    // Export and printing
    void exportCurrentHtml();
    void exportCurrentRtf();
    void printCurrent(bool preview);

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
    QList<FloatingWindow*> m_floatingWindows;

    QLabel* m_statusPosition = nullptr;
    QLabel* m_statusEncoding = nullptr;
    QLabel* m_statusEol = nullptr;
    QLabel* m_statusLanguage = nullptr;

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
