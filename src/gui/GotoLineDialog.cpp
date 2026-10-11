#include "GotoLineDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

namespace PadForge {

GotoLineDialog::GotoLineDialog(int maximumLine, QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Go to Line"));

    m_line = new QSpinBox(this);
    m_line->setRange(1, qMax(1, maximumLine));
    m_line->setValue(1);
    m_line->selectAll();

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_line, &QSpinBox::editingFinished, this, &QDialog::accept);

    auto* root = new QVBoxLayout(this);
    root->addWidget(new QLabel(tr("Line number (1..%1):").arg(maximumLine), this));
    root->addWidget(m_line);
    root->addWidget(buttons);
    m_line->setFocus();
}

int GotoLineDialog::lineNumber() const {
    return m_line->value();
}

} // namespace PadForge