#pragma once

#include <QTimer>
#include <QWidget>

class QMouseEvent;
class QPaintEvent;
class QResizeEvent;
class QWheelEvent;

namespace PadForge {

class EditorWidget;

// A compact whole-document overview rendered from the active editor's styles.
class DocumentMap : public QWidget {
    Q_OBJECT

public:
    explicit DocumentMap(QWidget* parent = nullptr);

    void attachEditor(EditorWidget* editor);
    void detachEditor();
    EditorWidget* editor() const { return m_editor; }

public slots:
    void refresh();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void scheduleRefresh();
    int lineAt(const QPoint& position) const;

    EditorWidget* m_editor = nullptr;
    QTimer m_timer;
    bool m_dragging = false;
};

} // namespace PadForge