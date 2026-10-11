#include "RunDialog.h"

#include <QDialogButtonBox>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

namespace PadForge {

namespace {

void replaceAll(QString& text, const QString& variable, const QString& value) {
    text.replace(variable, value);
}

} // namespace

RunDialog::RunDialog(const QString& currentFilePath, QWidget* parent)
    : QDialog(parent), m_currentFilePath(currentFilePath) {
    setWindowTitle(tr("Run"));
    resize(560, 360);

    m_command = new QLineEdit(this);
    m_command->setPlaceholderText(tr("Command, e.g. $(NPP_FULL) --file $(FULL_CURRENT_PATH)"));

    m_output = new QTextEdit(this);
    m_output->setReadOnly(true);
    m_output->setVisible(false);

    m_runButton = new QPushButton(tr("Run"), this);

    auto* closeButtons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(closeButtons, &QDialogButtonBox::rejected, this, &QDialog::accept);

    auto* row = new QHBoxLayout;
    row->addWidget(new QLabel(tr("Command:"), this));
    row->addWidget(m_command, 1);
    row->addWidget(m_runButton);

    auto* root = new QVBoxLayout(this);
    root->addWidget(new QLabel(tr("Variables: $CURRENT_DIRECTORY, $(FULL_CURRENT_PATH), "
                                  "$(CURRENT_FILENAME), $(NAME_PART), $(EXT_PART)."),
                               this));
    root->addLayout(row);
    root->addWidget(m_output);
    root->addWidget(closeButtons);

    connect(m_runButton, &QPushButton::clicked, this, &RunDialog::run);
    connect(m_command, &QLineEdit::returnPressed, this, &RunDialog::run);
    m_command->setFocus();
}

void RunDialog::expandVariables(QString& command) const {
    replaceAll(command, QStringLiteral("$(FULL_CURRENT_PATH)"), m_currentFilePath);
    replaceAll(command, QStringLiteral("$(CURRENT_DIRECTORY)"),
               m_currentFilePath.isEmpty() ? QString()
                                           : QFileInfo(m_currentFilePath).absolutePath());
    const QString fileName =
        m_currentFilePath.isEmpty() ? QString() : QFileInfo(m_currentFilePath).fileName();
    replaceAll(command, QStringLiteral("$(CURRENT_FILENAME)"), fileName);
    replaceAll(command, QStringLiteral("$(NAME_PART)"), QFileInfo(fileName).completeBaseName());
    replaceAll(command, QStringLiteral("$(EXT_PART)"), QFileInfo(fileName).suffix());
    replaceAll(command, QStringLiteral("$CURRENT_DIRECTORY"),
               m_currentFilePath.isEmpty() ? QString()
                                           : QFileInfo(m_currentFilePath).absolutePath());
}

void RunDialog::run() {
    QString command = m_command->text().trimmed();
    if (command.isEmpty())
        return;
    expandVariables(command);

    QProcess process;
    process.setStandardOutputFile(QProcess::nullDevice());
    process.start(QStringLiteral("sh"), {QStringLiteral("-c"), command});
    if (!process.waitForFinished(30 * 1000)) {
        m_output->setVisible(true);
        m_output->setPlainText(tr("Command timed out."));
        return;
    }
    const QByteArray stdoutData = process.readAllStandardOutput();
    const QByteArray stderrData = process.readAllStandardError();
    const QString text = QString::fromUtf8(stdoutData) + QString::fromUtf8(stderrData);
    m_output->setVisible(true);
    if (text.isEmpty()) {
        m_output->setPlainText(tr("Exit code: %1").arg(process.exitCode()));
    } else {
        m_output->setPlainText(text + QStringLiteral("\n\nexit code: %1").arg(process.exitCode()));
    }
}

} // namespace PadForge