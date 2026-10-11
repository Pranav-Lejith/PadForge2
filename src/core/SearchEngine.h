#pragma once

#include <QList>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

namespace PadForge {

struct SearchOptions {
    bool caseSensitive = false;
    bool wholeWord = false;
    bool regex = false;
    bool extended = false; // interpret \n, \t, \xHH, \uHHHH
    bool wrap = true;
    bool searchUp = false;
    bool dotMatchesNewline = false;
};

struct SearchMatch {
    int start = -1;
    int length = 0;
    int line = 0; // 1-based
    bool isValid() const { return start >= 0; }
};

struct FileSearchResult {
    QString file;
    int line = 0;
    int column = 0;
    int length = 0;
    QString text;
};

class SearchEngine {
public:
    static QString buildPattern(const QString& term, const SearchOptions& opts);
    static QRegularExpression buildRegex(const QString& term, const SearchOptions& opts);
    static QList<SearchMatch> findAll(const QString& text, const QString& term,
                                      const SearchOptions& opts, int limit = -1);
    static SearchMatch findNext(const QString& text, const QString& term, int from,
                                const SearchOptions& opts);
    static int count(const QString& text, const QString& term, const SearchOptions& opts);
    static QString replaceAll(const QString& text, const QString& term,
                              const QString& replacement, const SearchOptions& opts,
                              int* countOut = nullptr);
    static QString unescape(const QString& term);
    static int lineForPosition(const QString& text, int pos);
};

class FileSearcher {
public:
    static QList<FileSearchResult> searchInFile(const QString& path, const QString& term,
                                                const SearchOptions& opts,
                                                int maxResults = 1000);
    static QList<FileSearchResult> searchInDirectory(const QString& dir,
                                                     const QStringList& nameFilters,
                                                     const QString& term,
                                                     const SearchOptions& opts,
                                                     bool recursive);
};

} // namespace PadForge
