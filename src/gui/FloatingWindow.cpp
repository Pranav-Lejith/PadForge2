#include "FloatingWindow.h"

#include "EditorWidget.h"
#include "MainWindow.h"

#include <QCloseEvent>
#include <QFileInfo>
#include <QMenu>
#include <QMenuBar>
#include <QTabWidget>

namespace PadForge {

FloatingWindow::FloatingWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(tr("PadForge"));
    resize(720, 520);

    m_tabs = new QTabWidget(this);
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);
    m_tabs->setDocumentMode(true);
    setCentralWidget(m_tabs);

    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    QAction* reattach = fileMenu->addAction(tr("Reattach to Main Window"));
    reattach->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+D")));
    connect(reattach, &QAction::triggered, this, &FloatingWindow::reattachAll);
    fileMenu->addSeparator();
    QAction* close = fileMenu->addAction(tr("&Close"));
    connect(close, &QAction::triggered, this, &FloatingWindow::close);

    connect(m_tabs, &QTabWidget::tabCloseRequested, this, &FloatingWindow::onTabCloseRequested);
    connect(m_tabs, &QTabWidget::currentChanged, this, &FloatingWindow::updateTitle);
}

void FloatingWindow::addEditor(EditorWidget* editor, MainWindow* mainWindow) {
    m_mainWindow = mainWindow;
    m_tabs->addTab(editor, tr("Untitled"));
    m_tabs->setCurrentWidget(editor);
    editor->setFocus();
    updateTitle();
}

QList<EditorWidget*> FloatingWindow::editors() const {
    QList<EditorWidget*> result;
    for (int i = 0; i < m_tabs->count(); ++i) {
        if (auto* editor = qobject_cast<EditorWidget*>(m_tabs->widget(i)))
            result.append(editor);
    }
    return result;
}

void FloatingWindow::reattachEditor(EditorWidget* editor) {
    if (!m_mainWindow || !editor)
        return;
    m_mainWindow->reattachToMain(editor);
}

void FloatingWindow::reattachAll() {
    const QList<EditorWidget*> list = editors();
    for (EditorWidget* editor : list)
        reattachEditor(editor);
    if (m_tabs->count() == 0)
        close();
}

void FloatingWindow::onTabCloseRequested(int index) {
    if (auto* editor = qobject_cast<EditorWidget*>(m_tabs->widget(index)))
        reattachEditor(editor);
    updateTitle();
}

void FloatingWindow::closeEvent(QCloseEvent* event) {
    const QList<EditorWidget*> list = editors();
    for (EditorWidget* editor : list)
        reattachEditor(editor);
    event->accept();
}

void FloatingWindow::updateTitle() {
    EditorWidget* editor = qobject_cast<EditorWidget*>(m_tabs->currentWidget());
    QString name = tr("PadForge");
    if (editor && !editor->filePath().isEmpty())
        name = QStringLiteral("%1 - PadForge").arg(QFileInfo(editor->filePath()).fileName());
    setWindowTitle(name);
}

} // namespace PadForge