#pragma once

#include <QDialog>
#include <QList>

class QComboBox;

namespace PadForge {
class EditorWidget;
}

class CompareDocumentsDialog : public QDialog {
    Q_OBJECT

public:
    explicit CompareDocumentsDialog(const QList<PadForge::EditorWidget*>& documents,
                                    PadForge::EditorWidget* current, QWidget* parent = nullptr);

    PadForge::EditorWidget* firstDocument() const;
    PadForge::EditorWidget* secondDocument() const;

private:
    QList<PadForge::EditorWidget*> m_documents;
    QComboBox* m_first = nullptr;
    QComboBox* m_second = nullptr;
};