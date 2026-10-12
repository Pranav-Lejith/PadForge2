#include "DocumentMap.h"

#include "EditorWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <Scintilla.h>
#include <ScintillaEdit.h>
#include <algorithm>

namespace PadForge {

DocumentMap::DocumentMap(QWidget* parent) : QWidget(parent) {
    setMinimumWidth(70);
    setMouseTracking(true);
    m_timer.setSingleShot(true);
    m_timer.setInterval(150);
    connect(&m_timer, &QTimer::timeout, this, &DocumentMap::refresh);
}

void DocumentMap::attachEditor(EditorWidget* editor) {
    detachEditor();
    m_editor = editor;
    if (!m_editor)
        return;
    connect(m_editor, &EditorWidget::cursorPositionChanged, this, &DocumentMap::scheduleRefresh);
    connect(m_editor, &EditorWidget::modificationChanged, this, &DocumentMap::scheduleRefresh);
    connect(m_editor, &EditorWidget::languageChanged, this, &DocumentMap::scheduleRefresh);
    scheduleRefresh();
}

void DocumentMap::detachEditor() {
    if (m_editor)
        QObject::disconnect(m_editor, nullptr, this, nullptr);
    m_editor = nullptr;
    update();
}

void DocumentMap::refresh() {
    update();
}

void DocumentMap::scheduleRefresh() {
    m_timer.start();
}

int DocumentMap::lineAt(const QPoint& position) const {
    if (!m_editor || height() <= 0)
        return 0;
    const int totalLines = int(m_editor->editor()->lineCount());
    if (totalLines <= 0)
        return 0;
    const int y = std::clamp(position.y(), 0, height() - 1);
    return int(qint64(y) * totalLines / height());
}

void DocumentMap::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    const QRect area = rect().adjusted(1, 1, -1, -1);
    painter.fillRect(area,
                     m_editor ? m_editor->documentBackground() : palette().color(QPalette::Base));
    if (!m_editor)
        return;

    ScintillaEdit* sci = m_editor->editor();
    const int totalLines = int(sci->lineCount());
    if (totalLines <= 0 || area.height() <= 0)
        return;

    const int maxBlocks = std::max(1, area.height());
    const int step = std::max(1, totalLines / maxBlocks);
    for (int line = 0; line < totalLines; line += step) {
        const int y0 = int(qint64(line) * area.height() / totalLines);
        const int y1 =
            std::min(area.height(), int(qint64(line + step) * area.height() / totalLines));
        if (y1 <= y0)
            continue;
        painter.fillRect(QRect(area.left(), area.top() + y0, area.width(), y1 - y0),
                         m_editor->colorForLine(line));
    }

    // Viewport indicator.
    const int first = m_editor->firstVisibleLine();
    const int visible = std::max(1, m_editor->linesOnScreen());
    const int vpTop = int(qint64(first) * area.height() / totalLines);
    const int vpBottom =
        std::min(area.height(), int(qint64(first + visible) * area.height() / totalLines));
    painter.fillRect(
        QRect(area.left(), area.top() + vpTop, area.width(), std::max(2, vpBottom - vpTop)),
        QColor(255, 255, 255, 40));
    painter.setPen(QPen(QColor(255, 255, 255, 120), 0));
    painter.drawRect(QRect(area.left(), area.top() + vpTop, area.width() - 1,
                           std::max(2, vpBottom - vpTop) - 1));

    // Caret line.
    const int caret = m_editor->currentLine() - 1;
    const int caretY = int(qint64(caret) * area.height() / totalLines);
    painter.fillRect(QRect(area.left(), area.top() + caretY, area.width(), 1),
                     QColor(255, 255, 255, 160));
}

void DocumentMap::mousePressEvent(QMouseEvent* event) {
    if (!m_editor)
        return;
    m_dragging = true;
    m_editor->gotoLine(lineAt(event->pos()) + 1);
    m_editor->setFocus();
}

void DocumentMap::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging && m_editor)
        m_editor->gotoLine(lineAt(event->pos()) + 1);
}

void DocumentMap::mouseReleaseEvent(QMouseEvent* event) {
    Q_UNUSED(event);
    m_dragging = false;
}

void DocumentMap::wheelEvent(QWheelEvent* event) {
    if (!m_editor)
        return;
    const int lines = -event->angleDelta().y() / 120;
    if (lines != 0)
        m_editor->editor()->send(SCI_LINESCROLL, 0, uptr_t(std::clamp(lines, -20, 20)));
    event->accept();
}

void DocumentMap::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    scheduleRefresh();
}

} // namespace PadForge