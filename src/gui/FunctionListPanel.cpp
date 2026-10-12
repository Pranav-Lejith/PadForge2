#include "FunctionListPanel.h"

#include "EditorWidget.h"
#include "Outline.h"

#include <QVector>

namespace PadForge {

namespace {

QString kindLabel(const QString& kind) {
    if (kind == QLatin1String("class") || kind == QLatin1String("struct") ||
        kind == QLatin1String("interface"))
        return QStringLiteral("[C] ");
    if (kind == QLatin1String("section"))
        return QStringLiteral("[S] ");
    return QStringLiteral("[F] ");
}

} // namespace

FunctionListPanel::FunctionListPanel(QWidget* parent) : QTreeWidget(parent) {
    setHeaderHidden(true);
    setUniformRowHeights(true);
    setRootIsDecorated(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setIndentation(12);
    setExpandsOnDoubleClick(false);

    m_timer.setSingleShot(true);
    m_timer.setInterval(250);
    connect(&m_timer, &QTimer::timeout, this, &FunctionListPanel::refresh);

    connect(this, &QTreeWidget::itemActivated, this, &FunctionListPanel::activateItem);
    connect(this, &QTreeWidget::itemDoubleClicked, this, &FunctionListPanel::activateItem);
}

void FunctionListPanel::attachEditor(EditorWidget* editor) {
    detachEditor();
    m_editor = editor;
    if (!m_editor)
        return;
    connect(m_editor, &EditorWidget::textChanged, this, &FunctionListPanel::scheduleRefresh);
    connect(m_editor, &EditorWidget::languageChanged, this, &FunctionListPanel::scheduleRefresh);
    connect(m_editor, &QObject::destroyed, this, &FunctionListPanel::detachEditor);
    scheduleRefresh();
}

void FunctionListPanel::detachEditor() {
    if (m_editor)
        QObject::disconnect(m_editor, nullptr, this, nullptr);
    m_editor = nullptr;
    clear();
}

void FunctionListPanel::scheduleRefresh() {
    m_timer.start();
}

void FunctionListPanel::refresh() {
    clear();
    if (!m_editor)
        return;
    const QList<OutlineItem> items =
        OutlineParser::parse(m_editor->documentText(), m_editor->languageId());
    if (items.isEmpty())
        return;

    QVector<QTreeWidgetItem*> stack;
    for (const OutlineItem& entry : items) {
        while (!stack.isEmpty() && stack.size() > entry.level)
            stack.removeLast();
        auto* item = new QTreeWidgetItem;
        item->setText(0, kindLabel(entry.kind) + entry.name);
        item->setToolTip(0, entry.signature);
        item->setData(0, Qt::UserRole, entry.line);
        if (stack.isEmpty())
            addTopLevelItem(item);
        else
            stack.last()->addChild(item);
        stack.append(item);
    }
    if (topLevelItemCount() <= 40)
        expandAll();
}

void FunctionListPanel::activateItem(QTreeWidgetItem* item) {
    if (!m_editor || !item)
        return;
    const int line = item->data(0, Qt::UserRole).toInt();
    if (line > 0) {
        m_editor->gotoLine(line);
        m_editor->setFocus();
    }
}

} // namespace PadForge
