#pragma once

#include "Encoding.h"
#include "SearchEngine.h"
#include "Settings.h"
#include "TextOps.h"
#include "Theme.h"

#include <QHash>
#include <QTimer>
#include <QWidget>
#include <functional>

class QTextDocument;
class ScintillaEdit;

namespace PadForge {

// Identifiers for the GUID formats offered by the Insert menu.
enum class GuidFormat {
    Lower,       // 0f8fad5b-d9cb-469f-a165-70867728950e
    Upper,       // 0F8FAD5B-D9CB-469F-A165-70867728950E
    BracesUpper, // {0F8FAD5B-D9CB-469F-A165-70867728950E}
    Plain,       // 0F8FAD5BD9CB469FA16570867728950E
    Compact,     // 0f8fad5bd9cb469fa16570867728950e
};

// One open document: a ScintillaEdit plus its file/encoding/EOL/language state.
class EditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit EditorWidget(QWidget* parent = nullptr);

    ScintillaEdit* editor() const { return m_editor; }

    bool openFile(const QString& path, QString* error = nullptr);
    bool saveFile(QString* error = nullptr);
    bool saveFileAs(const QString& path, QString* error = nullptr);

    QString filePath() const { return m_filePath; }
    void setFilePath(const QString& path);
    bool isUntitled() const { return m_filePath.isEmpty(); }
    bool isModified() const;

    EncodingInfo encoding() const { return m_encoding; }
    void setEncoding(const EncodingInfo& encoding, bool reencode = false);
    Eol eol() const { return m_eol; }
    void setEol(Eol eol);

    QString languageId() const { return m_languageId; }
    void setLanguage(const QString& id);
    void detectLanguage();

    QString documentText() const;
    void setDocumentText(const QString& text);

    // Returns the selected text, or the whole document when nothing is selected.
    QString selectedTextOrDocument() const;
    // Replaces the selection (or the whole document when empty) with
    // `transform(text)`.
    void transformSelectionOrDocument(const std::function<QString(const QString&)>& transform);

    int currentLine() const;   // 1-based
    int currentColumn() const; // 1-based
    int cursorPosition() const;
    void gotoLine(int oneBasedLine);

    void setReadOnly(bool readOnly);
    bool isReadOnly() const;

    QList<int> bookmarkLines() const;
    void toggleBookmark();
    void clearBookmarks();
    void nextBookmark(bool forward = true);
    void copyBookmarkedLines();
    void cutBookmarkedLines();
    void deleteBookmarkedLines();
    void pasteToBookmarkedLines();
    void invertBookmarks();

    void setLargeFile(bool large);
    bool isLargeFile() const { return m_largeFile; }

    void applyTheme(const Theme& theme);
    void setZoom(int points);

    // Export and printing. The returned document is owned by the caller.
    QString exportHtml() const;
    QString exportRtf() const;
    QTextDocument* printDocument() const;

    // Scintilla change-history markers (insertions / deletions indicators).
    void setChangeHistoryEnabled(bool enabled);

    // Insert operations for the Insert menu.
    void insertDateTime(const QString& format);
    void insertGuid(GuidFormat format);
    void insertTextAtCursor(const QString& text);

    void highlightMatches(const QString& term, const SearchOptions& options);
    void clearSearchIndicators();

    void applySavedState(int cursor, int anchor, int firstVisibleLine, const QList<int>& bookmarks);

    // Accessors used by the Document Map panel.
    QColor colorForLine(int zeroBasedLine) const;
    QColor documentBackground() const;
    int firstVisibleLine() const;
    int linesOnScreen() const;

    // Text transformations (operate on the current selection, or the current
    // line when there is no selection).
    void duplicateCurrentLine();
    void deleteCurrentLine();
    void moveLineUp();
    void moveLineDown();
    void joinSelectedLines();
    void splitSelectedLines();
    void sortLinesAscending();
    void sortLinesDescending();
    void sortLinesAscendingCaseInsensitive();
    void removeDuplicateLines();
    void removeConsecutiveDuplicateLines();
    void removeEmptyLines();
    void convertCaseUpper();
    void convertCaseLower();
    void convertCaseTitle();
    void convertCaseSentence();
    void trimTrailingWhitespace();
    void trimLeadingWhitespace();
    void trimWhitespace();
    void tabifySelection();
    void untabifySelection();
    void toggleLineComment();
    void toggleBlockComment();

    // Display and editing options (persisted across sessions).
    void applySettings(const AppSettings& settings);
    void setWordWrapEnabled(bool enabled);
    void setShowLineNumbers(bool shown);
    void setShowFoldMargin(bool shown);
    void setShowBookmarkMargin(bool shown);
    void setShowWhitespace(bool shown);
    void setShowEol(bool shown);
    void setShowIndentGuides(bool shown);
    void setEdgeColumn(int column);
    void setEditorFont(const QString& family, int size);
    void setTabWidth(int width);
    void setUseTabs(bool useTabs);
    void setHighlightCurrentLine(bool highlighted);
    void setAutoCloseBrackets(bool enabled);
    void setAutoCloseTags(bool enabled);
    void setCalltips(bool enabled);
    void applyZoomFromSettings();

    // Word completion.
    void triggerCompletion();
    void setWordCompletionEnabled(bool enabled);

public slots:
    void findNextFromCursor(const QString& term, const SearchOptions& options);
    void replaceCurrent(const QString& term, const QString& replacement,
                        const SearchOptions& options);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

signals:
    void modificationChanged(bool dirty);
    void textChanged();
    void cursorPositionChanged(int line, int column);
    void filePathChanged(const QString& path);
    void languageChanged(const QString& id);
    void encodingChanged();
    void eolChanged();
    void bookmarkChanged();

private:
    void configureEditor();
    void configureMargins();
    void configureFoldingAndIndent();
    void configureMultiCursor();
    void updateBracketMatch();
    void updateStatus();
    void applyLanguageToEditor();
    void emitDocumentChanged();
    void handleTagAutoClose();
    void showCalltip();
    void refreshSignatures();
    void scheduleOutlineRefresh();

    // Replaces the given character range with UTF-8 text and re-selects it.
    void replaceRange(int start, int end, const QString& text);
    // Returns the full-line range covering the selection (or the current line
    // when there is no selection) as {startLine, endLine}.
    QPair<int, int> selectedLineRange() const;
    // Applies `transform` to the selected full-line text.
    void applyToSelectedLines(std::function<QString(const QString&)> transform);

    struct StyledRun {
        int start = 0;
        int length = 0;
        int style = 0;
    };
    // Style-change runs covering [start, end) retrieved from Scintilla.
    QList<StyledRun> styledRuns(int start, int end) const;
    QColor colorForStyle(int style) const;
    bool boldForStyle(int style) const;
    bool italicForStyle(int style) const;

    ScintillaEdit* m_editor = nullptr;
    QString m_filePath;
    EncodingInfo m_encoding{Charset::Utf8, false};
    Eol m_eol = Eol::Lf;
    QString m_languageId = QStringLiteral("text");
    bool m_largeFile = false;
    bool m_updating = false;
    bool m_wordCompletionOn = true;
    bool m_autoCloseBrackets = true;
    bool m_autoCloseTags = true;
    bool m_calltips = true;
    int m_tabWidth = 4;
    QString m_fontFamily = QStringLiteral("Monospace");
    int m_fontSize = 11;
    QHash<QString, QString> m_signatures;
    QTimer m_outlineTimer;
};

} // namespace PadForge
