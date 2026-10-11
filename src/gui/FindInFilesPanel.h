#pragma once

#include <QWidget>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

namespace PadForge {

class FindInFilesPanel : public QWidget {
    Q_OBJECT

public:
    explicit FindInFilesPanel(QWidget* parent = nullptr);

    void setDirectory(const QString& directory);

signals:
    void resultActivated(const QString& file, int line);

private slots:
    void browse();
    void runSearch();
    void clearResults();
    void itemActivated(QTreeWidgetItem* item, int column);

private:
    QStringList nameFilters() const;

    QLineEdit* m_directory = nullptr;
    QLineEdit* m_filter = nullptr;
    QLineEdit* m_term = nullptr;
    QCheckBox* m_matchCase = nullptr;
    QCheckBox* m_wholeWord = nullptr;
    QCheckBox* m_regex = nullptr;
    QPushButton* m_searchButton = nullptr;
    QLabel* m_summary = nullptr;
    QTreeWidget* m_results = nullptr;
};

} // namespace PadForge
