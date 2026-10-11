#include "Session.h"

#include <QDir>
#include <QTemporaryDir>
#include <QTest>

using namespace PadForge;

class TestSession : public QObject {
    Q_OBJECT

private slots:
    void saveAndLoadRoundTrip();
    void loadMissingFileReportsError();
    void jsonRoundTrip();
    void recentFilesDeduplicateAndLimit();
    void recentFilesPersist();
};

void TestSession::saveAndLoadRoundTrip()
{
    Session session;
    session.activeIndex = 1;
    session.splitView = true;
    session.activeView = 1;

    SessionFile a;
    a.path = "/tmp/one.txt";
    a.cursor = 42;
    a.anchor = 10;
    a.firstVisibleLine = 5;
    a.language = "cpp";
    a.encoding = "UTF-8";
    a.eol = "LF";
    a.bookmarkLines = {3, 7, 9};
    session.files.append(a);

    SessionFile b;
    b.path = "/tmp/two.py";
    b.untitled = true;
    b.untitledText = "print('hi')";
    session.files.append(b);

    SessionFile sub;
    sub.path = "/tmp/sub.md";
    session.subViewFiles.append(sub);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + "/session.json";

    QString error;
    QVERIFY(SessionManager::save(session, path, &error));
    QVERIFY2(error.isEmpty(), qPrintable(error));

    bool ok = false;
    const Session loaded = SessionManager::load(path, &ok);
    QVERIFY(ok);
    QCOMPARE(loaded.files.size(), 2);
    QCOMPARE(loaded.activeIndex, 1);
    QCOMPARE(loaded.splitView, true);
    QCOMPARE(loaded.activeView, 1);
    QCOMPARE(loaded.files[0].path, QString("/tmp/one.txt"));
    QCOMPARE(loaded.files[0].cursor, 42);
    QCOMPARE(loaded.files[0].bookmarkLines, QList<int>({3, 7, 9}));
    QCOMPARE(loaded.files[1].untitled, true);
    QCOMPARE(loaded.files[1].untitledText, QString("print('hi')"));
    QCOMPARE(loaded.subViewFiles.size(), 1);
}

void TestSession::loadMissingFileReportsError()
{
    bool ok = true;
    const Session s = SessionManager::load("/nonexistent/path/session.json", &ok);
    QVERIFY(!ok);
    QVERIFY(s.isEmpty());
}

void TestSession::jsonRoundTrip()
{
    Session session;
    SessionFile f;
    f.path = "/x/y.txt";
    f.cursor = 3;
    session.files.append(f);
    session.recentFiles = {"/a", "/b"};

    const Session restored = SessionManager::fromJson(SessionManager::toJson(session));
    QCOMPARE(restored.files.size(), 1);
    QCOMPARE(restored.files[0].cursor, 3);
    QCOMPARE(restored.recentFiles, QStringList({"/a", "/b"}));
}

void TestSession::recentFilesDeduplicateAndLimit()
{
    RecentFiles recent(3);
    recent.add("/a");
    recent.add("/b");
    recent.add("/a"); // moves to front
    recent.add("/c");
    recent.add("/d"); // pushes /b out
    QCOMPARE(recent.list(), QStringList({"/d", "/c", "/a"}));

    recent.remove("/c");
    QCOMPARE(recent.list(), QStringList({"/d", "/a"}));
}

void TestSession::recentFilesPersist()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + "/recent.json";

    RecentFiles recent(10);
    recent.add("/one");
    recent.add("/two");
    QVERIFY(recent.save(path));

    RecentFiles loaded(10);
    QVERIFY(loaded.load(path));
    QCOMPARE(loaded.list(), QStringList({"/two", "/one"}));
}

QTEST_MAIN(TestSession)
#include "test_session.moc"
