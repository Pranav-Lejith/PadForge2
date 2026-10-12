#include "SearchResultsPanel.h"

#include "EditorWidget.h"

#include <QFileInfo>
#include <QLabel>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace PadForge {

SearchResultsPanel::SearchResultsPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_summary = new QLabel(this);
    m_summary->setContentsMargins(6, 6, 6, 2);
    layout->addWidget(m_summary);

    m_results = new QTreeWidget(this);
    m_results->setHeaderLabels({tr("Document"), tr("Line"), tr("Column"), tr("Text")});
    m_results->setRootIsDecorated(true);
    m_results->setUniformRowHeights(true);
    layout->addWidget(m_results);

    connect(m_results, &QTreeWidget::itemActivated, this, &SearchResultsPanel::activateItem);
    connect(m_results, &QTreeWidget::itemDoubleClicked, this, &SearchResultsPanel::activateItem);

    clear();
}

void SearchResultsPanel::begin(const QString& term) {
    clear();
    m_summary->setText(tr("Results for \"%1\"").arg(term));
}

void SearchResultsPanel::addHit(EditorWidget* document, int line, int column, const QString& text) {
    if (!document || line < 1)
        return;
    QTreeWidgetItem* group = m_groups.value(document);
    if (!group) {
        const QString name = document->filePath().isEmpty()
                                 ? tr("Untitled")
                                 : QFileInfo(document->filePath()).fileName();
        group = new QTreeWidgetItem(m_results);
        group->setText(0, name);
        group->setData(0, Qt::UserRole, quintptr(document));
        m_groups.insert(document, group);
    }
    auto* child = new QTreeWidgetItem(group);
    child->setText(1, QString::number(line));
    child->setText(2, QString::number(column));
    child->setText(3, text);
    child->setData(0, Qt::UserRole, quintptr(document));
    child->setData(0, Qt::UserRole + 1, line);
    if (m_groups.size() == 1)
        group->setExpanded(true);
}

void SearchResultsPanel::finish(int total) {
    m_summary->setText(tr("%1 match(es) in %2 document(s). Double-click a row to jump.")
                           .arg(total)
                           .arg(m_groups.size()));
}

void SearchResultsPanel::clear() {
    m_results->clear();
    m_groups.clear();
    m_summary->clear();
}

void SearchResultsPanel::activateItem(QTreeWidgetItem* item, int) {
    if (!item)
        return;
    const quintptr raw = item->data(0, Qt::UserRole).toULongLong();
    auto* document = reinterpret_cast<EditorWidget*>(raw);
    const int line = item->data(0, Qt::UserRole + 1).toInt();
    if (document && line > 0)
        emit resultActivated(document, line);
}

} // namespace PadForge