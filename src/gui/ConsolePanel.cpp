#include "ConsolePanel.h"

#include "PluginHost.h"

#include <QFontDatabase>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QToolButton>
#include <QVBoxLayout>

namespace PadForge {

ConsolePanel::ConsolePanel(PluginHost* host, QWidget* parent) : QWidget(parent), m_host(host) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);

    m_output = new QPlainTextEdit(this);
    m_output->setReadOnly(true);
    m_output->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_output->setMaximumBlockCount(5000);
    m_output->setPlaceholderText(tr("Lua console — try  padforge.line_count()  or  1+1"));
    layout->addWidget(m_output, 1);

    auto* inputRow = new QHBoxLayout;
    inputRow->setSpacing(2);
    auto* prompt = new QLabel(QStringLiteral(">"), this);
    prompt->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_input = new QLineEdit(this);
    m_input->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_input->setPlaceholderText(tr("Lua expression or statement, Enter to run (Up/Down: history)"));
    m_input->installEventFilter(this);
    connect(m_input, &QLineEdit::returnPressed, this, &ConsolePanel::runCurrentLine);
    inputRow->addWidget(prompt);
    inputRow->addWidget(m_input, 1);

    m_clearButton = new QToolButton(this);
    m_clearButton->setText(tr("Clear"));
    connect(m_clearButton, &QToolButton::clicked, this, &ConsolePanel::clear);
    inputRow->addWidget(m_clearButton);
    layout->addLayout(inputRow);

    if (m_host)
        m_host->setOutputSink([this](const QString& line) { appendLine(line); });

    appendLine(tr("PadForge Lua console. Type Lua code and press Enter."));
}

void ConsolePanel::clear() {
    m_output->clear();
}

void ConsolePanel::appendLine(const QString& line) {
    m_output->appendPlainText(line);
}

void ConsolePanel::appendError(const QString& error) {
    m_output->appendHtml(
        QStringLiteral("<span style=\"color:#c0392b;\">%1</span>").arg(error.toHtmlEscaped()));
}

void ConsolePanel::runCurrentLine() {
    const QString code = m_input->text();
    if (code.trimmed().isEmpty())
        return;

    appendLine(QStringLiteral("> ") + code);
    if (m_history.isEmpty() || m_history.last() != code)
        m_history.append(code);
    m_historyIndex = m_history.size();

    QString result;
    QString error;
    if (m_host->evaluateLua(code, &result, &error)) {
        if (!result.isEmpty())
            appendLine(result);
    } else {
        appendError(error);
    }
    m_input->clear();
}

void ConsolePanel::navigateHistory(int delta) {
    if (m_history.isEmpty())
        return;
    m_historyIndex = qBound(0, m_historyIndex + delta, m_history.size());
    if (m_historyIndex == m_history.size())
        m_input->clear();
    else
        m_input->setText(m_history.at(m_historyIndex));
    m_input->end(false);
}

bool ConsolePanel::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_input && event->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(event);
        if (key->key() == Qt::Key_Up) {
            navigateHistory(-1);
            return true;
        }
        if (key->key() == Qt::Key_Down) {
            navigateHistory(1);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace PadForge
