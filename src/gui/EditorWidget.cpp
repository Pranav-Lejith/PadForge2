#include "EditorWidget.h"

#include "Encoding.h"
#include "LanguageManager.h"
#include "Outline.h"

#include <ILexer.h>
#include <Lexilla.h>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QMenu>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QString>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QUuid>
#include <QVBoxLayout>
#include <Scintilla.h>
#include <ScintillaEdit.h>
#include <algorithm>

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
constexpr int kCompareRemovedIndicator = 2;
constexpr int kCompareAddedIndicator = 3;

int sciColor(const QColor& c) {
    return c.red() | (c.green() << 8) | (c.blue() << 16);
}

int eolToSci(Eol eol) {
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

QString normalizedLineEndings(const QString& text) {
    QString out = text;
    out.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    out.replace(QChar('\r'), QChar('\n'));
    return out;
}

QString escapeRtf(const QString& text) {
    QString out;
    out.reserve(text.size());
    for (const QChar c : text) {
        const ushort u = c.unicode();
        if (c == QLatin1Char('\\')) {
            out += QStringLiteral("\\\\");
        } else if (c == QLatin1Char('{')) {
            out += QStringLiteral("\\{");
        } else if (c == QLatin1Char('}')) {
            out += QStringLiteral("\\}");
        } else if (u < 0x80) {
            out += c;
        } else {
            out += QStringLiteral("\\u%1?").arg(static_cast<short>(u));
        }
    }
    return out;
}

} // namespace

EditorWidget::EditorWidget(QWidget* parent) : QWidget(parent), m_editor(new ScintillaEdit(this)) {
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
    connect(m_editor, &ScintillaEdit::modified, this, [this]() {
        if (m_updating)
            return;
        emit textChanged();
        scheduleOutlineRefresh();
    });
    connect(m_editor, &ScintillaEdit::updateUi, this, [this](auto) {
        updateStatus();
        updateBracketMatch();
        const int first = int(m_editor->firstVisibleLine());
        if (first != m_lastFirstVisibleLine) {
            m_lastFirstVisibleLine = first;
            emit scrolled();
        }
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
                if (trimmed.endsWith(QLatin1Char('{')) || trimmed.endsWith(QLatin1Char('(')) ||
                    trimmed.endsWith(QLatin1Char(':'))) {
                    m_editor->insertText(pos, "    ");
                }
            }
        } else if (ch == '(' || ch == '[' || ch == '{') {
            if (m_autoCloseBrackets) {
                const char close = ch == '(' ? ')' : (ch == '[' ? ']' : '}');
                m_editor->insertText(pos, QByteArray(1, close).constData());
                m_editor->send(SCI_GOTOPOS, uptr_t(pos));
            }
            if (ch == '(' && m_calltips)
                showCalltip();
        } else if (ch == ')') {
            m_editor->send(SCI_CALLTIPCANCEL);
        } else if (ch == '>') {
            handleTagAutoClose();
        }
    });

    m_outlineTimer.setSingleShot(true);
    m_outlineTimer.setInterval(400);
    connect(&m_outlineTimer, &QTimer::timeout, this, &EditorWidget::refreshSignatures);
}

void EditorWidget::configureEditor() {
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
    m_editor->send(SCI_SETCHANGEHISTORY, SC_CHANGE_HISTORY_INDICATORS);
}

void EditorWidget::configureMargins() {
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

void EditorWidget::configureFoldingAndIndent() {
    m_editor->send(SCI_SETTABWIDTH, uptr_t(m_tabWidth));
    m_editor->send(SCI_SETUSETABS, 0);
    m_editor->send(SCI_SETINDENT, uptr_t(m_tabWidth));
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

void EditorWidget::configureMultiCursor() {
    // Enabled in configureEditor; nothing further required here.
}

void EditorWidget::applyLanguageToEditor() {
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

void EditorWidget::applyTheme(const Theme& theme) {
    const QString lexer =
        (LanguageManager::byId(m_languageId) ? LanguageManager::byId(m_languageId)->lexer
                                             : QStringLiteral("null"));
    m_editor->styleSetFont(STYLE_DEFAULT, m_fontFamily.toUtf8().constData());
    m_editor->styleSetSize(STYLE_DEFAULT, m_fontSize);
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
    m_editor->send(SCI_INDICSETFORE, kSearchIndicator,
                   sciColor(theme.dark ? QColor("#61afef") : QColor("#3399ff")));
    m_editor->send(SCI_INDICSETSTYLE, kSmartIndicator, INDIC_ROUNDBOX);
    m_editor->send(SCI_INDICSETFORE, kSmartIndicator, sciColor(QColor("#ffd24a")));

    // Compare highlighting: translucent fill behind removed/added lines.
    m_editor->send(SCI_INDICSETSTYLE, kCompareRemovedIndicator, INDIC_STRAIGHTBOX);
    m_editor->send(SCI_INDICSETFORE, kCompareRemovedIndicator,
                   sciColor(theme.dark ? QColor("#8b2d2d") : QColor("#ff8a80")));
    m_editor->send(SCI_INDICSETALPHA, kCompareRemovedIndicator, 70);
    m_editor->send(SCI_INDICSETSTYLE, kCompareAddedIndicator, INDIC_STRAIGHTBOX);
    m_editor->send(SCI_INDICSETFORE, kCompareAddedIndicator,
                   sciColor(theme.dark ? QColor("#2d5f2d") : QColor("#80ffa0")));
    m_editor->send(SCI_INDICSETALPHA, kCompareAddedIndicator, 70);

    // Scintilla change-history indicators (36..43):
    // even numbers are insertions, odd numbers are deletions.
    for (int i = 0; i < 8; ++i) {
        const int indicator = 36 + i;
        m_editor->send(SCI_INDICSETSTYLE, uptr_t(indicator), INDIC_STRAIGHTBOX);
        m_editor->send(SCI_INDICSETFORE, uptr_t(indicator),
                       sciColor((i % 2 == 0) ? QColor("#43a047") : QColor("#e53935")));
    }

    m_editor->send(SCI_COLOURISE, 0, -1);
}

void EditorWidget::setZoom(int points) {
    m_editor->send(SCI_SETZOOM, uptr_t(points));
}

QColor EditorWidget::colorForLine(int zeroBasedLine) const {
    const int pos = int(m_editor->send(SCI_POSITIONFROMLINE, uptr_t(zeroBasedLine)));
    if (pos < 0)
        return documentBackground();
    const int style = int(m_editor->send(SCI_GETSTYLEAT, uptr_t(pos)));
    return colorForStyle(style);
}

QColor EditorWidget::documentBackground() const {
    return ThemeManager::current().background;
}

int EditorWidget::firstVisibleLine() const {
    return int(m_editor->send(SCI_GETFIRSTVISIBLELINE));
}

int EditorWidget::linesOnScreen() const {
    return int(m_editor->send(SCI_LINESONSCREEN));
}

QList<EditorWidget::StyledRun> EditorWidget::styledRuns(int start, int end) const {
    QList<StyledRun> runs;
    const int length = end - start;
    if (length <= 0)
        return runs;
    std::vector<char> buffer(2u * length + 2u);
    Sci_TextRange range;
    range.chrg.cpMin = start;
    range.chrg.cpMax = end;
    range.lpstrText = buffer.data();
    m_editor->send(SCI_GETSTYLEDTEXT, 0, reinterpret_cast<sptr_t>(&range));
    int currentStyle = -1;
    int runStart = 0;
    for (int i = 0; i < length; ++i) {
        const int style = static_cast<unsigned char>(buffer[2u * i + 1]) & 0xFF;
        if (style != currentStyle) {
            if (currentStyle >= 0)
                runs.append({start + runStart, i - runStart, currentStyle});
            currentStyle = style;
            runStart = i;
        }
    }
    if (currentStyle >= 0)
        runs.append({start + runStart, length - runStart, currentStyle});
    return runs;
}

QColor EditorWidget::colorForStyle(int style) const {
    const Language* language = LanguageManager::byId(m_languageId);
    const QString lexer = language ? language->lexer : QStringLiteral("null");
    return ThemeManager::current().colorFor(lexer, style);
}

bool EditorWidget::boldForStyle(int style) const {
    const Language* language = LanguageManager::byId(m_languageId);
    const QString lexer = language ? language->lexer : QStringLiteral("null");
    return ThemeManager::current().boldFor(lexer, style);
}

bool EditorWidget::italicForStyle(int style) const {
    const Language* language = LanguageManager::byId(m_languageId);
    const QString lexer = language ? language->lexer : QStringLiteral("null");
    return ThemeManager::current().italicFor(lexer, style);
}

void EditorWidget::setChangeHistoryEnabled(bool enabled) {
    m_editor->send(SCI_SETCHANGEHISTORY,
                   enabled ? SC_CHANGE_HISTORY_INDICATORS : SC_CHANGE_HISTORY_DISABLED);
}

void EditorWidget::insertDateTime(const QString& format) {
    const QString text = QDateTime::currentDateTime().toString(format);
    if (!text.isEmpty())
        insertTextAtCursor(text);
}

void EditorWidget::insertGuid(GuidFormat format) {
    const QUuid id = QUuid::createUuid();
    QString text = id.toString(QUuid::WithoutBraces);
    switch (format) {
    case GuidFormat::Lower:
        break;
    case GuidFormat::Upper:
        text = text.toUpper();
        break;
    case GuidFormat::BracesUpper:
        text = QLatin1Char('{') + text.toUpper() + QLatin1Char('}');
        break;
    case GuidFormat::Plain:
        text = text.toUpper().remove(QLatin1Char('-'));
        break;
    case GuidFormat::Compact:
        text = text.remove(QLatin1Char('-'));
        break;
    }
    insertTextAtCursor(text);
}

void EditorWidget::insertTextAtCursor(const QString& text) {
    const QByteArray utf8 = text.toUtf8();
    m_editor->send(SCI_REPLACESEL, 0, reinterpret_cast<sptr_t>(utf8.constData()));
}

QString EditorWidget::exportHtml() const {
    const QString title = m_filePath.isEmpty() ? tr("Untitled") : m_filePath;
    const Theme& theme = ThemeManager::current();
    QString html;
    html += QStringLiteral(
                "<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"utf-8\">\n<title>%1</title>\n"
                "<style>\nbody { background-color: %2; color: %3; font-family: %4, monospace; "
                "font-size: %5pt; }\npre { margin: 0; }\n</style>\n</head>\n<body>\n<pre>")
                .arg(title.toHtmlEscaped(), theme.background.name(), theme.foreground.name(),
                     m_fontFamily)
                .arg(m_fontSize);

    const int length = int(m_editor->send(SCI_GETLENGTH));
    const int defaultStyleColor = sciColor(theme.foreground);
    for (const StyledRun& run : styledRuns(0, length)) {
        const QColor color = colorForStyle(run.style);
        const bool bold = boldForStyle(run.style);
        const bool italic = italicForStyle(run.style);
        QString text = QString::fromUtf8(m_editor->textRange(run.start, run.start + run.length))
                           .toHtmlEscaped();
        text.replace(QLatin1Char('\n'), QStringLiteral("<br>\n"));
        QString css;
        if (sciColor(color) != defaultStyleColor)
            css += QStringLiteral("color:%1;").arg(color.name());
        if (bold)
            css += QStringLiteral("font-weight:bold;");
        if (italic)
            css += QStringLiteral("font-style:italic;");
        if (css.isEmpty()) {
            html += text;
        } else {
            css.chop(1); // strip trailing ';'
            html += QStringLiteral("<span style=\"%1\">%2</span>").arg(css, text);
        }
    }
    html += QStringLiteral("\n</pre>\n</body>\n</html>\n");
    return html;
}

QString EditorWidget::exportRtf() const {
    const int length = int(m_editor->send(SCI_GETLENGTH));
    const QList<StyledRun> runs = styledRuns(0, length);

    struct ColorEntry {
        QColor color;
        int index;
    };
    QList<ColorEntry> palette;
    auto colorIndex = [&palette](const QColor& color) {
        for (const ColorEntry& entry : palette) {
            if (entry.color == color)
                return entry.index;
        }
        palette.append({color, palette.size() + 1}); // index 0 is reserved for auto
        return palette.last().index;
    };
    for (const StyledRun& run : runs)
        colorIndex(colorForStyle(run.style));

    QString out;
    out += QStringLiteral("{\\rtf1\\ansi\\deff0");
    QString rtfFont = m_fontFamily;
    rtfFont.replace(QLatin1Char(' '), QStringLiteral("~"));
    out += QStringLiteral("{\\fonttbl{\\f0 %1;}}").arg(rtfFont);
    out += QStringLiteral("{\\colortbl");
    for (const ColorEntry& entry : palette) {
        out += QStringLiteral("\\red%1\\green%2\\blue%3;")
                   .arg(entry.color.red())
                   .arg(entry.color.green())
                   .arg(entry.color.blue());
    }
    out += QStringLiteral("}");

    const int defaultColorIndex = colorIndex(ThemeManager::current().foreground);
    out += QStringLiteral("\\f0\\fs%1").arg(m_fontSize * 2);

    for (const StyledRun& run : runs) {
        const int index = colorIndex(colorForStyle(run.style));
        const bool bold = boldForStyle(run.style);
        const bool italic = italicForStyle(run.style);
        QString text =
            escapeRtf(QString::fromUtf8(m_editor->textRange(run.start, run.start + run.length)));
        text.replace(QLatin1Char('\n'), QStringLiteral("\\par "));
        QString control = QStringLiteral(" ");
        if (index != defaultColorIndex)
            control += QStringLiteral("\\cf%1").arg(index);
        if (bold)
            control += QStringLiteral("\\b");
        if (italic)
            control += QStringLiteral("\\i");
        out += control + text;
        if (bold)
            out += QStringLiteral("\\b0");
        if (italic)
            out += QStringLiteral("\\i0");
    }
    out += QStringLiteral("\\par\n}");
    return out;
}

QTextDocument* EditorWidget::printDocument() const {
    auto* doc = new QTextDocument;
    QTextCursor cursor(doc);
    const int length = int(m_editor->send(SCI_GETLENGTH));
    const int defaultStyleColor = sciColor(ThemeManager::current().foreground);
    QTextCharFormat plain;
    plain.setFontFamily(m_fontFamily);
    plain.setFontPointSize(m_fontSize);
    for (const StyledRun& run : styledRuns(0, length)) {
        const QColor color = colorForStyle(run.style);
        const bool bold = boldForStyle(run.style);
        const bool italic = italicForStyle(run.style);
        if (sciColor(color) == defaultStyleColor && !bold && !italic) {
            cursor.insertText(m_editor->textRange(run.start, run.start + run.length), plain);
            continue;
        }
        QTextCharFormat format = plain;
        format.setForeground(color);
        if (bold)
            format.setFontWeight(QFont::Bold);
        if (italic)
            format.setFontItalic(true);
        cursor.insertText(m_editor->textRange(run.start, run.start + run.length), format);
    }
    return doc;
}

QString EditorWidget::documentText() const {
    const int length = int(m_editor->send(SCI_GETLENGTH));
    QByteArray buffer(length + 1, '\0');
    m_editor->send(SCI_GETTEXT, uptr_t(length + 1), reinterpret_cast<sptr_t>(buffer.data()));
    buffer.resize(length);
    return QString::fromUtf8(buffer);
}

void EditorWidget::setDocumentText(const QString& text) {
    const QByteArray utf8 = normalizedLineEndings(text).toUtf8();
    m_editor->send(SCI_SETTEXT, 0, reinterpret_cast<sptr_t>(utf8.constData()));
}

QString EditorWidget::selectedTextOrDocument() const {
    const int start = int(m_editor->send(SCI_GETSELECTIONSTART));
    const int end = int(m_editor->send(SCI_GETSELECTIONEND));
    if (start != end)
        return QString::fromUtf8(m_editor->textRange(start, end));
    return documentText();
}

void EditorWidget::transformSelectionOrDocument(
    const std::function<QString(const QString&)>& transform) {
    const int start = int(m_editor->send(SCI_GETSELECTIONSTART));
    const int end = int(m_editor->send(SCI_GETSELECTIONEND));
    if (start != end) {
        const QString selected = QString::fromUtf8(m_editor->textRange(start, end));
        replaceRange(start, end, transform(selected));
    } else {
        setDocumentText(transform(documentText()));
    }
}

bool EditorWidget::openFile(const QString& path, QString* error) {
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
    setReadOnly(!QFileInfo(path).isWritable());
    m_updating = false;

    emit filePathChanged(m_filePath);
    emit encodingChanged();
    emit eolChanged();
    emit modificationChanged(false);
    updateStatus();
    return true;
}

bool EditorWidget::saveFile(QString* error) {
    if (m_filePath.isEmpty()) {
        if (error)
            *error = tr("There is no file path to save to");
        return false;
    }
    return saveFileAs(m_filePath, error);
}

bool EditorWidget::saveFileAs(const QString& path, QString* error) {
    if (m_backupOnSave && QFileInfo::exists(path)) {
        const QString bak = path + QStringLiteral(".bak");
        QFile::remove(bak);
        QFile::copy(path, bak);
    }

    QString text = normalizedLineEndings(documentText());
    if (m_eol == Eol::CrLf)
        text.replace(QChar('\n'), QStringLiteral("\r\n"));
    else if (m_eol == Eol::Cr)
        text.replace(QChar('\n'), QChar('\r'));

    bool representable = true;
    const QByteArray raw = EncodingDetector::encode(text, m_encoding, &representable);
    if (!representable) {
        if (error)
            *error = tr("The document contains characters that cannot be saved as %1")
                         .arg(m_encoding.displayName());
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error)
            *error = file.errorString();
        return false;
    }
    if (file.write(raw) != raw.size()) {
        if (error)
            *error = file.errorString();
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        if (error)
            *error = file.errorString();
        return false;
    }
    m_filePath = path;
    setReadOnly(!QFileInfo(path).isWritable());
    m_editor->send(SCI_SETSAVEPOINT);
    emit filePathChanged(m_filePath);
    emit modificationChanged(false);
    return true;
}

bool EditorWidget::saveCopyAs(const QString& path, QString* error) {
    QString text = normalizedLineEndings(documentText());
    if (m_eol == Eol::CrLf)
        text.replace(QChar('\n'), QStringLiteral("\r\n"));
    else if (m_eol == Eol::Cr)
        text.replace(QChar('\n'), QChar('\r'));

    bool representable = true;
    const QByteArray raw = EncodingDetector::encode(text, m_encoding, &representable);
    if (!representable) {
        if (error)
            *error = tr("The document contains characters that cannot be saved as %1")
                         .arg(m_encoding.displayName());
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error)
            *error = file.errorString();
        return false;
    }
    if (file.write(raw) != raw.size()) {
        if (error)
            *error = file.errorString();
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        if (error)
            *error = file.errorString();
        return false;
    }
    return true;
}

bool EditorWidget::isModified() const {
    return m_editor->send(SCI_GETMODIFY) != 0;
}

void EditorWidget::setFilePath(const QString& path) {
    m_filePath = path;
    detectLanguage();
    emit filePathChanged(m_filePath);
}

void EditorWidget::setEncoding(const EncodingInfo& encoding, bool reencode) {
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

void EditorWidget::setEol(Eol eol) {
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

void EditorWidget::setLanguage(const QString& id) {
    m_languageId = id;
    applyLanguageToEditor();
    refreshSignatures();
    emit languageChanged(m_languageId);
}

void EditorWidget::detectLanguage() {
    const QString firstLine = documentText().section(QLatin1Char('\n'), 0, 0);
    const Language* language = LanguageManager::forFile(m_filePath, firstLine);
    const QString id = language ? language->id : QStringLiteral("text");
    if (id != m_languageId || m_languageId == "text") {
        m_languageId = id;
        applyLanguageToEditor();
        emit languageChanged(m_languageId);
    }
}

int EditorWidget::currentLine() const {
    const sptr_t pos = m_editor->send(SCI_GETCURRENTPOS);
    return int(m_editor->send(SCI_LINEFROMPOSITION, uptr_t(pos))) + 1;
}

int EditorWidget::currentColumn() const {
    const sptr_t pos = m_editor->send(SCI_GETCURRENTPOS);
    return int(m_editor->send(SCI_GETCOLUMN, uptr_t(pos))) + 1;
}

int EditorWidget::cursorPosition() const {
    return int(m_editor->send(SCI_GETCURRENTPOS));
}

void EditorWidget::gotoLine(int oneBasedLine) {
    if (oneBasedLine < 1)
        oneBasedLine = 1;
    m_editor->send(SCI_GOTOLINE, uptr_t(oneBasedLine - 1));
}

void EditorWidget::setReadOnly(bool readOnly) {
    m_editor->send(SCI_SETREADONLY, readOnly ? 1 : 0);
}

bool EditorWidget::isReadOnly() const {
    return m_editor->send(SCI_GETREADONLY) != 0;
}

QList<int> EditorWidget::bookmarkLines() const {
    QList<int> lines;
    const uptr_t mask = 1u << kBookmarkMarker;
    int line = int(m_editor->send(SCI_MARKERNEXT, 0, sptr_t(mask)));
    while (line >= 0) {
        lines.append(line);
        line = int(m_editor->send(SCI_MARKERNEXT, uptr_t(line + 1), sptr_t(mask)));
    }
    return lines;
}

void EditorWidget::toggleBookmark() {
    const int line = currentLine() - 1;
    if (m_editor->send(SCI_MARKERGET, uptr_t(line)) & (1 << kBookmarkMarker))
        m_editor->send(SCI_MARKERDELETE, uptr_t(line), kBookmarkMarker);
    else
        m_editor->send(SCI_MARKERADD, uptr_t(line), kBookmarkMarker);
    emit bookmarkChanged();
}

void EditorWidget::clearBookmarks() {
    m_editor->send(SCI_MARKERDELETEALL, uptr_t(kBookmarkMarker));
    emit bookmarkChanged();
}

void EditorWidget::nextBookmark(bool forward) {
    const uptr_t mask = 1u << kBookmarkMarker;
    const int line = currentLine() - 1;
    int target = -1;
    if (forward)
        target = int(m_editor->send(SCI_MARKERNEXT, uptr_t(line + 1), sptr_t(mask)));
    else
        target = int(m_editor->send(SCI_MARKERPREVIOUS, uptr_t(line - 1), sptr_t(mask)));
    if (target < 0)
        target = forward
                     ? int(m_editor->send(SCI_MARKERNEXT, 0, sptr_t(mask)))
                     : int(m_editor->send(SCI_MARKERPREVIOUS,
                                          uptr_t(m_editor->send(SCI_GETLINECOUNT)), sptr_t(mask)));
    if (target >= 0)
        m_editor->send(SCI_GOTOLINE, uptr_t(target));
}

void EditorWidget::setLargeFile(bool large) {
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

void EditorWidget::highlightMatches(const QString& term, const SearchOptions& options) {
    clearSearchIndicators();
    if (term.isEmpty())
        return;
    m_editor->send(SCI_SETINDICATORCURRENT, kSearchIndicator);
    const QList<SearchMatch> matches = SearchEngine::findAll(documentText(), term, options, 10000);
    for (const SearchMatch& match : matches) {
        m_editor->send(SCI_INDICATORFILLRANGE, uptr_t(match.start), sptr_t(match.length));
    }
}

void EditorWidget::clearSearchIndicators() {
    m_editor->send(SCI_SETINDICATORCURRENT, kSearchIndicator);
    const int length = int(m_editor->send(SCI_GETLENGTH));
    m_editor->send(SCI_INDICATORCLEARRANGE, 0, length);
}

namespace {
void fillCompareLine(ScintillaEdit* editor, int oneBasedLine, int indicator) {
    const int line = oneBasedLine - 1;
    const sptr_t start = editor->send(SCI_POSITIONFROMLINE, uptr_t(line));
    const sptr_t end = editor->send(SCI_POSITIONFROMLINE, uptr_t(line + 1));
    const sptr_t length = int(editor->send(SCI_GETLENGTH));
    if (start < 0 || end < start || start >= length)
        return;
    editor->send(SCI_SETINDICATORCURRENT, uptr_t(indicator));
    editor->send(SCI_INDICATORFILLRANGE, uptr_t(start), sptr_t(end - start));
}
} // namespace

void EditorWidget::setCompareMarkers(const QList<int>& removedLines, const QList<int>& addedLines) {
    clearCompareMarkers();
    for (int line : removedLines)
        fillCompareLine(m_editor, line, kCompareRemovedIndicator);
    for (int line : addedLines)
        fillCompareLine(m_editor, line, kCompareAddedIndicator);
}

void EditorWidget::clearCompareMarkers() {
    const int length = int(m_editor->send(SCI_GETLENGTH));
    m_editor->send(SCI_SETINDICATORCURRENT, kCompareRemovedIndicator);
    m_editor->send(SCI_INDICATORCLEARRANGE, 0, length);
    m_editor->send(SCI_SETINDICATORCURRENT, kCompareAddedIndicator);
    m_editor->send(SCI_INDICATORCLEARRANGE, 0, length);
}

void EditorWidget::findNextFromCursor(const QString& term, const SearchOptions& options) {
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
                                  const SearchOptions& options) {
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
                                   const QList<int>& bookmarks) {
    m_editor->send(SCI_SETSEL, uptr_t(anchor), sptr_t(cursor));
    m_editor->send(SCI_SETFIRSTVISIBLELINE, uptr_t(firstVisibleLine));
    for (int line : bookmarks)
        m_editor->send(SCI_MARKERADD, uptr_t(line), kBookmarkMarker);
    m_editor->send(SCI_SETSAVEPOINT);
}

void EditorWidget::updateBracketMatch() {
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

void EditorWidget::foldAll(bool fold) {
    if (m_largeFile)
        return;
    m_editor->send(SCI_FOLDALL, fold ? SC_FOLDACTION_CONTRACT : SC_FOLDACTION_EXPAND);
}

void EditorWidget::unfoldAll() {
    foldAll(false);
}

void EditorWidget::toggleAllFolds() {
    if (m_largeFile)
        return;
    m_editor->send(SCI_FOLDALL, SC_FOLDACTION_TOGGLE);
}

void EditorWidget::foldCurrent(bool fold) {
    if (m_largeFile)
        return;
    const sptr_t line =
        m_editor->send(SCI_LINEFROMPOSITION, uptr_t(m_editor->send(SCI_GETCURRENTPOS)));
    m_editor->send(SCI_FOLDLINE, uptr_t(line),
                   fold ? SC_FOLDACTION_CONTRACT : SC_FOLDACTION_EXPAND);
}

void EditorWidget::toggleCurrentFold() {
    if (m_largeFile)
        return;
    const sptr_t line =
        m_editor->send(SCI_LINEFROMPOSITION, uptr_t(m_editor->send(SCI_GETCURRENTPOS)));
    m_editor->send(SCI_TOGGLEFOLD, uptr_t(line));
}

bool EditorWidget::matchingBraces(int* source, int* match) const {
    const sptr_t pos = m_editor->send(SCI_GETCURRENTPOS);
    sptr_t start = pos;
    const int ch = int(m_editor->send(SCI_GETCHARAT, uptr_t(pos)));
    if (ch != '(' && ch != '[' && ch != '{' && ch != ')' && ch != ']' && ch != '}' && pos > 0)
        start = pos - 1;
    const sptr_t partner = m_editor->send(SCI_BRACEMATCH, uptr_t(start), 0);
    if (partner < 0)
        return false;
    if (source)
        *source = int(start);
    if (match)
        *match = int(partner);
    return true;
}

void EditorWidget::gotoMatchingBrace() {
    int source = 0;
    int match = 0;
    if (!matchingBraces(&source, &match))
        return;
    Q_UNUSED(source);
    m_editor->send(SCI_GOTOPOS, uptr_t(match));
    m_editor->send(SCI_SETSEL, uptr_t(match), sptr_t(match));
}

void EditorWidget::selectToMatchingBrace() {
    int source = 0;
    int match = 0;
    if (!matchingBraces(&source, &match))
        return;
    const int from = std::min(source, match);
    const int to = std::max(source, match) + 1;
    m_editor->send(SCI_SETSEL, uptr_t(from), sptr_t(to));
}

void EditorWidget::updateStatus() {
    emit cursorPositionChanged(currentLine(), currentColumn());
}

void EditorWidget::emitDocumentChanged() {
    emit modificationChanged(isModified());
}

void EditorWidget::replaceRange(int start, int end, const QString& text) {
    const QByteArray utf8 = text.toUtf8();
    m_editor->send(SCI_SETTARGETSTART, uptr_t(start));
    m_editor->send(SCI_SETTARGETEND, uptr_t(end));
    m_editor->send(SCI_REPLACETARGET, uptr_t(utf8.size()),
                   reinterpret_cast<sptr_t>(utf8.constData()));
    m_editor->send(SCI_SETSEL, uptr_t(start), sptr_t(start + utf8.size()));
}

QPair<int, int> EditorWidget::selectedLineRange() const {
    const int selStart = int(m_editor->send(SCI_GETSELECTIONSTART));
    const int selEnd = int(m_editor->send(SCI_GETSELECTIONEND));
    int startLine = int(m_editor->send(SCI_LINEFROMPOSITION, uptr_t(selStart)));
    int endLine = int(m_editor->send(SCI_LINEFROMPOSITION, uptr_t(selEnd)));
    if (selStart == selEnd) {
        endLine = startLine;
    } else if (selEnd == int(m_editor->send(SCI_POSITIONFROMLINE, uptr_t(endLine)))) {
        --endLine;
        if (endLine < startLine)
            endLine = startLine;
    }
    return {startLine, endLine};
}

void EditorWidget::applyToSelectedLines(std::function<QString(const QString&)> transform) {
    const auto range = selectedLineRange();
    const int start = int(m_editor->send(SCI_POSITIONFROMLINE, uptr_t(range.first)));
    int end = int(m_editor->send(SCI_POSITIONFROMLINE, uptr_t(range.second + 1)));
    if (end < 0)
        end = int(m_editor->send(SCI_GETLENGTH));
    const QString original = QString::fromUtf8(m_editor->textRange(start, end));
    m_editor->beginUndoAction();
    replaceRange(start, end, transform(original));
    m_editor->endUndoAction();
}

void EditorWidget::duplicateCurrentLine() {
    const auto range = selectedLineRange();
    const int start = int(m_editor->send(SCI_POSITIONFROMLINE, uptr_t(range.first)));
    int end = int(m_editor->send(SCI_POSITIONFROMLINE, uptr_t(range.second + 1)));
    if (end < 0)
        end = int(m_editor->send(SCI_GETLENGTH));
    const QString block = QString::fromUtf8(m_editor->textRange(start, end));
    m_editor->beginUndoAction();
    replaceRange(start, end, block + block);
    m_editor->endUndoAction();
}

void EditorWidget::deleteCurrentLine() {
    m_editor->beginUndoAction();
    m_editor->send(SCI_LINEDELETE);
    m_editor->endUndoAction();
}

void EditorWidget::moveLineUp() {
    m_editor->beginUndoAction();
    m_editor->send(SCI_MOVESELECTEDLINESUP);
    m_editor->endUndoAction();
}

void EditorWidget::moveLineDown() {
    m_editor->beginUndoAction();
    m_editor->send(SCI_MOVESELECTEDLINESDOWN);
    m_editor->endUndoAction();
}

void EditorWidget::joinSelectedLines() {
    applyToSelectedLines(&TextOps::joinLinesOperation);
}

void EditorWidget::splitSelectedLines() {
    applyToSelectedLines(&TextOps::splitLinesOperation);
}

void EditorWidget::sortLinesAscending() {
    applyToSelectedLines(
        [](const QString& text) { return TextOps::sortLines(text, true, true, false); });
}

void EditorWidget::sortLinesDescending() {
    applyToSelectedLines(
        [](const QString& text) { return TextOps::sortLines(text, false, true, false); });
}

void EditorWidget::sortLinesAscendingCaseInsensitive() {
    applyToSelectedLines(
        [](const QString& text) { return TextOps::sortLines(text, true, false, false); });
}

void EditorWidget::removeDuplicateLines() {
    applyToSelectedLines(&TextOps::removeDuplicateLines);
}

void EditorWidget::removeConsecutiveDuplicateLines() {
    applyToSelectedLines(&TextOps::removeConsecutiveDuplicateLines);
}

void EditorWidget::removeEmptyLines() {
    applyToSelectedLines(&TextOps::removeEmptyLines);
}

void EditorWidget::convertCaseUpper() {
    applyToSelectedLines(
        [](const QString& text) { return TextOps::convertCase(text, CaseMode::Upper); });
}

void EditorWidget::convertCaseLower() {
    applyToSelectedLines(
        [](const QString& text) { return TextOps::convertCase(text, CaseMode::Lower); });
}

void EditorWidget::convertCaseTitle() {
    applyToSelectedLines(
        [](const QString& text) { return TextOps::convertCase(text, CaseMode::Title); });
}

void EditorWidget::convertCaseSentence() {
    applyToSelectedLines(
        [](const QString& text) { return TextOps::convertCase(text, CaseMode::Sentence); });
}

void EditorWidget::convertCaseInvert() {
    applyToSelectedLines(
        [](const QString& text) { return TextOps::convertCase(text, CaseMode::Invert); });
}

void EditorWidget::trimTrailingWhitespace() {
    applyToSelectedLines(TextOps::trimTrailing);
}

void EditorWidget::trimLeadingWhitespace() {
    applyToSelectedLines(TextOps::trimLeading);
}

void EditorWidget::trimWhitespace() {
    applyToSelectedLines(TextOps::trimBoth);
}

void EditorWidget::tabifySelection() {
    applyToSelectedLines([this](const QString& text) { return TextOps::tabify(text, m_tabWidth); });
}

void EditorWidget::untabifySelection() {
    applyToSelectedLines(
        [this](const QString& text) { return TextOps::untabify(text, m_tabWidth); });
}

void EditorWidget::toggleLineComment() {
    const Language* language = LanguageManager::byId(m_languageId);
    if (!language || !language->hasLineComment())
        return;
    const QString token = language->lineComment;
    applyToSelectedLines(
        [&token](const QString& text) { return TextOps::toggleLineComment(text, token); });
}

void EditorWidget::toggleBlockComment() {
    const Language* language = LanguageManager::byId(m_languageId);
    if (!language || !language->hasBlockComment())
        return;
    const int selStart = int(m_editor->send(SCI_GETSELECTIONSTART));
    const int selEnd = int(m_editor->send(SCI_GETSELECTIONEND));
    const QString sub = QString::fromUtf8(m_editor->textRange(selStart, selEnd));
    const QString result =
        TextOps::toggleBlockComment(sub, language->blockCommentStart, language->blockCommentEnd);
    m_editor->beginUndoAction();
    replaceRange(selStart, selEnd, result);
    m_editor->endUndoAction();
}

void EditorWidget::copyBookmarkedLines() {
    const QList<int> lines = bookmarkLines();
    if (lines.isEmpty())
        return;
    QString text;
    for (int line : lines) {
        const int start = int(m_editor->send(SCI_POSITIONFROMLINE, uptr_t(line)));
        int end = int(m_editor->send(SCI_POSITIONFROMLINE, uptr_t(line + 1)));
        if (end < 0)
            end = int(m_editor->send(SCI_GETLENGTH));
        text += QString::fromUtf8(m_editor->textRange(start, end));
    }
    QApplication::clipboard()->setText(text);
}

void EditorWidget::cutBookmarkedLines() {
    copyBookmarkedLines();
    deleteBookmarkedLines();
}

void EditorWidget::deleteBookmarkedLines() {
    const QList<int> lines = bookmarkLines();
    if (lines.isEmpty())
        return;
    struct Range {
        int start;
        int end;
    };
    QList<Range> ranges;
    for (int line : lines) {
        const int start = int(m_editor->send(SCI_POSITIONFROMLINE, uptr_t(line)));
        int end = int(m_editor->send(SCI_POSITIONFROMLINE, uptr_t(line + 1)));
        if (end < 0)
            end = int(m_editor->send(SCI_GETLENGTH));
        ranges.append({start, end});
    }
    std::sort(ranges.begin(), ranges.end(),
              [](const Range& a, const Range& b) { return a.start > b.start; });
    m_editor->beginUndoAction();
    for (const Range& range : ranges)
        replaceRange(range.start, range.end, QString());
    m_editor->endUndoAction();
}

void EditorWidget::pasteToBookmarkedLines() {
    const QString text = QApplication::clipboard()->text();
    if (text.isEmpty())
        return;
    const QList<int> lines = bookmarkLines();
    if (lines.isEmpty())
        return;
    QList<int> sorted = lines;
    std::sort(sorted.begin(), sorted.end(), std::greater<int>());
    m_editor->beginUndoAction();
    for (int line : sorted) {
        const int start = int(m_editor->send(SCI_POSITIONFROMLINE, uptr_t(line)));
        replaceRange(start, start, text);
    }
    m_editor->endUndoAction();
}

void EditorWidget::invertBookmarks() {
    const uptr_t mask = 1u << kBookmarkMarker;
    const int lineCount = TextOps::lineCount(documentText());
    for (int line = 0; line < lineCount; ++line) {
        if (m_editor->send(SCI_MARKERGET, uptr_t(line)) & mask)
            m_editor->send(SCI_MARKERDELETE, uptr_t(line), kBookmarkMarker);
        else
            m_editor->send(SCI_MARKERADD, uptr_t(line), kBookmarkMarker);
    }
    emit bookmarkChanged();
}

void EditorWidget::applySettings(const AppSettings& settings) {
    setWordWrapEnabled(settings.wordWrap);
    setShowLineNumbers(settings.showLineNumbers);
    setShowFoldMargin(settings.showFoldMargin);
    setShowBookmarkMargin(settings.showBookmarkMargin);
    setShowWhitespace(settings.showWhitespace);
    setShowEol(settings.showEol);
    setShowIndentGuides(settings.showIndentGuides);
    setEdgeColumn(settings.edgeColumn);
    setEditorFont(settings.fontFamily, settings.fontSize);
    setTabWidth(settings.tabWidth);
    setUseTabs(settings.tabMode == QStringLiteral("tabs"));
    setHighlightCurrentLine(settings.highlightCurrentLine);
    setAutoCloseBrackets(settings.autoCloseBrackets);
    setAutoCloseTags(settings.autoCloseTags);
    setCalltips(settings.calltips);
    setBackupOnSave(settings.backupOnSave);
    setChangeHistoryEnabled(settings.showChangeHistory);
    m_wordCompletionOn = settings.autoCompletion;
}

void EditorWidget::setWordWrapEnabled(bool enabled) {
    m_editor->send(SCI_SETWRAPMODE, enabled ? SC_WRAP_WORD : SC_WRAP_NONE);
}

void EditorWidget::setShowLineNumbers(bool shown) {
    m_editor->send(SCI_SETMARGINWIDTHN, uptr_t(kLineNumberMargin), shown ? 48 : 0);
}

void EditorWidget::setShowFoldMargin(bool shown) {
    m_editor->send(SCI_SETMARGINWIDTHN, uptr_t(kFoldMargin), shown && !m_largeFile ? 14 : 0);
}

void EditorWidget::setShowBookmarkMargin(bool shown) {
    m_editor->send(SCI_SETMARGINWIDTHN, uptr_t(kBookmarkMargin), shown ? 16 : 0);
}

void EditorWidget::setShowWhitespace(bool shown) {
    m_editor->send(SCI_SETVIEWWS, shown ? SCWS_VISIBLEALWAYS : SCWS_INVISIBLE);
}

void EditorWidget::setShowEol(bool shown) {
    m_editor->send(SCI_SETVIEWEOL, shown ? 1 : 0);
}

void EditorWidget::setShowIndentGuides(bool shown) {
    m_editor->send(SCI_SETINDENTATIONGUIDES, shown ? SC_IV_REAL : SC_IV_NONE);
}

void EditorWidget::setEdgeColumn(int column) {
    if (column > 0) {
        m_editor->send(SCI_SETEDGECOLUMN, uptr_t(column));
        m_editor->send(SCI_SETEDGEMODE, EDGE_LINE);
    } else {
        m_editor->send(SCI_SETEDGEMODE, EDGE_NONE);
    }
}

void EditorWidget::setEditorFont(const QString& family, int size) {
    m_fontFamily = family;
    m_fontSize = size;
    applyTheme(ThemeManager::current());
}

void EditorWidget::setTabWidth(int width) {
    m_tabWidth = width;
    m_editor->send(SCI_SETTABWIDTH, uptr_t(width));
    m_editor->send(SCI_SETINDENT, uptr_t(width));
}

void EditorWidget::setUseTabs(bool useTabs) {
    m_editor->send(SCI_SETUSETABS, useTabs ? 1 : 0);
}

void EditorWidget::setHighlightCurrentLine(bool highlighted) {
    m_editor->send(SCI_SETCARETLINEVISIBLE, highlighted ? 1 : 0);
}

void EditorWidget::setAutoCloseBrackets(bool enabled) {
    m_autoCloseBrackets = enabled;
}

void EditorWidget::setAutoCloseTags(bool enabled) {
    m_autoCloseTags = enabled;
}

void EditorWidget::setCalltips(bool enabled) {
    m_calltips = enabled;
    if (enabled)
        scheduleOutlineRefresh();
    else
        m_editor->send(SCI_CALLTIPCANCEL);
}

void EditorWidget::setBackupOnSave(bool enabled) {
    m_backupOnSave = enabled;
}

void EditorWidget::setFirstVisibleLine(int line) {
    m_editor->send(SCI_SETFIRSTVISIBLELINE, uptr_t(line));
}

void EditorWidget::triggerSymbolCompletion() {
    const sptr_t pos = m_editor->send(SCI_GETCURRENTPOS);
    const sptr_t wordStart = m_editor->send(SCI_WORDSTARTPOSITION, uptr_t(pos), true);
    const int typedLength = int(pos - wordStart);
    const QString prefix = typedLength > 0
                               ? QString::fromUtf8(m_editor->textRange(int(wordStart), int(pos)))
                               : QString();

    QSet<QString> seen;
    if (OutlineParser::supports(m_languageId)) {
        const QList<OutlineItem> items = OutlineParser::parse(documentText(), m_languageId);
        for (const OutlineItem& item : items) {
            if (item.kind == QLatin1String("function") || item.kind == QLatin1String("class") ||
                item.kind == QLatin1String("struct") || item.kind == QLatin1String("interface")) {
                if (typedLength == 0 || item.name.startsWith(prefix, Qt::CaseInsensitive))
                    seen.insert(item.name);
            }
        }
    }

    if (seen.isEmpty()) {
        const QString text = documentText();
        const QRegularExpression wordRe(QStringLiteral("[A-Za-z_][A-Za-z0-9_]*"));
        auto it = wordRe.globalMatch(text);
        while (it.hasNext()) {
            const QString word = it.next().captured();
            if (word.size() >= 2 &&
                (typedLength == 0 || word.startsWith(prefix, Qt::CaseInsensitive)))
                seen.insert(word);
            if (seen.size() >= 500)
                break;
        }
    }
    if (seen.isEmpty())
        return;

    QStringList words(seen.begin(), seen.end());
    words.sort(Qt::CaseInsensitive);
    m_editor->send(SCI_AUTOCSETIGNORECASE, 1);
    m_editor->send(SCI_AUTOCSETSEPARATOR, uptr_t(' '));
    m_editor->send(SCI_AUTOCSETCHOOSESINGLE, 0);
    const QByteArray list = words.join(QStringLiteral(" ")).toUtf8();
    m_editor->send(SCI_AUTOCSHOW, uptr_t(typedLength), reinterpret_cast<sptr_t>(list.constData()));
}

void EditorWidget::handleTagAutoClose() {
    static const QSet<QString> tagLanguages = {
        QStringLiteral("html"),  QStringLiteral("htm"), QStringLiteral("xml"),
        QStringLiteral("xhtml"), QStringLiteral("php"), QStringLiteral("asp"),
        QStringLiteral("jsp"),   QStringLiteral("svg"), QStringLiteral("xsl"),
        QStringLiteral("vue"),
    };
    static const QSet<QString> voidElements = {
        QStringLiteral("area"),  QStringLiteral("base"),  QStringLiteral("br"),
        QStringLiteral("col"),   QStringLiteral("embed"), QStringLiteral("hr"),
        QStringLiteral("img"),   QStringLiteral("input"), QStringLiteral("link"),
        QStringLiteral("meta"),  QStringLiteral("param"), QStringLiteral("source"),
        QStringLiteral("track"), QStringLiteral("wbr"),
    };
    if (!m_autoCloseTags || !tagLanguages.contains(m_languageId.toLower()))
        return;

    const int pos = int(m_editor->send(SCI_GETCURRENTPOS));
    const int line = int(m_editor->send(SCI_LINEFROMPOSITION, uptr_t(pos)));
    const int lineStart = int(m_editor->send(SCI_POSITIONFROMLINE, uptr_t(line)));
    const QString before = QString::fromUtf8(m_editor->textRange(lineStart, pos));
    const int open = before.lastIndexOf(QLatin1Char('<'));
    if (open < 0)
        return;

    QString content = before.mid(open + 1);
    if (content.endsWith(QLatin1Char('>')))
        content.chop(1);
    if (content.isEmpty() || content.startsWith(QLatin1Char('/')) ||
        content.startsWith(QLatin1Char('!')) || content.startsWith(QLatin1Char('?')) ||
        content.endsWith(QLatin1Char('/')))
        return;

    static const QRegularExpression nameRe(QStringLiteral("^([A-Za-z][\\w:.-]*)"));
    const QRegularExpressionMatch match = nameRe.match(content);
    if (!match.hasMatch())
        return;
    const QString name = match.captured(1);
    if (voidElements.contains(name.toLower()))
        return;

    if (QString::fromUtf8(m_editor->textRange(pos, pos + 2)) == QLatin1String("</"))
        return;

    m_editor->insertText(pos, QStringLiteral("</%1>").arg(name).toUtf8());
    m_editor->send(SCI_GOTOPOS, uptr_t(pos));
}

void EditorWidget::showCalltip() {
    if (m_signatures.isEmpty())
        return;
    const int pos = int(m_editor->send(SCI_GETCURRENTPOS));
    int i = pos - 2; // character before the just-typed '('
    while (i >= 0) {
        const int ch = int(m_editor->send(SCI_GETCHARAT, uptr_t(i)));
        const bool word = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                          (ch >= '0' && ch <= '9') || ch == '_';
        if (!word)
            break;
        --i;
    }
    const QString name = QString::fromUtf8(m_editor->textRange(i + 1, pos - 1));
    const auto it = m_signatures.constFind(name);
    if (it == m_signatures.constEnd())
        return;
    const QByteArray tip = it.value().toUtf8();
    m_editor->send(SCI_CALLTIPSHOW, uptr_t(pos), reinterpret_cast<sptr_t>(tip.constData()));
}

void EditorWidget::refreshSignatures() {
    m_signatures.clear();
    if (!m_calltips || m_largeFile || !OutlineParser::supports(m_languageId))
        return;
    const QList<OutlineItem> items = OutlineParser::parse(documentText(), m_languageId);
    for (const OutlineItem& item : items) {
        if (item.kind == QLatin1String("function"))
            m_signatures.insert(item.name, item.signature);
    }
}

void EditorWidget::scheduleOutlineRefresh() {
    m_outlineTimer.start();
}

void EditorWidget::applyZoomFromSettings() {
    m_editor->send(SCI_SETZOOM, 0);
}

void EditorWidget::setWordCompletionEnabled(bool enabled) {
    m_wordCompletionOn = enabled;
}

void EditorWidget::triggerCompletion() {
    if (!m_wordCompletionOn)
        return;
    const sptr_t pos = m_editor->send(SCI_GETCURRENTPOS);
    const sptr_t wordStart = m_editor->send(SCI_WORDSTARTPOSITION, uptr_t(pos), true);
    const int typedLength = int(pos - wordStart);
    if (typedLength <= 0)
        return;

    const QString text = documentText();
    const QString prefix = text.mid(int(wordStart), typedLength);
    const QRegularExpression wordRe(QStringLiteral("[A-Za-z_][A-Za-z0-9_]*"));
    QSet<QString> seen;
    auto it = wordRe.globalMatch(text);
    while (it.hasNext()) {
        const QString word = it.next().captured();
        if (word.size() >= 3 && word.startsWith(prefix, Qt::CaseInsensitive))
            seen.insert(word);
        if (seen.size() >= 500)
            break;
    }
    QStringList words(seen.begin(), seen.end());
    words.sort(Qt::CaseInsensitive);
    if (words.isEmpty())
        return;

    m_editor->send(SCI_AUTOCSETIGNORECASE, 1);
    m_editor->send(SCI_AUTOCSETSEPARATOR, uptr_t(' '));
    m_editor->send(SCI_AUTOCSETCHOOSESINGLE, 0);
    const QByteArray list = words.join(QStringLiteral(" ")).toUtf8();
    m_editor->send(SCI_AUTOCSHOW, uptr_t(typedLength), reinterpret_cast<sptr_t>(list.constData()));
}

void EditorWidget::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);
    QAction* cut = menu.addAction(tr("Cut"));
    QAction* copy = menu.addAction(tr("Copy"));
    QAction* paste = menu.addAction(tr("Paste"));
    QAction* selectAll = menu.addAction(tr("Select All"));
    connect(cut, &QAction::triggered, this, [this]() { m_editor->cut(); });
    connect(copy, &QAction::triggered, this, [this]() { m_editor->copy(); });
    connect(paste, &QAction::triggered, this, [this]() { m_editor->paste(); });
    connect(selectAll, &QAction::triggered, this, [this]() { m_editor->selectAll(); });
    menu.addSeparator();
    QAction* comment = menu.addAction(tr("Toggle Line Comment"));
    comment->setEnabled(LanguageManager::byId(m_languageId) &&
                        LanguageManager::byId(m_languageId)->hasLineComment());
    connect(comment, &QAction::triggered, this, &EditorWidget::toggleLineComment);
    QAction* duplicate = menu.addAction(tr("Duplicate Current Line"));
    connect(duplicate, &QAction::triggered, this, &EditorWidget::duplicateCurrentLine);
    QAction* toggleBookmark = menu.addAction(tr("Toggle Bookmark"));
    connect(toggleBookmark, &QAction::triggered, this, &EditorWidget::toggleBookmark);
    menu.exec(event->globalPos());
}

} // namespace PadForge
