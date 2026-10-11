#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QSpinBox;

namespace PadForge {

// Column editor: insert repeated text or consecutive numbers at a column.
class ColumnEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit ColumnEditorDialog(QWidget* parent = nullptr);

    bool insertTextMode() const;
    int column() const;
    QString text() const;
    int initialNumber() const;
    int step() const;
    int paddingWidth() const;
    int fromLine() const; // 1-based; 0 = first line
    int toLine() const;   // 1-based; -1 = last line
    bool repeatIntoEmptyLines() const;
    bool positionAtEnd() const;

private:
    void build();

    QComboBox* m_mode = nullptr; // "Text" or "Numbers"
    QSpinBox* m_column = nullptr;
    QLineEdit* m_text = nullptr;
    QSpinBox* m_initial = nullptr;
    QSpinBox* m_step = nullptr;
    QSpinBox* m_padding = nullptr;
    QSpinBox* m_fromLine = nullptr;
    QSpinBox* m_toLine = nullptr;
    QCheckBox* m_repeatIntoEmpty = nullptr;
    QCheckBox* m_positionEnd = nullptr;
};

} // namespace PadForge