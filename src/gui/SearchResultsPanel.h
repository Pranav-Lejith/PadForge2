#pragma once

#include <QHash>
#include <QWidget>

class QLabel;
class QTreeWidget;
class QTreeWidgetItem;

namespace PadForge {

class EditorWidget;

// Dock widget listing matches from Find All in the current document or in all
// open documents. Results are grouped per document with click-to-jump.
class SearchResultsPanel : public QWidget {
    Q_OBJECT

public:
    explicit SearchResultsPanel(QWidget* parent = nullptr);

    void begin(const QString& term);
    void addHit(EditorWidget* document, int line, int column, const QString& text);
    void finish(int total);
    void clear();

signals:
    void resultActivated(EditorWidget* document, int line);

private slots:
    void activateItem(QTreeWidgetItem* item, int column);

private:
    QLabel* m_summary = nullptr;
    QTreeWidget* m_results = nullptr;
    QHash<EditorWidget*, QTreeWidgetItem*> m_groups;
};

} // namespace PadForge