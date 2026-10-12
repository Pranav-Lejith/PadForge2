#pragma once

#include <QList>
#include <QString>

namespace PadForge {

// One snapshot of an unsaved (or modified) document on disk.
struct BackupEntry {
    QString key;          // filesystem-safe identifier used as the file name
    QString originalPath; // empty for untitled documents
    QString text;         // full document contents
    bool untitled = false;
    qint64 timestamp = 0; // Unix milliseconds
};

// A crash-recovery store for snapping unsaved editor contents to disk.
// Each entry is written as a standalone JSON file so a partial write of one
// document never corrupts the others.
class BackupManager {
public:
    static QString directory();
    static void setDirectory(const QString& dir); // mainly for tests

    // Stable, filesystem-safe key derived from an absolute path.
    static QString keyForPath(const QString& path);

    static bool write(const BackupEntry& entry, QString* error = nullptr);
    static BackupEntry read(const QString& key);
    static QList<BackupEntry> list();
    static void remove(const QString& key);
    static void clear();

    // True when at least one snapshot exists.
    static bool hasBackups();
};

} // namespace PadForge
