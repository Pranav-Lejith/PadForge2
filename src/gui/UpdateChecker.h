#pragma once

#include <QObject>
#include <QString>

class QWidget;

namespace PadForge {

class UpdateChecker : public QObject {
    Q_OBJECT

public:
    explicit UpdateChecker(QObject* parent = nullptr);
    void checkForUpdates(QWidget* parent);

    static bool isNewerVersion(const QString& current, const QString& latest);
};

} // namespace PadForge
