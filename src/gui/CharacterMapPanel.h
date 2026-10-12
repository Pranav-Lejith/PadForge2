#pragma once

#include <QVector>
#include <QWidget>

class QComboBox;
class QLabel;
class QTableWidget;

namespace PadForge {

// A browsable grid of Unicode characters that inserts the selected glyph.
class CharacterMapPanel : public QWidget {
    Q_OBJECT

public:
    explicit CharacterMapPanel(QWidget* parent = nullptr);

signals:
    void characterSelected(const QString& text);

private:
    void populateBlocks();
    void populateBlock(int index);
    void insertItem();
    void updateStatus();

    QComboBox* m_blocks = nullptr;
    QTableWidget* m_grid = nullptr;
    QLabel* m_status = nullptr;
    QVector<uint> m_codepoints;
};

} // namespace PadForge
