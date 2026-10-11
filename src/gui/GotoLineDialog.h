#pragma once

#include <QDialog>

class QSpinBox;

namespace PadForge {

// Go-to-line dialog.
class GotoLineDialog : public QDialog {
    Q_OBJECT

public:
    explicit GotoLineDialog(int maximumLine, QWidget* parent = nullptr);

    int lineNumber() const;

private:
    QSpinBox* m_line = nullptr;
};

} // namespace PadForge