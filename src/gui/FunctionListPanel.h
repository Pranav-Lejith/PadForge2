#pragma once

#include <QTimer>
#include <QTreeWidget>

namespace PadForge {

class EditorWidget;

// A document outline ("Function List") built from the active editor's text.
class FunctionListPanel : public QTreeWidget {
    Q_OBJECT

public:
    explicit FunctionListPanel(QWidget* parent = nullptr);

    void attachEditor(EditorWidget* editor);
    void detachEditor();
    EditorWidget* editor() const { return m_editor; }

public slots:
    void refresh();

private:
    void scheduleRefresh();
    void activateItem(QTreeWidgetItem* item);

    EditorWidget* m_editor = nullptr;
    QTimer m_timer;
};

} // namespace PadForge
