#include "FindInFilesPanel.h"

#include "SearchEngine.h"

#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QFileDialog>
#include <QRegularExpression>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace PadForge {

FindInFilesPanel::FindInFilesPanel(QWidget* parent)
    : QWidget(parent)
{
    m_directory = new QLineEdit(this);
    m_directory->setPlaceholderText(tr("Directory"));
    auto* browseButton = new QPushButton(tr("..."), this);
    browseButton->setFixedWidth(32);

    m_filter = new QLineEdit(this);
    m_filter->setPlaceholderText(tr("*.cpp;*.h;*.txt"));
    m_filter->setText(QStringLiteral("*.*"));

    m_term = new QLineEdit(this);
    m_term->setPlaceholderText(tr("Find in files"));

    m_matchCase = new QCheckBox(tr("Match case"), this);
    m_wholeWord = new QCheckBox(tr("Whole word"), this);
    m_regex = new QCheckBox(tr("Regex"), this);

    m_searchButton = new QPushButton(tr("Find All"), this);
    auto* clearButton = new QPushButton(tr("Clear"), this);

    auto* dirRow = new QHBoxLayout;
    dirRow->addWidget(m_directory);
    dirRow->addWidget(browseButton);

    auto* optionRow = new QHBoxLayout;
    optionRow->addWidget(m_matchCase);
    optionRow->addWidget(m_wholeWord);
    optionRow->addWidget(m_regex);
    optionRow->addStretch();
    optionRow->addWidget(m_searchButton);
    optionRow->addWidget(clearButton);

    m_results = new QTreeWidget(this);
    m_results->setHeaderLabels({tr("File"), tr("Line"), tr("Text")});
    m_results->setRootIsDecorated(true);
    m_results->setColumnWidth(0, 220);
    m_results->setColumnWidth(1, 60);

    m_summary = new QLabel(this);

    auto* root = new QVBoxLayout(this);
    root->addLayout(dirRow);
    root->addWidget(m_filter);
    root->addWidget(m_term);
    root->addLayout(optionRow);
    root->addWidget(m_summary);
    root->addWidget(m_results, 1);

    connect(browseButton, &QPushButton::clicked, this, &FindInFilesPanel::browse);
    connect(m_searchButton, &QPushButton::clicked, this, &FindInFilesPanel::runSearch);
    connect(m_term, &QLineEdit::returnPressed, this, &FindInFilesPanel::runSearch);
    connect(clearButton, &QPushButton::clicked, this, &FindInFilesPanel::clearResults);
    connect(m_results, &QTreeWidget::itemActivated, this, &FindInFilesPanel::itemActivated);
    connect(m_results, &QTreeWidget::itemDoubleClicked, this, &FindInFilesPanel::itemActivated);
}

void FindInFilesPanel::setDirectory(const QString& directory)
{
    m_directory->setText(directory);
}

QStringList FindInFilesPanel::nameFilters() const
{
    const QString raw = m_filter->text();
    QStringList filters;
    for (QString part : raw.split(QRegularExpression(QStringLiteral("[;, ]+")), Qt::SkipEmptyParts)) {
        if (part == QLatin1String("*.*"))
            part = QStringLiteral("*");
        filters.append(part);
    }
    if (filters.isEmpty())
        filters.append(QStringLiteral("*"));
    return filters;
}

void FindInFilesPanel::browse()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Choose directory"),
                                                          m_directory->text());
    if (!dir.isEmpty())
        m_directory->setText(dir);
}

void FindInFilesPanel::runSearch()
{
    const QString term = m_term->text();
    if (term.isEmpty() || m_directory->text().isEmpty())
        return;

    SearchOptions options;
    options.caseSensitive = m_matchCase->isChecked();
    options.wholeWord = m_wholeWord->isChecked();
    options.regex = m_regex->isChecked();

    QApplication::setOverrideCursor(Qt::WaitCursor);
    const QList<FileSearchResult> results =
        FileSearcher::searchInDirectory(m_directory->text(), nameFilters(), term, options, true);
    QApplication::restoreOverrideCursor();

    clearResults();
    QTreeWidgetItem* currentFileItem = nullptr;
    QString currentFile;
    for (const FileSearchResult& r : results) {
        if (r.file != currentFile) {
            currentFile = r.file;
            currentFileItem = new QTreeWidgetItem(m_results);
            currentFileItem->setText(0, r.file);
            currentFileItem->setExpanded(true);
        }
        auto* child = new QTreeWidgetItem(currentFileItem);
        child->setText(1, QString::number(r.line));
        child->setText(2, r.text.trimmed());
        child->setData(0, Qt::UserRole, r.file);
        child->setData(0, Qt::UserRole + 1, r.line);
    }
    m_summary->setText(tr("%1 matches in %2 files")
                           .arg(results.size())
                           .arg(m_results->topLevelItemCount()));
}

void FindInFilesPanel::clearResults()
{
    m_results->clear();
    m_summary->clear();
}

void FindInFilesPanel::itemActivated(QTreeWidgetItem* item, int)
{
    if (!item)
        return;
    const QString file = item->data(0, Qt::UserRole).toString();
    const int line = item->data(0, Qt::UserRole + 1).toInt();
    if (!file.isEmpty())
        emit resultActivated(file, line);
}

} // namespace PadForge
