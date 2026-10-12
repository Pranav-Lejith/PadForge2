#pragma once

#include <QList>
#include <QMainWindow>

class QTabWidget;

namespace PadForge {

class EditorWidget;
class MainWindow;

// A floating window hosting tabs detached from the main window. Editors remain
// registered with the main window and are reattached when the window closes.
class FloatingWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit FloatingWindow(QWidget* parent = nullptr);

    void addEditor(EditorWidget* editor, MainWindow* mainWindow);
    QList<EditorWidget*> editors() const;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void reattachAll();
    void onTabCloseRequested(int index);

private:
    void updateTitle();
    void reattachEditor(EditorWidget* editor);

    QTabWidget* m_tabs = nullptr;
    MainWindow* m_mainWindow = nullptr;
};

} // namespace PadForge