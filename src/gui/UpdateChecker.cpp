#include "UpdateChecker.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QUrl>
#include <QVersionNumber>
#include <QWidget>

namespace PadForge {

namespace {

const QUrl releaseApiUrl(
    QStringLiteral("https://api.github.com/repos/Pranav-Lejith/PadForge2/releases/latest"));
const QUrl
    releasesUrl(QStringLiteral("https://github.com/Pranav-Lejith/PadForge2/releases/latest"));
constexpr qint64 checkIntervalSeconds = 24 * 60 * 60;

QString normalizedVersion(QString version) {
    version = version.trimmed();
    if (version.startsWith(QLatin1Char('v'), Qt::CaseInsensitive))
        version.remove(0, 1);
    static const QRegularExpression versionPattern(QStringLiteral("^[0-9]+(\\.[0-9]+)*$"));
    if (!versionPattern.match(version).hasMatch())
        return {};
    return version;
}

} // namespace

UpdateChecker::UpdateChecker(QObject* parent) : QObject(parent) {}

bool UpdateChecker::isNewerVersion(const QString& current, const QString& latest) {
    const QString normalizedCurrent = normalizedVersion(current);
    const QString normalizedLatest = normalizedVersion(latest);
    if (normalizedCurrent.isEmpty() || normalizedLatest.isEmpty())
        return false;

    const QVersionNumber currentVersion = QVersionNumber::fromString(normalizedCurrent);
    const QVersionNumber latestVersion = QVersionNumber::fromString(normalizedLatest);
    return QVersionNumber::compare(latestVersion, currentVersion) > 0;
}

void UpdateChecker::checkForUpdates(QWidget* parent) {
    if (qEnvironmentVariableIsSet("PADFORGE_TESTING"))
        return;

    QSettings settings;
    const QDateTime lastChecked =
        settings.value(QStringLiteral("updates/lastCheckedUtc")).toDateTime();
    if (lastChecked.isValid() &&
        lastChecked.secsTo(QDateTime::currentDateTimeUtc()) < checkIntervalSeconds)
        return;

    settings.setValue(QStringLiteral("updates/lastCheckedUtc"), QDateTime::currentDateTimeUtc());

    auto* network = new QNetworkAccessManager(this);
    QNetworkRequest request(releaseApiUrl);
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("User-Agent", "PadForge-update-check");
    request.setTransferTimeout(15000);
    QNetworkReply* reply = network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, parent, network]() {
        const QByteArray response = reply->readAll();
        const bool succeeded = reply->error() == QNetworkReply::NoError;
        reply->deleteLater();
        network->deleteLater();
        if (!succeeded)
            return;

        const QJsonDocument document = QJsonDocument::fromJson(response);
        if (!document.isObject())
            return;
        const QString latest = document.object().value(QStringLiteral("tag_name")).toString();
        const QString current = QCoreApplication::applicationVersion();
        if (!isNewerVersion(current, latest))
            return;

        QSettings settings;
        if (settings.value(QStringLiteral("updates/ignoredVersion")).toString() == latest)
            return;

        auto* dialog = new QMessageBox(
            QMessageBox::Information, tr("PadForge update available"),
            tr("PadForge %1 is available. You are using version %2.").arg(latest, current),
            QMessageBox::NoButton, parent);
        QPushButton* downloadButton = dialog->addButton(tr("View update"), QMessageBox::AcceptRole);
        dialog->addButton(tr("Ignore this version"), QMessageBox::RejectRole);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowModality(Qt::NonModal);
        connect(dialog, &QMessageBox::finished, this, [dialog, downloadButton, latest]() {
            QSettings settings;
            settings.setValue(QStringLiteral("updates/ignoredVersion"), latest);
            if (dialog->clickedButton() == downloadButton)
                QDesktopServices::openUrl(releasesUrl);
        });
        dialog->open();
    });
}

} // namespace PadForge
