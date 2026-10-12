#pragma once

#include <QStringList>
#include <QWidget>

class QLineEdit;
class QPlainTextEdit;
class QToolButton;

namespace PadForge {

class PluginHost;

// Interactive Lua console dock. It evaluates input in the same Lua state the
// plugins use (the `padforge` table is available), captures `print(...)` output
// and keeps a command history.
class ConsolePanel : public QWidget {
    Q_OBJECT

public:
    explicit ConsolePanel(PluginHost* host, QWidget* parent = nullptr);

    void clear();
    void appendLine(const QString& line);

public slots:
    void runCurrentLine();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void appendError(const QString& error);
    void navigateHistory(int delta);

    PluginHost* m_host = nullptr;
    QPlainTextEdit* m_output = nullptr;
    QLineEdit* m_input = nullptr;
    QToolButton* m_clearButton = nullptr;
    QStringList m_history;
    int m_historyIndex = 0;
};

} // namespace PadForge
