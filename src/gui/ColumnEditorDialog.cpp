#include "ColumnEditorDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

namespace PadForge {

ColumnEditorDialog::ColumnEditorDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Column Editor"));
    build();
}

void ColumnEditorDialog::build() {
    m_mode = new QComboBox(this);
    m_mode->addItem(tr("Insert Text"));
    m_mode->addItem(tr("Insert Numbers"));

    m_column = new QSpinBox(this);
    m_column->setRange(1, 1000);
    m_column->setValue(1);
    m_column->setToolTip(tr("Column at which the insertion begins (1-based)"));

    m_text = new QLineEdit(this);
    m_text->setPlaceholderText(tr("Text to insert"));

    m_initial = new QSpinBox(this);
    m_initial->setRange(-1000000, 1000000);
    m_initial->setValue(1);
    m_step = new QSpinBox(this);
    m_step->setRange(-1000000, 1000000);
    m_step->setValue(1);
    m_padding = new QSpinBox(this);
    m_padding->setRange(1, 12);
    m_padding->setValue(2);
    m_padding->setToolTip(tr("Zero-padding width for the numbers"));

    m_fromLine = new QSpinBox(this);
    m_fromLine->setRange(0, 1000000);
    m_fromLine->setValue(0);
    m_fromLine->setSpecialValueText(tr("First line"));
    m_toLine = new QSpinBox(this);
    m_toLine->setRange(-1, 1000000);
    m_toLine->setValue(-1);
    m_toLine->setSpecialValueText(tr("Last line"));

    m_repeatIntoEmpty = new QCheckBox(tr("Repeat into empty lines"), this);
    m_positionEnd = new QCheckBox(tr("Insert at line start/end of selection only"), this);

    auto* group = new QGroupBox(tr("Options"), this);
    auto* form = new QFormLayout(group);
    form->addRow(tr("Mode:"), m_mode);
    form->addRow(tr("Column:"), m_column);
    form->addRow(tr("Text:"), m_text);
    form->addRow(tr("Initial number:"), m_initial);
    form->addRow(tr("Step:"), m_step);
    form->addRow(tr("Padding width:"), m_padding);
    form->addRow(tr("From line:"), m_fromLine);
    form->addRow(tr("To line:"), m_toLine);
    form->addRow(tr(""), m_repeatIntoEmpty);
    form->addRow(tr(""), m_positionEnd);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* root = new QVBoxLayout(this);
    root->addWidget(group);
    root->addWidget(buttons);
}

bool ColumnEditorDialog::insertTextMode() const {
    return m_mode->currentIndex() == 0;
}

int ColumnEditorDialog::column() const {
    return m_column->value();
}

QString ColumnEditorDialog::text() const {
    return m_text->text();
}

int ColumnEditorDialog::initialNumber() const {
    return m_initial->value();
}

int ColumnEditorDialog::step() const {
    return m_step->value();
}

int ColumnEditorDialog::paddingWidth() const {
    return m_padding->value();
}

int ColumnEditorDialog::fromLine() const {
    return m_fromLine->value();
}

int ColumnEditorDialog::toLine() const {
    return m_toLine->value();
}

bool ColumnEditorDialog::repeatIntoEmptyLines() const {
    return m_repeatIntoEmpty->isChecked();
}

bool ColumnEditorDialog::positionAtEnd() const {
    return m_positionEnd->isChecked();
}

} // namespace PadForge