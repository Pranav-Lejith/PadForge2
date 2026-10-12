#include "Backup.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace PadForge {

namespace {

QString& directoryOverride() {
    static QString dir;
    return dir;
}

QString entryPath(const QString& key) {
    return BackupManager::directory() + QLatin1Char('/') + key + QStringLiteral(".json");
}

BackupEntry fromJson(const QString& key, const QJsonObject& o) {
    BackupEntry entry;
    entry.key = key;
    entry.originalPath = o.value(QStringLiteral("originalPath")).toString();
    entry.text = o.value(QStringLiteral("text")).toString();
    entry.untitled = o.value(QStringLiteral("untitled")).toBool();
    entry.timestamp = static_cast<qint64>(o.value(QStringLiteral("timestamp")).toDouble());
    return entry;
}

QJsonObject toJson(const BackupEntry& entry) {
    QJsonObject o;
    o[QStringLiteral("originalPath")] = entry.originalPath;
    o[QStringLiteral("text")] = entry.text;
    o[QStringLiteral("untitled")] = entry.untitled;
    o[QStringLiteral("timestamp")] = static_cast<double>(entry.timestamp);
    return o;
}

} // namespace

QString BackupManager::directory() {
    if (!directoryOverride().isEmpty())
        return directoryOverride();
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return base + QStringLiteral("/backup");
}

void BackupManager::setDirectory(const QString& dir) {
    directoryOverride() = dir;
}

QString BackupManager::keyForPath(const QString& path) {
    const QString clean = QDir::cleanPath(QFileInfo(path).absoluteFilePath());
    const QByteArray hash = QCryptographicHash::hash(clean.toUtf8(), QCryptographicHash::Sha1);
    return QString::fromLatin1(hash.toHex());
}

bool BackupManager::write(const BackupEntry& entry, QString* error) {
    if (entry.key.isEmpty()) {
        if (error)
            *error = QStringLiteral("backup key is empty");
        return false;
    }
    QDir().mkpath(directory());
    QFile file(entryPath(entry.key));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error)
            *error = file.errorString();
        return false;
    }
    BackupEntry stored = entry;
    if (stored.timestamp == 0)
        stored.timestamp = QDateTime::currentMSecsSinceEpoch();
    file.write(QJsonDocument(toJson(stored)).toJson(QJsonDocument::Compact));
    file.close();
    return true;
}

BackupEntry BackupManager::read(const QString& key) {
    BackupEntry entry;
    QFile file(entryPath(key));
    if (!file.open(QIODevice::ReadOnly))
        return entry;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return entry;
    return fromJson(key, doc.object());
}

QList<BackupEntry> BackupManager::list() {
    QList<BackupEntry> entries;
    QDir dir(directory());
    if (!dir.exists())
        return entries;
    const QStringList files = dir.entryList({QStringLiteral("*.json")}, QDir::Files, QDir::Time);
    for (const QString& name : files) {
        const QString key = QFileInfo(name).completeBaseName();
        const BackupEntry entry = read(key);
        if (!entry.key.isEmpty())
            entries.append(entry);
    }
    return entries;
}

void BackupManager::remove(const QString& key) {
    QFile::remove(entryPath(key));
}

void BackupManager::clear() {
    QDir dir(directory());
    if (!dir.exists())
        return;
    const QStringList files = dir.entryList({QStringLiteral("*.json")}, QDir::Files);
    for (const QString& name : files)
        QFile::remove(dir.filePath(name));
}

bool BackupManager::hasBackups() {
    QDir dir(directory());
    if (!dir.exists())
        return false;
    return !dir.entryList({QStringLiteral("*.json")}, QDir::Files).isEmpty();
}

} // namespace PadForge
