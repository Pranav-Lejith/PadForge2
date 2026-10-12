#include "Backup.h"

#include <QDir>
#include <QTemporaryDir>
#include <QTest>

using namespace PadForge;

class BackupTest : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void roundTrip();
    void listAndRemove();
    void keyForPathIsStableAndSafe();

private:
    QTemporaryDir m_dir;
};

void BackupTest::init() {
    QVERIFY(m_dir.isValid());
    BackupManager::setDirectory(m_dir.path() + QStringLiteral("/backup"));
    BackupManager::clear();
}

void BackupTest::cleanup() {
    BackupManager::clear();
    BackupManager::setDirectory(QString());
}

void BackupTest::roundTrip() {
    BackupEntry entry;
    entry.key = BackupManager::keyForPath(QStringLiteral("/tmp/demo.txt"));
    entry.originalPath = QStringLiteral("/tmp/demo.txt");
    entry.text = QStringLiteral("line one\nline two\n");
    QString error;
    QVERIFY(BackupManager::write(entry, &error));
    QVERIFY(error.isEmpty());

    const BackupEntry read = BackupManager::read(entry.key);
    QCOMPARE(read.key, entry.key);
    QCOMPARE(read.originalPath, entry.originalPath);
    QCOMPARE(read.text, entry.text);
    QVERIFY(!read.untitled);
    QVERIFY(read.timestamp > 0);
}

void BackupTest::listAndRemove() {
    BackupEntry untitled;
    untitled.key = QStringLiteral("untitled-0");
    untitled.untitled = true;
    untitled.text = QStringLiteral("scratch");
    QVERIFY(BackupManager::write(untitled));

    BackupEntry file;
    file.key = BackupManager::keyForPath(QStringLiteral("/tmp/a.txt"));
    file.originalPath = QStringLiteral("/tmp/a.txt");
    file.text = QStringLiteral("data");
    QVERIFY(BackupManager::write(file));

    QVERIFY(BackupManager::hasBackups());
    const QList<BackupEntry> all = BackupManager::list();
    QCOMPARE(all.size(), 2);

    BackupManager::remove(untitled.key);
    QCOMPARE(BackupManager::list().size(), 1);

    BackupManager::clear();
    QVERIFY(!BackupManager::hasBackups());
    QVERIFY(BackupManager::list().isEmpty());
}

void BackupTest::keyForPathIsStableAndSafe() {
    const QString a = BackupManager::keyForPath(QStringLiteral("/tmp/foo.txt"));
    const QString b = BackupManager::keyForPath(QStringLiteral("/tmp/foo.txt"));
    const QString c = BackupManager::keyForPath(QStringLiteral("/tmp/bar.txt"));
    QCOMPARE(a, b);
    QVERIFY(a != c);
    QVERIFY(!a.contains(QLatin1Char('/')));
    QVERIFY(!a.contains(QLatin1Char(' ')));
}

QTEST_MAIN(BackupTest)
#include "test_backup.moc"
