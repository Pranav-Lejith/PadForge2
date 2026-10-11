#include "SearchEngine.h"

#include "Encoding.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>

namespace PadForge {

QString SearchEngine::buildPattern(const QString& term, const SearchOptions& opts)
{
    if (opts.regex) {
        if (opts.wholeWord)
            return QStringLiteral("\\b(?:") + term + QStringLiteral(")\\b");
        return term;
    }

    const QString literal = opts.extended ? unescape(term) : term;
    QString escaped = QRegularExpression::escape(literal);
    if (opts.wholeWord)
        escaped = QStringLiteral("\\b(?:") + escaped + QStringLiteral(")\\b");
    return escaped;
}

QRegularExpression SearchEngine::buildRegex(const QString& term, const SearchOptions& opts)
{
    const QString pattern = buildPattern(term, opts);
    QRegularExpression::PatternOptions flags = QRegularExpression::NoPatternOption;
    if (!opts.caseSensitive)
        flags |= QRegularExpression::CaseInsensitiveOption;
    if (opts.dotMatchesNewline)
        flags |= QRegularExpression::DotMatchesEverythingOption;
    QRegularExpression re(pattern, flags);
    re.optimize();
    return re;
}

QString SearchEngine::unescape(const QString& term)
{
    QString out;
    out.reserve(term.size());
    for (int i = 0; i < term.size(); ++i) {
        const QChar c = term.at(i);
        if (c != QLatin1Char('\\') || i + 1 >= term.size()) {
            out.append(c);
            continue;
        }
        const QChar n = term.at(++i);
        switch (n.unicode()) {
        case 'n':
            out.append(QLatin1Char('\n'));
            break;
        case 'r':
            out.append(QLatin1Char('\r'));
            break;
        case 't':
            out.append(QLatin1Char('\t'));
            break;
        case '0':
            out.append(QChar(0));
            break;
        case '\\':
            out.append(QLatin1Char('\\'));
            break;
        case 'x': {
            bool ok = false;
            QString hex;
            while (hex.size() < 2 && i + 1 < term.size()
                   && term.at(i + 1).isLetterOrNumber()) {
                hex.append(term.at(++i));
            }
            const int value = hex.toInt(&ok, 16);
            if (ok)
                out.append(QChar(ushort(value & 0xFF)));
            break;
        }
        case 'u': {
            bool ok = false;
            QString hex;
            while (hex.size() < 4 && i + 1 < term.size()
                   && term.at(i + 1).isLetterOrNumber()) {
                hex.append(term.at(++i));
            }
            const int value = hex.toInt(&ok, 16);
            if (ok)
                out.append(QChar(ushort(value)));
            break;
        }
        default:
            out.append(n);
            break;
        }
    }
    return out;
}

int SearchEngine::lineForPosition(const QString& text, int pos)
{
    int line = 1;
    const int limit = qBound(0, pos, int(text.size()));
    for (int i = 0; i < limit; ++i)
        if (text.at(i) == QLatin1Char('\n'))
            ++line;
    return line;
}

QList<SearchMatch> SearchEngine::findAll(const QString& text, const QString& term,
                                         const SearchOptions& opts, int limit)
{
    QList<SearchMatch> out;
    if (term.isEmpty())
        return out;

    const QRegularExpression re = buildRegex(term, opts);
    if (!re.isValid())
        return out;

    int line = 1;
    int lastPos = 0;
    auto it = re.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        const int start = int(m.capturedStart());
        for (int i = lastPos; i < start; ++i)
            if (text.at(i) == QLatin1Char('\n'))
                ++line;
        lastPos = start;
        out.append(SearchMatch{start, int(m.capturedLength()), line});
        if (limit > 0 && out.size() >= limit)
            break;
    }
    return out;
}

SearchMatch SearchEngine::findNext(const QString& text, const QString& term, int from,
                                   const SearchOptions& opts)
{
    const QList<SearchMatch> matches = findAll(text, term, opts);
    if (matches.isEmpty())
        return {};

    if (opts.searchUp) {
        SearchMatch result;
        for (const SearchMatch& m : matches)
            if (m.start < from)
                result = m;
            else
                break;
        if (!result.isValid() && opts.wrap)
            result = matches.last();
        return result;
    }

    for (const SearchMatch& m : matches)
        if (m.start >= from)
            return m;
    if (opts.wrap)
        return matches.first();
    return {};
}

int SearchEngine::count(const QString& text, const QString& term, const SearchOptions& opts)
{
    return int(findAll(text, term, opts).size());
}

QString SearchEngine::replaceAll(const QString& text, const QString& term,
                                 const QString& replacement, const SearchOptions& opts,
                                 int* countOut)
{
    const QRegularExpression re = buildRegex(term, opts);
    QString replacementString;
    if (opts.regex) {
        replacementString = replacement;
    } else {
        QString literal = opts.extended ? unescape(replacement) : replacement;
        literal.replace(QLatin1String("\\"), QLatin1String("\\\\"));
        replacementString = literal;
    }

    int occurrences = 0;
    {
        auto it = re.globalMatch(text);
        while (it.hasNext()) {
            it.next();
            ++occurrences;
        }
    }
    if (countOut)
        *countOut = occurrences;
    if (occurrences == 0 || !re.isValid())
        return text;

    QString result = text;
    result.replace(re, replacementString);
    return result;
}

QList<FileSearchResult> FileSearcher::searchInFile(const QString& path, const QString& term,
                                                   const SearchOptions& opts, int maxResults)
{
    QList<FileSearchResult> results;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return results;

    const QByteArray raw = file.readAll();
    if (EncodingDetector::looksBinary(raw.left(8192)))
        return results;

    const EncodingInfo enc = EncodingDetector::detect(raw.left(65536)).encoding;
    const QString text = EncodingDetector::decode(raw, enc);

    const QList<SearchMatch> matches =
        SearchEngine::findAll(text, term, opts, maxResults > 0 ? maxResults : -1);
    for (const SearchMatch& m : matches) {
        const int lineStart = text.lastIndexOf(QLatin1Char('\n'), m.start - 1) + 1;
        const int lineEnd = text.indexOf(QLatin1Char('\n'), m.start);
        FileSearchResult r;
        r.file = path;
        r.line = m.line;
        r.column = m.start - lineStart + 1;
        r.length = m.length;
        r.text = text.mid(lineStart, (lineEnd < 0 ? int(text.size()) : lineEnd) - lineStart);
        results.append(r);
        if (maxResults > 0 && results.size() >= maxResults)
            break;
    }
    return results;
}

QList<FileSearchResult> FileSearcher::searchInDirectory(const QString& dir,
                                                        const QStringList& nameFilters,
                                                        const QString& term,
                                                        const SearchOptions& opts,
                                                        bool recursive)
{
    QList<FileSearchResult> results;
    const QStringList filters =
        nameFilters.isEmpty() ? QStringList{QStringLiteral("*")} : nameFilters;

    QDir::Filters flags = QDir::Files | QDir::Readable;
    if (recursive)
        flags |= QDir::AllDirs | QDir::NoDotAndDotDot;

    QDirIterator it(dir, filters, flags, recursive ? QDirIterator::Subdirectories
                                                   : QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        const QString path = it.next();
        if (!recursive && QFileInfo(path).isDir())
            continue;
        if (path.contains(QStringLiteral("/.git/")) || path.contains(QStringLiteral("/.svn/")))
            continue;
        results.append(searchInFile(path, term, opts));
    }
    return results;
}

} // namespace PadForge
