#pragma once

#include <QByteArray>
#include <QCryptographicHash>
#include <QString>
#include <QStringList>

namespace PadForge {

// Pure, line-oriented text transformations used by the editor's Edit menu.
// All functions treat '\n' as the line separator and preserve a trailing
// newline if the input had one. They are stateless and unit tested.

enum class CaseMode {
    Upper,
    Lower,
    Title,    // First Letter Of Each Word
    Sentence, // First letter of each sentence
    Proper,   // first letter lower, rest per word
    Invert,
};

class TextOps {
public:
    // Split into lines, reporting whether the text ended with a newline.
    static QStringList split(const QString& text, bool* trailingNewline = nullptr);
    static QString join(const QStringList& lines, bool trailingNewline);

    // Line operations.
    static QString duplicateLines(const QString& text);
    static QString deleteLines(const QString& text);
    static QString joinLinesOperation(const QString& text);
    static QString splitLinesOperation(const QString& text);

    // Sorting / de-duplication.
    static QString sortLines(const QString& text, bool ascending, bool caseSensitive,
                             bool removeDuplicates);
    static QString removeDuplicateLines(const QString& text);
    static QString removeConsecutiveDuplicateLines(const QString& text);

    // Blank operations.
    static QString removeEmptyLines(const QString& text);
    static QString removeEmptyLinesAtStart(const QString& text);
    static QString removeEmptyLinesAtEnd(const QString& text);
    static QString removeEmptyLinesInside(const QString& text);

    // Whitespace.
    static QString trimTrailing(const QString& text);
    static QString trimLeading(const QString& text);
    static QString trimBoth(const QString& text);
    static QString removeTrailingSpacesEveryLine(const QString& text);

    // Case conversion.
    static QString convertCase(const QString& text, CaseMode mode);

    // Indentation.
    static QString indent(const QString& text, int spaces);
    static QString unindent(const QString& text, int spaces);
    static QString tabify(const QString& text, int tabWidth);
    static QString untabify(const QString& text, int tabWidth);

    // Comments.
    static QString toggleLineComment(const QString& text, const QString& token);
    static QString toggleBlockComment(const QString& text, const QString& open,
                                      const QString& close);

    // Column editing.
    static QString insertLineNumbers(const QString& text, int start, int step, int width,
                                     const QString& separator);
    static QString columnInsertText(const QString& text, int column, const QString& insert,
                                    int startLine, int endLine, bool repeatIntoEmptyLines,
                                    bool useTabs, int tabWidth);
    static QString columnInsertNumbers(const QString& text, int column, int start, int step,
                                       int padding, int startLine, int endLine,
                                       bool repeatIntoEmptyLines, bool useTabs, int tabWidth);

    // Word/character statistics.
    static int wordCount(const QString& text);
    static int lineCount(const QString& text);
    static int characterCount(const QString& text);

    // Line addressing (all line numbers are 1-based).
    static int lineNumberAt(const QString& text, int offset);
    static int lineStartAt(const QString& text, int lineNumber);
    static QString lineContent(const QString& text, int lineNumber);

    // Encoders / converters (MIME Tools / Converter style). Text is treated as
    // UTF-8 where an octet stream is required.
    static QString base64Encode(const QString& text);
    static QString base64Decode(const QString& text, bool* ok = nullptr);
    static QString urlEncode(const QString& text);
    static QString urlDecode(const QString& text);
    static QString htmlEncode(const QString& text);
    static QString htmlDecode(const QString& text);
    static QString unicodeEscape(const QString& text);
    static QString unicodeUnescape(const QString& text);
    static QString rot13(const QString& text);
    static QString toHex(const QString& text);
    static QString fromHex(const QString& text, bool* ok = nullptr);

    // Hex digest of the UTF-8 bytes of `text` (upper-case, no separators).
    static QString digest(const QString& text, QCryptographicHash::Algorithm algorithm);
};

} // namespace PadForge
