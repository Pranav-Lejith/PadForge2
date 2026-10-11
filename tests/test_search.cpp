#include "SearchEngine.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace PadForge;

class TestSearch : public QObject {
    Q_OBJECT

private slots:
    void findAllLiteral();
    void caseSensitivity();
    void wholeWord();
    void lineNumbers();
    void findNextForwardWrap();
    void findNextBackwardWrap();
    void regexSearch();
    void replaceAllLiteral();
    void replaceAllRegexBackrefs();
    void extendedEscapes();
    void countMatches();

    void findInFileAndDirectory();
};

void TestSearch::findAllLiteral()
{
    const SearchOptions opts;
    const auto matches = SearchEngine::findAll("cat dog cat bird cat", "cat", opts);
    QCOMPARE(matches.size(), 3);
    QCOMPARE(matches[0].start, 0);
    QCOMPARE(matches[1].start, 8);
    QCOMPARE(matches[2].start, 17);
}

void TestSearch::caseSensitivity()
{
    SearchOptions opts;
    QCOMPARE(SearchEngine::findAll("Cat cat CAT", "cat", opts).size(), 3);
    opts.caseSensitive = true;
    QCOMPARE(SearchEngine::findAll("Cat cat CAT", "cat", opts).size(), 1);
}

void TestSearch::wholeWord()
{
    SearchOptions opts;
    opts.wholeWord = true;
    QCOMPARE(SearchEngine::findAll("cat category cat", "cat", opts).size(), 2);
    QCOMPARE(SearchEngine::findAll("scatter cat", "cat", opts).size(), 1);
}

void TestSearch::lineNumbers()
{
    const SearchOptions opts;
    const auto matches = SearchEngine::findAll("alpha\nbeta gamma\nbeta", "beta", opts);
    QCOMPARE(matches.size(), 2);
    QCOMPARE(matches[0].line, 2);
    QCOMPARE(matches[1].line, 3);
}

void TestSearch::findNextForwardWrap()
{
    SearchOptions opts;
    opts.wrap = true;
    SearchMatch m = SearchEngine::findNext("cat dog cat", "cat", 1, opts);
    QCOMPARE(m.start, 8);
    m = SearchEngine::findNext("cat dog cat", "cat", 9, opts);
    QCOMPARE(m.start, 0); // wrapped
}

void TestSearch::findNextBackwardWrap()
{
    SearchOptions opts;
    opts.searchUp = true;
    SearchMatch m = SearchEngine::findNext("cat dog cat", "cat", 9, opts);
    QCOMPARE(m.start, 8);
    m = SearchEngine::findNext("cat dog cat", "cat", 8, opts);
    QCOMPARE(m.start, 0);
    m = SearchEngine::findNext("cat dog cat", "cat", 0, opts);
    QCOMPARE(m.start, 8); // wrapped to last
}

void TestSearch::regexSearch()
{
    SearchOptions opts;
    opts.regex = true;
    const auto matches = SearchEngine::findAll("a1 b22 c333", "[0-9]+", opts);
    QCOMPARE(matches.size(), 3);
    QCOMPARE(matches[0].length, 1);
    QCOMPARE(matches[1].length, 2);
    QCOMPARE(matches[2].length, 3);
}

void TestSearch::replaceAllLiteral()
{
    const SearchOptions opts;
    int count = 0;
    const QString result = SearchEngine::replaceAll("a.b a.b", "a.b", "x.y", opts, &count);
    QCOMPARE(result, QStringLiteral("x.y x.y"));
    QCOMPARE(count, 2);
}

void TestSearch::replaceAllRegexBackrefs()
{
    SearchOptions opts;
    opts.regex = true;
    int count = 0;
    const QString result =
        SearchEngine::replaceAll("2020-01-02", "(\\d+)-(\\d+)-(\\d+)", "\\3/\\2/\\1", opts, &count);
    QCOMPARE(result, QStringLiteral("02/01/2020"));
    QCOMPARE(count, 1);
}

void TestSearch::extendedEscapes()
{
    SearchOptions opts;
    opts.extended = true;
    QCOMPARE(SearchEngine::unescape("a\\tb\\nc"), QStringLiteral("a\tb\nc"));
    QCOMPARE(SearchEngine::unescape("\\x41\\u0042"), QStringLiteral("AB"));
    const auto matches = SearchEngine::findAll("a\tb", "\\t", opts);
    QCOMPARE(matches.size(), 1);
}

void TestSearch::countMatches()
{
    const SearchOptions opts;
    QCOMPARE(SearchEngine::count("the cat sat on the mat", "the", opts), 2);
}

void TestSearch::findInFileAndDirectory()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/a.txt");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("hello world\nsecond hello\n");
    f.close();
    QFile f2(dir.path() + "/b.log");
    QVERIFY(f2.open(QIODevice::WriteOnly));
    f2.write("nothing here");
    f2.close();

    const SearchOptions opts;
    const auto fileResults = FileSearcher::searchInFile(dir.path() + "/a.txt", "hello", opts);
    QCOMPARE(fileResults.size(), 2);
    QCOMPARE(fileResults[0].line, 1);
    QCOMPARE(fileResults[1].line, 2);
    QCOMPARE(fileResults[1].text, QStringLiteral("second hello"));

    const auto dirResults =
        FileSearcher::searchInDirectory(dir.path(), {"*.txt"}, "hello", opts, false);
    QCOMPARE(dirResults.size(), 2);
}

QTEST_MAIN(TestSearch)
#include "test_search.moc"
