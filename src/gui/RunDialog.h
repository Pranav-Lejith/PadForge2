#pragma once

#include <QDialog>

class QLineEdit;
class QPushButton;
class QTextEdit;

namespace PadForge {

// Run a command from inside the editor. `$(VAR)` placeholders are expanded
// using the given file path and the `FULL_CURRENT_PATH` style variables.
class RunDialog : public QDialog {
    Q_OBJECT

public:
    explicit RunDialog(const QString& currentFilePath, QWidget* parent = nullptr);

private:
    void run();
    void expandVariables(QString& command) const;

    const QString m_currentFilePath;
    QLineEdit* m_command = nullptr;
    QTextEdit* m_output = nullptr;
    QPushButton* m_runButton = nullptr;
};

} // namespace PadForge