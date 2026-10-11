#pragma once

#include "Encoding.h"
#include "SearchEngine.h"
#include "Theme.h"

#include <QWidget>

class ScintillaEdit;

namespace PadForge {

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

    void setLargeFile(bool large);
    bool isLargeFile() const { return m_largeFile; }

    void applyTheme(const Theme& theme);
    void setZoom(int points);

    void highlightMatches(const QString& term, const SearchOptions& options);
    void clearSearchIndicators();

    void applySavedState(int cursor, int anchor, int firstVisibleLine, const QList<int>& bookmarks);

public slots:
    void findNextFromCursor(const QString& term, const SearchOptions& options);
    void replaceCurrent(const QString& term, const QString& replacement, const SearchOptions& options);

signals:
    void modificationChanged(bool dirty);
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

    ScintillaEdit* m_editor = nullptr;
    QString m_filePath;
    EncodingInfo m_encoding{Charset::Utf8, false};
    Eol m_eol = Eol::Lf;
    QString m_languageId = QStringLiteral("text");
    bool m_largeFile = false;
    bool m_updating = false;
};

} // namespace PadForge
