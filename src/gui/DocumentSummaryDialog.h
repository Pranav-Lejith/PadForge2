#pragma once

#include <QDialog>

class QLabel;

namespace PadForge {

// Displays document statistics collected by the caller.
class DocumentSummaryDialog : public QDialog {
    Q_OBJECT

public:
    explicit DocumentSummaryDialog(const QString& summaryText, QWidget* parent = nullptr);

private:
    QLabel* m_summary = nullptr;
};

} // namespace PadForge