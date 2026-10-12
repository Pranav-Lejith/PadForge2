#include "MainWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QIcon>
#include <QStringList>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("PadForge"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QCoreApplication::setOrganizationName(QStringLiteral("PadForge"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("padforge.local"));
    QApplication::setApplicationDisplayName(QStringLiteral("PadForge"));
    QGuiApplication::setDesktopFileName(QStringLiteral("org.padforge.PadForge"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("PadForge — a fast, native text editor inspired by Notepad++."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("files"),
                                 QStringLiteral("Files to open."),
                                 QStringLiteral("[files...]"));
    parser.process(app);

    PadForge::MainWindow window;
    window.show();

    const QStringList files = parser.positionalArguments();
    if (!files.isEmpty()) {
        QStringList open;
        for (const QString& file : files)
            open << QFileInfo(file).absoluteFilePath();
        window.openPaths(open);
    }

    return app.exec();
}
