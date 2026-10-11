#include "EditorWidget.h"

#include "Encoding.h"
#include "LanguageManager.h"

#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QVBoxLayout>

#include <ILexer.h>
#include <Lexilla.h>
#include <Scintilla.h>
#include <ScintillaEdit.h>

namespace PadForge {

namespace {

constexpr qint64 kLargeFileBytes = 10 * 1024 * 1024;
constexpr qint64 kMaxFileBytes = 512 * 1024 * 1024;

constexpr int kBookmarkMarker = 0;
constexpr int kBookmarkMargin = 2;
constexpr int kFoldMargin = 1;
constexpr int kLineNumberMargin = 0;

constexpr int kSearchIndicator = 0;
constexpr int kSmartIndicator = 1;

int sciColor(const QColor& c)
{
    return c.red() | (c.green() << 8) | (c.blue() << 16);
}

int eolToSci(Eol eol)
{
    switch (eol) {
    case Eol::CrLf:
        return SC_EOL_CRLF;
    case Eol::Cr:
        return SC_EOL_CR;
    case Eol::Lf:
    case Eol::Mixed:
    case Eol::None:
        break;
    }
    return SC_EOL_LF;
}

QString normalizedLineEndings(const QString& text)
{
    QString out = text;
    out.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    out.replace(QChar('\r'), QChar('\n'));
    return out;
}

} // namespace

EditorWidget::EditorWidget(QWidget* parent)
    : QWidget(parent)
    , m_editor(new ScintillaEdit(this))
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_editor);

    configureEditor();
    configureMargins();
    configureFoldingAndIndent();
    configureMultiCursor();
    applyTheme(ThemeManager::current());

    connect(m_editor, &ScintillaEdit::savePointChanged, this, [this](bool dirty) {
        if (!m_updating)
            emit modificationChanged(dirty);
    });
    connect(m_editor, &ScintillaEdit::updateUi, this, [this](auto) {
        updateStatus();
        updateBracketMatch();
    });
    connect(m_editor, &ScintillaEdit::marginClicked, this, [this](auto pos, auto, int margin) {
        if (margin == kBookmarkMargin) {
            const int line = int(m_editor->send(SCI_LINEFROMPOSITION, uptr_t(pos)));
            if (m_editor->send(SCI_MARKERGET, uptr_t(line)) & (1 << kBookmarkMarker))
                m_editor->send(SCI_MARKERDELETE, uptr_t(line), kBookmarkMarker);
            else
                m_editor->send(SCI_MARKERADD, uptr_t(line), kBookmarkMarker);
            emit bookmarkChanged();
        } else if (margin == kFoldMargin) {
            const int line = int(m_editor->send(SCI_LINEFROMPOSITION, uptr_t(pos)));
            m_editor->send(SCI_TOGGLEFOLD, uptr_t(line));
        }
    });
    connect(m_editor, &ScintillaEdit::charAdded, this, [this](int ch) {
        if (m_updating)
            return;
        const sptr_t pos = m_editor->send(SCI_GETCURRENTPOS);
        if (ch == '\n' || ch == '\r') {
            const int line = int(m_editor->send(SCI_LINEFROMPOSITION, uptr_t(pos)));
            if (line > 0) {
                const int prev = line - 1;
                const int start = int(m_editor->send(SCI_POSITIONFROMLINE, uptr_t(prev)));
                const int end = int(m_editor->send(SCI_GETLINEENDPOSITION, uptr_t(prev)));
                const QString previous = QString::fromUtf8(m_editor->textRange(start, end));
                const QString trimmed = previous.trimmed();
                if (trimmed.endsWith(QLatin1Char('{')) || trimmed.endsWith(QLatin1Char('('))
                    || trimmed.endsWith(QLatin1Char(':'))) {
                    m_editor->insertText(pos, "    ");
                }
            }
        } else if (ch == '(' || ch == '[' || ch == '{') {
            const char close = ch == '(' ? ')' : (ch == '[' ? ']' : '}');
            m_editor->insertText(pos, QByteArray(1, close).constData());
            m_editor->send(SCI_GOTOPOS, uptr_t(pos));
        }
    });
}

void EditorWidget::configureEditor()
{
    m_editor->send(SCI_SETCODEPAGE, SC_CP_UTF8);
    m_editor->send(SCI_SETMARGINS, 3);
    m_editor->send(SCI_SETSCROLLWIDTH, 1);
    m_editor->send(SCI_SETSCROLLWIDTHTRACKING, 1);
    m_editor->send(SCI_SETEDGEMODE, EDGE_NONE);
    m_editor->send(SCI_SETVIEWWS, SCWS_INVISIBLE);
    m_editor->send(SCI_SETMULTIPLESELECTION, 1);
    m_editor->send(SCI_SETADDITIONALSELECTIONTYPING, 1);
    m_editor->send(SCI_SETMULTIPASTE, SC_MULTIPASTE_EACH);
    m_editor->send(SCI_SETRECTANGULARSELECTIONMODIFIER, SCMOD_ALT);
}

void EditorWidget::configureMargins()
{
    m_editor->send(SCI_SETMARGINTYPEN, kLineNumberMargin, SC_MARGIN_NUMBER);
    m_editor->send(SCI_SETMARGINWIDTHN, kLineNumberMargin, 48);

    m_editor->send(SCI_SETMARGINTYPEN, kFoldMargin, SC_MARGIN_SYMBOL);
    m_editor->send(SCI_SETMARGINWIDTHN, kFoldMargin, 14);
    m_editor->send(SCI_SETMARGINMASKN, kFoldMargin, SC_MASK_FOLDERS);
    m_editor->send(SCI_SETMARGINSENSITIVEN, kFoldMargin, 1);

    m_editor->send(SCI_SETMARGINTYPEN, kBookmarkMargin, SC_MARGIN_SYMBOL);
    m_editor->send(SCI_SETMARGINWIDTHN, kBookmarkMargin, 16);
    m_editor->send(SCI_SETMARGINMASKN, kBookmarkMargin, 1 << kBookmarkMarker);
    m_editor->send(SCI_SETMARGINSENSITIVEN, kBookmarkMargin, 1);

    m_editor->send(SCI_MARKERDEFINE, kBookmarkMarker, SC_MARK_BOOKMARK);
}

void EditorWidget::configureFoldingAndIndent()
{
    m_editor->send(SCI_SETTABWIDTH, 4);
    m_editor->send(SCI_SETUSETABS, 0);
    m_editor->send(SCI_SETINDENT, 4);
    m_editor->send(SCI_SETTABINDENTS, 1);
    m_editor->send(SCI_SETBACKSPACEUNINDENTS, 1);

    m_editor->send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED);
    m_editor->send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED);
    m_editor->send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER);
    m_editor->send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER);
    m_editor->send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE);
    m_editor->send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS);
    m_editor->send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS);

    m_editor->send(SCI_SETFOLDFLAGS, SC_FOLDFLAG_LINEAFTER_CONTRACTED);
    m_editor->send(SCI_SETAUTOMATICFOLD,
                   SC_AUTOMATICFOLD_SHOW | SC_AUTOMATICFOLD_CLICK | SC_AUTOMATICFOLD_CHANGE);
}

void EditorWidget::configureMultiCursor()
{
    // Enabled in configureEditor; nothing further required here.
}

void EditorWidget::applyLanguageToEditor()
{
    const Language* language = LanguageManager::byId(m_languageId);
    if (!language || language->lexer == "null") {
        m_editor->send(SCI_SETILEXER, 0, 0);
        m_editor->send(SCI_STYLECLEARALL);
        applyTheme(ThemeManager::current());
        return;
    }

    Scintilla::ILexer5* lexer = CreateLexer(language->lexer.toUtf8().constData());
    if (lexer) {
        m_editor->send(SCI_SETILEXER, 0, reinterpret_cast<sptr_t>(lexer));
        m_editor->send(SCI_SETPROPERTY, reinterpret_cast<uptr_t>("fold"),
                       reinterpret_cast<sptr_t>("1"));
        m_editor->send(SCI_SETPROPERTY, reinterpret_cast<uptr_t>("fold.comment"),
                       reinterpret_cast<sptr_t>("1"));
        m_editor->send(SCI_SETPROPERTY, reinterpret_cast<uptr_t>("fold.compact"),
                       reinterpret_cast<sptr_t>("1"));
        const QByteArray keywords = language->keywords.toUtf8();
        m_editor->send(SCI_SETKEYWORDS, 0, reinterpret_cast<sptr_t>(keywords.constData()));
    }
    applyTheme(ThemeManager::current());
    m_editor->send(SCI_COLOURISE, 0, -1);
}

void EditorWidget::applyTheme(const Theme& theme)
{
    const QString lexer = (LanguageManager::byId(m_languageId) ? LanguageManager::byId(m_languageId)->lexer
                                                               : QStringLiteral("null"));
    m_editor->styleSetFont(STYLE_DEFAULT, "Monospace");
    m_editor->styleSetSize(STYLE_DEFAULT, 11);
    m_editor->styleSetFore(STYLE_DEFAULT, sciColor(theme.foreground));
    m_editor->styleSetBack(STYLE_DEFAULT, sciColor(theme.background));
    m_editor->send(SCI_STYLECLEARALL);

    for (int style = 0; style <= STYLE_MAX; ++style) {
        if (style == STYLE_LINENUMBER || style == STYLE_DEFAULT)
            continue;
        m_editor->styleSetFore(style, sciColor(theme.colorFor(lexer, style)));
        m_editor->styleSetBack(style, sciColor(theme.background));
        m_editor->styleSetBold(style, theme.boldFor(lexer, style));
        m_editor->styleSetItalic(style, theme.italicFor(lexer, style));
    }

    m_editor->styleSetFore(STYLE_LINENUMBER, sciColor(theme.lineNumberFg));
    m_editor->styleSetBack(STYLE_LINENUMBER, sciColor(theme.lineNumberBg));
    for (int margin = 0; margin < 3; ++margin)
        m_editor->send(SCI_SETMARGINBACKN, uptr_t(margin), sciColor(theme.lineNumberBg));

    m_editor->send(SCI_SETCARETFORE, sciColor(theme.caret));
    m_editor->send(SCI_SETSELBACK, 1, sciColor(theme.selectionBg));
    m_editor->send(SCI_SETSELFORE, 1, sciColor(theme.selectionFg));
    m_editor->send(SCI_SETCARETLINEVISIBLE, 1);
    m_editor->send(SCI_SETCARETLINEBACK, sciColor(theme.currentLine));
    m_editor->send(SCI_SETEDGECOLOUR, sciColor(theme.edgeLine));

    for (int marker = SC_MARKNUM_FOLDEREND; marker <= SC_MARKNUM_FOLDEROPEN; ++marker) {
        m_editor->send(SCI_MARKERSETFORE, uptr_t(marker), sciColor(theme.foldFg));
        m_editor->send(SCI_MARKERSETBACK, uptr_t(marker), sciColor(theme.lineNumberBg));
    }
    m_editor->send(SCI_MARKERSETFORE, kBookmarkMarker, sciColor(theme.bookmarkFg));

    m_editor->send(SCI_INDICSETSTYLE, kSearchIndicator, INDIC_ROUNDBOX);
    m_editor->send(SCI_INDICSETFORE, kSearchIndicator, sciColor(theme.dark ? QColor("#61afef")
                                                                           : QColor("#3399ff")));
    m_editor->send(SCI_INDICSETSTYLE, kSmartIndicator, INDIC_ROUNDBOX);
    m_editor->send(SCI_INDICSETFORE, kSmartIndicator, sciColor(QColor("#ffd24a")));

    m_editor->send(SCI_COLOURISE, 0, -1);
}

void EditorWidget::setZoom(int points)
{
    m_editor->send(SCI_SETZOOM, uptr_t(points));
}

QString EditorWidget::documentText() const
{
    const int length = int(m_editor->send(SCI_GETLENGTH));
    QByteArray buffer(length + 1, '\0');
    m_editor->send(SCI_GETTEXT, uptr_t(length + 1), reinterpret_cast<sptr_t>(buffer.data()));
    buffer.resize(length);
    return QString::fromUtf8(buffer);
}

void EditorWidget::setDocumentText(const QString& text)
{
    const QByteArray utf8 = normalizedLineEndings(text).toUtf8();
    m_editor->send(SCI_SETTEXT, 0, reinterpret_cast<sptr_t>(utf8.constData()));
}

bool EditorWidget::openFile(const QString& path, QString* error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = file.errorString();
        return false;
    }
    if (file.size() > kMaxFileBytes) {
        if (error)
            *error = tr("File is larger than the %1 MB limit").arg(kMaxFileBytes / (1024 * 1024));
        return false;
    }

    const QByteArray raw = file.readAll();
    const DetectionResult detection = EncodingDetector::detect(raw.left(65536));
    m_encoding = detection.encoding;
    m_eol = (detection.eol == Eol::None || detection.eol == Eol::Mixed) ? Eol::Lf : detection.eol;

    m_updating = true;
    setDocumentText(EncodingDetector::decode(raw, m_encoding));
    m_editor->send(SCI_SETEOLMODE, uptr_t(eolToSci(m_eol)));
    m_filePath = path;
    detectLanguage();
    m_editor->send(SCI_EMPTYUNDOBUFFER);
    m_editor->send(SCI_SETSAVEPOINT);
    setLargeFile(raw.size() > kLargeFileBytes);
    m_updating = false;

    emit filePathChanged(m_filePath);
    emit encodingChanged();
    emit eolChanged();
    emit modificationChanged(false);
    updateStatus();
    return true;
}

bool EditorWidget::saveFile(QString* error)
{
    if (m_filePath.isEmpty()) {
        if (error)
            *error = tr("There is no file path to save to");
        return false;
    }
    return saveFileAs(m_filePath, error);
}

bool EditorWidget::saveFileAs(const QString& path, QString* error)
{
    QString text = normalizedLineEndings(documentText());
    if (m_eol == Eol::CrLf)
        text.replace(QChar('\n'), QStringLiteral("\r\n"));
    else if (m_eol == Eol::Cr)
        text.replace(QChar('\n'), QChar('\r'));

    const QByteArray raw = EncodingDetector::encode(text, m_encoding);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error)
            *error = file.errorString();
        return false;
    }
    if (file.write(raw) != raw.size()) {
        if (error)
            *error = file.errorString();
        return false;
    }
    file.close();
    m_filePath = path;
    m_editor->send(SCI_SETSAVEPOINT);
    emit filePathChanged(m_filePath);
    emit modificationChanged(false);
    return true;
}

bool EditorWidget::isModified() const
{
    return m_editor->send(SCI_GETMODIFY) != 0;
}

void EditorWidget::setFilePath(const QString& path)
{
    m_filePath = path;
    detectLanguage();
    emit filePathChanged(m_filePath);
}

void EditorWidget::setEncoding(const EncodingInfo& encoding, bool reencode)
{
    m_encoding = encoding;
    if (reencode && !m_filePath.isEmpty()) {
        QFile file(m_filePath);
        if (file.open(QIODevice::ReadOnly)) {
            const QByteArray raw = file.readAll();
            m_updating = true;
            setDocumentText(EncodingDetector::decode(raw, m_encoding));
            m_updating = false;
        }
    }
    emit encodingChanged();
}

void EditorWidget::setEol(Eol eol)
{
    m_eol = (eol == Eol::Mixed || eol == Eol::None) ? Eol::Lf : eol;
    const QString current = documentText();
    if (m_eol == Eol::CrLf)
        m_editor->send(SCI_CONVERTEOLS, uptr_t(SC_EOL_CRLF));
    else if (m_eol == Eol::Cr)
        m_editor->send(SCI_CONVERTEOLS, uptr_t(SC_EOL_CR));
    else
        m_editor->send(SCI_CONVERTEOLS, uptr_t(SC_EOL_LF));
    m_editor->send(SCI_SETEOLMODE, uptr_t(eolToSci(m_eol)));
    Q_UNUSED(current);
    emit eolChanged();
    emit modificationChanged(isModified());
}

void EditorWidget::setLanguage(const QString& id)
{
    m_languageId = id;
    applyLanguageToEditor();
    emit languageChanged(m_languageId);
}

void EditorWidget::detectLanguage()
{
    const QString firstLine = documentText().section(QLatin1Char('\n'), 0, 0);
    const Language* language = LanguageManager::forFile(m_filePath, firstLine);
    const QString id = language ? language->id : QStringLiteral("text");
    if (id != m_languageId || m_languageId == "text") {
        m_languageId = id;
        applyLanguageToEditor();
        emit languageChanged(m_languageId);
    }
}

int EditorWidget::currentLine() const
{
    const sptr_t pos = m_editor->send(SCI_GETCURRENTPOS);
    return int(m_editor->send(SCI_LINEFROMPOSITION, uptr_t(pos))) + 1;
}

int EditorWidget::currentColumn() const
{
    const sptr_t pos = m_editor->send(SCI_GETCURRENTPOS);
    return int(m_editor->send(SCI_GETCOLUMN, uptr_t(pos))) + 1;
}

int EditorWidget::cursorPosition() const
{
    return int(m_editor->send(SCI_GETCURRENTPOS));
}

void EditorWidget::gotoLine(int oneBasedLine)
{
    if (oneBasedLine < 1)
        oneBasedLine = 1;
    m_editor->send(SCI_GOTOLINE, uptr_t(oneBasedLine - 1));
}

void EditorWidget::setReadOnly(bool readOnly)
{
    m_editor->send(SCI_SETREADONLY, readOnly ? 1 : 0);
}

bool EditorWidget::isReadOnly() const
{
    return m_editor->send(SCI_GETREADONLY) != 0;
}

QList<int> EditorWidget::bookmarkLines() const
{
    QList<int> lines;
    const uptr_t mask = 1u << kBookmarkMarker;
    int line = int(m_editor->send(SCI_MARKERNEXT, 0, sptr_t(mask)));
    while (line >= 0) {
        lines.append(line);
        line = int(m_editor->send(SCI_MARKERNEXT, uptr_t(line + 1), sptr_t(mask)));
    }
    return lines;
}

void EditorWidget::toggleBookmark()
{
    const int line = currentLine() - 1;
    if (m_editor->send(SCI_MARKERGET, uptr_t(line)) & (1 << kBookmarkMarker))
        m_editor->send(SCI_MARKERDELETE, uptr_t(line), kBookmarkMarker);
    else
        m_editor->send(SCI_MARKERADD, uptr_t(line), kBookmarkMarker);
    emit bookmarkChanged();
}

void EditorWidget::clearBookmarks()
{
    m_editor->send(SCI_MARKERDELETEALL, uptr_t(kBookmarkMarker));
    emit bookmarkChanged();
}

void EditorWidget::nextBookmark(bool forward)
{
    const uptr_t mask = 1u << kBookmarkMarker;
    const int line = currentLine() - 1;
    int target = -1;
    if (forward)
        target = int(m_editor->send(SCI_MARKERNEXT, uptr_t(line + 1), sptr_t(mask)));
    else
        target = int(m_editor->send(SCI_MARKERPREVIOUS, uptr_t(line - 1), sptr_t(mask)));
    if (target < 0)
        target = forward ? int(m_editor->send(SCI_MARKERNEXT, 0, sptr_t(mask)))
                         : int(m_editor->send(SCI_MARKERPREVIOUS,
                                              uptr_t(m_editor->send(SCI_GETLINECOUNT)),
                                              sptr_t(mask)));
    if (target >= 0)
        m_editor->send(SCI_GOTOLINE, uptr_t(target));
}

void EditorWidget::setLargeFile(bool large)
{
    m_largeFile = large;
    if (large) {
        m_editor->send(SCI_SETAUTOMATICFOLD, 0);
        m_editor->send(SCI_SETMARGINWIDTHN, kFoldMargin, 0);
        m_editor->send(SCI_SETCARETLINEVISIBLE, 0);
    } else {
        m_editor->send(SCI_SETMARGINWIDTHN, kFoldMargin, 14);
        m_editor->send(SCI_SETCARETLINEVISIBLE, 1);
        configureFoldingAndIndent();
    }
}

void EditorWidget::highlightMatches(const QString& term, const SearchOptions& options)
{
    clearSearchIndicators();
    if (term.isEmpty())
        return;
    m_editor->send(SCI_SETINDICATORCURRENT, kSearchIndicator);
    const QList<SearchMatch> matches = SearchEngine::findAll(documentText(), term, options, 10000);
    for (const SearchMatch& match : matches) {
        m_editor->send(SCI_INDICATORFILLRANGE, uptr_t(match.start), sptr_t(match.length));
    }
}

void EditorWidget::clearSearchIndicators()
{
    m_editor->send(SCI_SETINDICATORCURRENT, kSearchIndicator);
    const int length = int(m_editor->send(SCI_GETLENGTH));
    m_editor->send(SCI_INDICATORCLEARRANGE, 0, length);
}

void EditorWidget::findNextFromCursor(const QString& term, const SearchOptions& options)
{
    if (term.isEmpty())
        return;
    const int from = options.searchUp ? int(m_editor->send(SCI_GETSELECTIONSTART))
                                      : int(m_editor->send(SCI_GETSELECTIONEND));
    const SearchMatch match = SearchEngine::findNext(documentText(), term, from, options);
    if (!match.isValid())
        return;
    m_editor->send(SCI_SETSEL, uptr_t(match.start), sptr_t(match.start + match.length));
    m_editor->send(SCI_SCROLLCARET);
}

void EditorWidget::replaceCurrent(const QString& term, const QString& replacement,
                                  const SearchOptions& options)
{
    if (term.isEmpty())
        return;
    const int from = int(m_editor->send(SCI_GETSELECTIONSTART));
    const SearchMatch match = SearchEngine::findNext(documentText(), term, from, options);
    if (!match.isValid())
        return;
    m_editor->send(SCI_SETSEL, uptr_t(match.start), sptr_t(match.start + match.length));
    const QByteArray utf8 = replacement.toUtf8();
    m_editor->send(SCI_REPLACESEL, 0, reinterpret_cast<sptr_t>(utf8.constData()));
}

void EditorWidget::applySavedState(int cursor, int anchor, int firstVisibleLine,
                                   const QList<int>& bookmarks)
{
    m_editor->send(SCI_SETSEL, uptr_t(anchor), sptr_t(cursor));
    m_editor->send(SCI_SETFIRSTVISIBLELINE, uptr_t(firstVisibleLine));
    for (int line : bookmarks)
        m_editor->send(SCI_MARKERADD, uptr_t(line), kBookmarkMarker);
    m_editor->send(SCI_SETSAVEPOINT);
}

void EditorWidget::updateBracketMatch()
{
    if (m_largeFile)
        return;
    const sptr_t pos = m_editor->send(SCI_GETCURRENTPOS);
    const sptr_t brace = m_editor->send(SCI_BRACEMATCH, uptr_t(pos), 0);
    if (brace >= 0) {
        m_editor->send(SCI_BRACEHIGHLIGHT, uptr_t(brace), sptr_t(pos));
    } else {
        m_editor->send(SCI_BRACEBADLIGHT, uptr_t(pos));
    }
}

void EditorWidget::updateStatus()
{
    emit cursorPositionChanged(currentLine(), currentColumn());
}

void EditorWidget::emitDocumentChanged()
{
    emit modificationChanged(isModified());
}

} // namespace PadForge
