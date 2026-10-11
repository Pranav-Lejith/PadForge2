#pragma once

#include "SearchEngine.h"

#include <QDialog>

class QCheckBox;
class QLabel;
class QLineEdit;

namespace PadForge {

// Non-modal find / replace dialog. It only reports actions; MainWindow applies
// them to the active document.
class FindReplaceDialog : public QDialog {
    Q_OBJECT

public:
    explicit FindReplaceDialog(QWidget* parent = nullptr);

    QString term() const;
    QString replacement() const;
    SearchOptions options() const;

    void setTerm(const QString& term);
    void showForReplaceMode(bool replace);

signals:
    void findNextRequested();
    void findPreviousRequested();
    void replaceRequested();
    void replaceAllRequested();
    void countRequested();
    void markAllRequested();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    QLineEdit* m_findEdit = nullptr;
    QLineEdit* m_replaceEdit = nullptr;
    QCheckBox* m_matchCase = nullptr;
    QCheckBox* m_wholeWord = nullptr;
    QCheckBox* m_regex = nullptr;
    QCheckBox* m_extended = nullptr;
    QCheckBox* m_wrap = nullptr;
    QCheckBox* m_dotNewline = nullptr;
    QCheckBox* m_searchUp = nullptr;
    QLabel* m_status = nullptr;
};

} // namespace PadForge
