#include "FindReplaceDialog.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

namespace PadForge {

FindReplaceDialog::FindReplaceDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Find / Replace"));
    setModal(false);

    m_findEdit = new QLineEdit(this);
    m_findEdit->setPlaceholderText(tr("Find"));
    m_replaceEdit = new QLineEdit(this);
    m_replaceEdit->setPlaceholderText(tr("Replace with"));

    m_matchCase = new QCheckBox(tr("Match case"), this);
    m_wholeWord = new QCheckBox(tr("Whole word"), this);
    m_regex = new QCheckBox(tr("Regular expression"), this);
    m_extended = new QCheckBox(tr("Extended (\\n, \\t, \\x..)"), this);
    m_wrap = new QCheckBox(tr("Wrap around"), this);
    m_wrap->setChecked(true);
    m_dotNewline = new QCheckBox(tr(". matches newline"), this);
    m_searchUp = new QCheckBox(tr("Search up"), this);

    auto* optionsBox = new QGroupBox(tr("Options"), this);
    auto* optionsLayout = new QGridLayout(optionsBox);
    optionsLayout->addWidget(m_matchCase, 0, 0);
    optionsLayout->addWidget(m_wholeWord, 0, 1);
    optionsLayout->addWidget(m_regex, 1, 0);
    optionsLayout->addWidget(m_extended, 1, 1);
    optionsLayout->addWidget(m_wrap, 2, 0);
    optionsLayout->addWidget(m_dotNewline, 2, 1);
    optionsLayout->addWidget(m_searchUp, 3, 0);

    auto* findNext = new QPushButton(tr("Find Next"), this);
    auto* findPrev = new QPushButton(tr("Find Previous"), this);
    auto* countBtn = new QPushButton(tr("Count"), this);
    auto* markBtn = new QPushButton(tr("Mark All"), this);
    auto* replaceBtn = new QPushButton(tr("Replace"), this);
    auto* replaceAllBtn = new QPushButton(tr("Replace All"), this);

    m_status = new QLabel(this);

    auto* closeButtons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(closeButtons, &QDialogButtonBox::rejected, this, &QWidget::hide);

    auto* buttonRow = new QHBoxLayout;
    buttonRow->addWidget(findNext);
    buttonRow->addWidget(findPrev);
    buttonRow->addWidget(countBtn);
    buttonRow->addWidget(markBtn);
    buttonRow->addStretch();
    buttonRow->addWidget(replaceBtn);
    buttonRow->addWidget(replaceAllBtn);

    auto* root = new QVBoxLayout(this);
    root->addWidget(m_findEdit);
    root->addWidget(m_replaceEdit);
    root->addWidget(optionsBox);
    root->addLayout(buttonRow);
    root->addWidget(m_status);
    root->addWidget(closeButtons);

    connect(findNext, &QPushButton::clicked, this, &FindReplaceDialog::findNextRequested);
    connect(findPrev, &QPushButton::clicked, this, &FindReplaceDialog::findPreviousRequested);
    connect(countBtn, &QPushButton::clicked, this, &FindReplaceDialog::countRequested);
    connect(markBtn, &QPushButton::clicked, this, &FindReplaceDialog::markAllRequested);
    connect(replaceBtn, &QPushButton::clicked, this, &FindReplaceDialog::replaceRequested);
    connect(replaceAllBtn, &QPushButton::clicked, this, &FindReplaceDialog::replaceAllRequested);
    connect(m_findEdit, &QLineEdit::returnPressed, this, &FindReplaceDialog::findNextRequested);

    m_findEdit->setFocus();
}

QString FindReplaceDialog::term() const
{
    return m_findEdit->text();
}

QString FindReplaceDialog::replacement() const
{
    return m_replaceEdit->text();
}

SearchOptions FindReplaceDialog::options() const
{
    SearchOptions options;
    options.caseSensitive = m_matchCase->isChecked();
    options.wholeWord = m_wholeWord->isChecked();
    options.regex = m_regex->isChecked();
    options.extended = m_extended->isChecked();
    options.wrap = m_wrap->isChecked();
    options.searchUp = m_searchUp->isChecked();
    options.dotMatchesNewline = m_dotNewline->isChecked();
    return options;
}

void FindReplaceDialog::setTerm(const QString& term)
{
    m_findEdit->setText(term);
    m_findEdit->selectAll();
}

void FindReplaceDialog::showForReplaceMode(bool replace)
{
    if (replace)
        m_replaceEdit->setFocus();
    else
        m_findEdit->setFocus();
}

void FindReplaceDialog::closeEvent(QCloseEvent* event)
{
    Q_UNUSED(event);
    hide();
}

} // namespace PadForge
