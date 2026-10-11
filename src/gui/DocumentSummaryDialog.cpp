#include "DocumentSummaryDialog.h"

#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace PadForge {

DocumentSummaryDialog::DocumentSummaryDialog(const QString& summaryText, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Document Summary"));

    m_summary = new QLabel(summaryText, this);
    m_summary->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_summary->setWordWrap(false);

    auto* copy = new QPushButton(tr("Copy to Clipboard"), this);
    connect(copy, &QPushButton::clicked, this,
            [this]() { QApplication::clipboard()->setText(m_summary->text()); });

    auto* closeButtons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(closeButtons, &QDialogButtonBox::rejected, this, &QDialog::accept);

    auto* root = new QVBoxLayout(this);
    root->addWidget(m_summary);
    root->addLayout([&]() {
        auto* row = new QHBoxLayout;
        row->addWidget(copy);
        row->addStretch();
        return row;
    }());
    root->addWidget(closeButtons);
}

} // namespace PadForge