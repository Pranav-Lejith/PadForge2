#include "TextOps.h"

#include <QRegularExpression>
#include <QSet>
#include <algorithm>

namespace PadForge {

namespace {

QString indentUnit(int spaces) {
    return QString(spaces, QLatin1Char(' '));
}

bool isBlank(const QString& line) {
    return line.trimmed().isEmpty();
}

} // namespace

QStringList TextOps::split(const QString& text, bool* trailingNewline) {
    bool trailing = text.endsWith(QLatin1Char('\n'));
    if (trailingNewline)
        *trailingNewline = trailing;
    QStringList lines = text.split(QLatin1Char('\n'));
    if (trailing && !lines.isEmpty() && lines.last().isEmpty())
        lines.removeLast();
    return lines;
}

QString TextOps::join(const QStringList& lines, bool trailingNewline) {
    QString result = lines.join(QLatin1Char('\n'));
    if (trailingNewline)
        result += QLatin1Char('\n');
    return result;
}

QString TextOps::duplicateLines(const QString& text) {
    bool trailing = false;
    const QStringList lines = split(text, &trailing);
    QStringList out;
    for (const QString& line : lines) {
        out << line;
        out << line;
    }
    return join(out, trailing);
}

QString TextOps::deleteLines(const QString& text) {
    Q_UNUSED(text);
    return QString();
}

QString TextOps::joinLinesOperation(const QString& text) {
    bool trailing = false;
    QStringList lines = split(text, &trailing);
    QString joined;
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines.at(i).trimmed();
        if (i > 0 && !joined.isEmpty())
            joined += QLatin1Char(' ');
        joined += line;
    }
    return joined + (trailing ? QStringLiteral("\n") : QString());
}

QString TextOps::splitLinesOperation(const QString& text) {
    const QStringList words =
        text.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    return join(words, text.endsWith(QLatin1Char('\n')));
}

QString TextOps::sortLines(const QString& text, bool ascending, bool caseSensitive,
                           bool removeDuplicates) {
    bool trailing = false;
    QStringList lines = split(text, &trailing);
    auto less = [ascending, caseSensitive](const QString& a, const QString& b) {
        const int cmp = caseSensitive ? QString::compare(a, b, Qt::CaseSensitive)
                                      : QString::compare(a, b, Qt::CaseInsensitive);
        return ascending ? cmp < 0 : cmp > 0;
    };
    std::stable_sort(lines.begin(), lines.end(), less);
    if (removeDuplicates) {
        QStringList unique;
        for (const QString& line : lines) {
            if (unique.isEmpty() || unique.last() != line)
                unique << line;
        }
        lines = unique;
    }
    return join(lines, trailing);
}

QString TextOps::removeDuplicateLines(const QString& text) {
    bool trailing = false;
    const QStringList lines = split(text, &trailing);
    QStringList out;
    QSet<QString> seen;
    for (const QString& line : lines) {
        if (seen.contains(line))
            continue;
        seen.insert(line);
        out << line;
    }
    return join(out, trailing);
}

QString TextOps::removeConsecutiveDuplicateLines(const QString& text) {
    bool trailing = false;
    const QStringList lines = split(text, &trailing);
    QStringList out;
    for (const QString& line : lines) {
        if (out.isEmpty() || out.last() != line)
            out << line;
    }
    return join(out, trailing);
}

QString TextOps::removeEmptyLines(const QString& text) {
    bool trailing = false;
    const QStringList lines = split(text, &trailing);
    QStringList out;
    for (const QString& line : lines) {
        if (!isBlank(line))
            out << line;
    }
    return join(out, trailing);
}

QString TextOps::removeEmptyLinesAtStart(const QString& text) {
    bool trailing = false;
    QStringList lines = split(text, &trailing);
    while (!lines.isEmpty() && isBlank(lines.first()))
        lines.removeFirst();
    return join(lines, trailing);
}

QString TextOps::removeEmptyLinesAtEnd(const QString& text) {
    bool trailing = false;
    QStringList lines = split(text, &trailing);
    while (!lines.isEmpty() && isBlank(lines.last()))
        lines.removeLast();
    return join(lines, trailing);
}

QString TextOps::removeEmptyLinesInside(const QString& text) {
    bool trailing = false;
    QStringList lines = split(text, &trailing);
    QStringList out;
    bool seenContent = false;
    for (int i = 0; i < lines.size(); ++i) {
        const QString& line = lines.at(i);
        if (isBlank(line)) {
            if (!seenContent)
                continue; // leading
            // Look ahead: is there any content after this?
            bool contentAfter = false;
            for (int j = i + 1; j < lines.size(); ++j) {
                if (!isBlank(lines.at(j))) {
                    contentAfter = true;
                    break;
                }
            }
            if (!contentAfter)
                continue; // trailing
            out << line;
        } else {
            seenContent = true;
            out << line;
        }
    }
    return join(out, trailing);
}

QString TextOps::trimTrailing(const QString& text) {
    return removeTrailingSpacesEveryLine(text);
}

QString TextOps::trimLeading(const QString& text) {
    bool trailing = false;
    QStringList lines = split(text, &trailing);
    for (QString& line : lines)
        line = line.remove(QRegularExpression(QStringLiteral("^[ \\t]+")));
    return join(lines, trailing);
}

QString TextOps::trimBoth(const QString& text) {
    bool trailing = false;
    QStringList lines = split(text, &trailing);
    for (QString& line : lines)
        line = line.trimmed();
    return join(lines, trailing);
}

QString TextOps::removeTrailingSpacesEveryLine(const QString& text) {
    bool trailing = false;
    QStringList lines = split(text, &trailing);
    for (QString& line : lines)
        line = line.remove(QRegularExpression(QStringLiteral("[ \\t]+$")));
    return join(lines, trailing);
}

QString TextOps::convertCase(const QString& text, CaseMode mode) {
    switch (mode) {
    case CaseMode::Upper:
        return text.toUpper();
    case CaseMode::Lower:
        return text.toLower();
    case CaseMode::Title: {
        QString out = text.toLower();
        bool startOfWord = true;
        for (int i = 0; i < out.size(); ++i) {
            const QChar c = out.at(i);
            if (c.isLetter()) {
                if (startOfWord)
                    out[i] = c.toUpper();
                startOfWord = false;
            } else {
                startOfWord = !c.isLetterOrNumber();
            }
        }
        return out;
    }
    case CaseMode::Sentence: {
        QString out = text.toLower();
        bool startOfSentence = true;
        for (int i = 0; i < out.size(); ++i) {
            const QChar c = out.at(i);
            if (c.isLetter()) {
                if (startOfSentence)
                    out[i] = c.toUpper();
                startOfSentence = false;
            } else if (c == QLatin1Char('.') || c == QLatin1Char('!') || c == QLatin1Char('?') ||
                       c == QLatin1Char('\n')) {
                startOfSentence = true;
            }
        }
        return out;
    }
    case CaseMode::Proper: {
        QString out = text.toLower();
        bool startOfWord = true;
        for (int i = 0; i < out.size(); ++i) {
            const QChar c = out.at(i);
            if (c.isLetter()) {
                startOfWord = false;
            } else if (c.isSpace()) {
                startOfWord = true;
            }
            if (startOfWord && i + 1 < out.size())
                out[i + 1] = out.at(i + 1).toUpper();
        }
        return out;
    }
    case CaseMode::Invert: {
        QString out = text;
        for (int i = 0; i < out.size(); ++i) {
            const QChar c = out.at(i);
            if (c.isUpper())
                out[i] = c.toLower();
            else if (c.isLower())
                out[i] = c.toUpper();
        }
        return out;
    }
    }
    return text;
}

QString TextOps::indent(const QString& text, int spaces) {
    bool trailing = false;
    QStringList lines = split(text, &trailing);
    const QString unit = indentUnit(spaces);
    for (QString& line : lines) {
        if (!line.isEmpty())
            line.prepend(unit);
    }
    return join(lines, trailing);
}

QString TextOps::unindent(const QString& text, int spaces) {
    bool trailing = false;
    QStringList lines = split(text, &trailing);
    for (QString& line : lines) {
        int removed = 0;
        if (line.startsWith(QLatin1Char('\t'))) {
            line.remove(0, 1);
            continue;
        }
        while (removed < spaces && !line.isEmpty() && line.at(0) == QLatin1Char(' ')) {
            line.remove(0, 1);
            ++removed;
        }
    }
    return join(lines, trailing);
}

QString TextOps::tabify(const QString& text, int tabWidth) {
    if (tabWidth <= 0)
        return text;
    bool trailing = false;
    QStringList lines = split(text, &trailing);
    for (QString& line : lines) {
        QString out;
        int column = 0;
        for (const QChar c : line) {
            if (c == QLatin1Char(' ')) {
                ++column;
                if (column % tabWidth == 0) {
                    out += QLatin1Char('\t');
                }
            } else {
                out += c;
                ++column;
            }
        }
        line = out;
    }
    return join(lines, trailing);
}

QString TextOps::untabify(const QString& text, int tabWidth) {
    if (tabWidth <= 0)
        return text;
    bool trailing = false;
    QStringList lines = split(text, &trailing);
    for (QString& line : lines) {
        QString out;
        int column = 0;
        for (const QChar c : line) {
            if (c == QLatin1Char('\t')) {
                const int spaces = tabWidth - (column % tabWidth);
                out += QString(spaces, QLatin1Char(' '));
                column += spaces;
            } else {
                out += c;
                ++column;
            }
        }
        line = out;
    }
    return join(lines, trailing);
}

QString TextOps::toggleLineComment(const QString& text, const QString& token) {
    if (token.isEmpty())
        return text;
    bool trailing = false;
    QStringList lines = split(text, &trailing);

    bool allCommented = true;
    bool anyContent = false;
    for (const QString& line : lines) {
        if (isBlank(line))
            continue;
        anyContent = true;
        const QString trimmed = line.trimmed();
        if (!trimmed.startsWith(token)) {
            allCommented = false;
            break;
        }
    }
    if (!anyContent)
        return text;

    const QString marker = token + QLatin1Char(' ');
    for (QString& line : lines) {
        if (isBlank(line))
            continue;
        const int indentLen = line.size() - line.trimmed().size();
        const QString indent = line.left(indentLen);
        QString body = line.mid(indentLen);
        if (allCommented) {
            if (body.startsWith(marker))
                body.remove(0, marker.size());
            else if (body.startsWith(token))
                body.remove(0, token.size());
        } else {
            body.prepend(marker);
        }
        line = indent + body;
    }
    return join(lines, trailing);
}

QString TextOps::toggleBlockComment(const QString& text, const QString& open,
                                    const QString& close) {
    if (open.isEmpty() || close.isEmpty())
        return text;
    const QString trimmed = text.trimmed();
    if (trimmed.startsWith(open) && trimmed.endsWith(close) &&
        trimmed.size() >= open.size() + close.size()) {
        const int start = text.indexOf(open);
        const int end = text.lastIndexOf(close);
        QString inner = text.mid(start + open.size(), end - start - open.size());
        if (inner.startsWith(QLatin1Char(' ')))
            inner.remove(0, 1);
        if (inner.endsWith(QLatin1Char(' ')))
            inner.chop(1);
        return text.left(start) + inner + text.mid(end + close.size());
    }
    return open + QLatin1Char(' ') + text + QLatin1Char(' ') + close;
}

QString TextOps::insertLineNumbers(const QString& text, int start, int step, int width,
                                   const QString& separator) {
    bool trailing = false;
    QStringList lines = split(text, &trailing);
    int number = start;
    for (QString& line : lines) {
        const QString label = QString::number(number).rightJustified(width, QLatin1Char(' '));
        line = label + separator + line;
        number += step;
    }
    return join(lines, trailing);
}

namespace {

// Returns the prefix of `line` covering the first (column - 1) display columns,
// padding with spaces (or tabs, at tab stops) when the line is shorter.
QString padToColumn(const QString& line, int column, bool useTabs, int tabWidth) {
    const int target = column - 1;
    if (target <= 0)
        return QString();
    int current = 0;
    QString out;
    for (const QChar c : line) {
        if (current >= target)
            break;
        if (c == QLatin1Char('\t') && tabWidth > 0)
            current += tabWidth - (current % tabWidth);
        else
            ++current;
        out += c;
    }
    while (current < target) {
        if (useTabs && tabWidth > 0 && (current % tabWidth) == 0 && target - current >= tabWidth) {
            out += QLatin1Char('\t');
            current += tabWidth;
        } else {
            out += QLatin1Char(' ');
            ++current;
        }
    }
    return out;
}

} // namespace

QString TextOps::columnInsertText(const QString& text, int column, const QString& insert,
                                  int startLine, int endLine, bool repeatIntoEmptyLines,
                                  bool useTabs, int tabWidth) {
    bool trailing = false;
    QStringList lines = split(text, &trailing);
    if (startLine < 0)
        startLine = 0;
    if (endLine < 0 || endLine >= lines.size())
        endLine = lines.size() - 1;
    bool changed = false;
    for (int i = startLine; i <= endLine && i < lines.size(); ++i) {
        if (!repeatIntoEmptyLines && lines.at(i).trimmed().isEmpty())
            continue;
        const QString padded = padToColumn(lines.at(i), column, useTabs, tabWidth);
        QString rest = lines.at(i).mid(padded.size());
        lines[i] = padded + insert + rest;
        changed = true;
    }
    return changed ? join(lines, trailing) : text;
}

QString TextOps::columnInsertNumbers(const QString& text, int column, int start, int step,
                                     int padding, int startLine, int endLine,
                                     bool repeatIntoEmptyLines, bool useTabs, int tabWidth) {
    bool trailing = false;
    QStringList lines = split(text, &trailing);
    if (startLine < 0)
        startLine = 0;
    if (endLine < 0 || endLine >= lines.size())
        endLine = lines.size() - 1;
    int number = start;
    bool changed = false;
    for (int i = startLine; i <= endLine && i < lines.size(); ++i) {
        if (!repeatIntoEmptyLines && lines.at(i).trimmed().isEmpty()) {
            number += step;
            continue;
        }
        const QString padded = padToColumn(lines.at(i), column, useTabs, tabWidth);
        const QString rest = lines.at(i).mid(padded.size());
        const QString num = QString::number(number).rightJustified(padding, QLatin1Char('0'));
        lines[i] = padded + num + rest;
        changed = true;
        number += step;
    }
    return changed ? join(lines, trailing) : text;
}

int TextOps::wordCount(const QString& text) {
    int count = 0;
    const QRegularExpression re(QStringLiteral("\\S+"));
    auto it = re.globalMatch(text);
    while (it.hasNext()) {
        it.next();
        ++count;
    }
    return count;
}

int TextOps::lineCount(const QString& text) {
    if (text.isEmpty())
        return 0;
    return text.count(QLatin1Char('\n')) + (text.endsWith(QLatin1Char('\n')) ? 0 : 1);
}

int TextOps::characterCount(const QString& text) {
    return text.size();
}

} // namespace PadForge
