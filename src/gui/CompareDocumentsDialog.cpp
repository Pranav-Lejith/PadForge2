#include "CompareDocumentsDialog.h"

#include "EditorWidget.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QVBoxLayout>

CompareDocumentsDialog::CompareDocumentsDialog(const QList<PadForge::EditorWidget*>& documents,
                                               PadForge::EditorWidget* current, QWidget* parent)
    : QDialog(parent), m_documents(documents) {
    setWindowTitle(tr("Compare Documents"));
    auto* layout = new QVBoxLayout(this);
    auto* form = new QFormLayout;

    m_first = new QComboBox(this);
    m_second = new QComboBox(this);
    int currentIndex = 0;
    int otherIndex = -1;
    for (int i = 0; i < m_documents.size(); ++i) {
        const PadForge::EditorWidget* editor = m_documents.at(i);
        const QString label = editor->filePath().isEmpty()
                                  ? tr("Untitled")
                                  : QFileInfo(editor->filePath()).fileName();
        m_first->addItem(label, i);
        m_second->addItem(label, i);
        if (editor == current)
            currentIndex = i;
        else if (otherIndex < 0)
            otherIndex = i;
    }
    m_first->setCurrentIndex(currentIndex);
    m_second->setCurrentIndex(otherIndex >= 0 ? otherIndex : currentIndex);

    form->addRow(tr("Document &A:"), m_first);
    form->addRow(tr("Document &B:"), m_second);
    layout->addLayout(form);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

PadForge::EditorWidget* CompareDocumentsDialog::firstDocument() const {
    return m_documents.value(m_first->currentData().toInt());
}

PadForge::EditorWidget* CompareDocumentsDialog::secondDocument() const {
    return m_documents.value(m_second->currentData().toInt());
}