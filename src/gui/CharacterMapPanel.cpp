#include "CharacterMapPanel.h"

#include <QComboBox>
#include <QFontDatabase>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>
#include <iterator>

namespace PadForge {

namespace {

struct Block {
    const char* name;
    uint start;
    int count;
};

const Block kBlocks[] = {
    {"Basic Latin", 0x0020, 0x5F},
    {"Latin-1 Supplement", 0x00A0, 0x60},
    {"Latin Extended-A", 0x0100, 0x80},
    {"Greek and Coptic", 0x0391, 0x38},
    {"Cyrillic", 0x0410, 0x40},
    {"General Punctuation", 0x2000, 0x70},
    {"Superscripts and Subscripts", 0x2070, 0x30},
    {"Currency Symbols", 0x20A0, 0x20},
    {"Letterlike Symbols", 0x2100, 0x50},
    {"Arrows", 0x2190, 0x70},
    {"Mathematical Operators", 0x2200, 0x100},
    {"Box Drawing", 0x2500, 0x80},
    {"Block Elements", 0x2580, 0x20},
    {"Geometric Shapes", 0x25A0, 0x60},
    {"Miscellaneous Symbols", 0x2600, 0x100},
    {"Dingbats", 0x2700, 0x30},
    {"Braille Patterns", 0x2800, 0x100},
    {"Hiragana", 0x3040, 0x60},
    {"Katakana", 0x30A0, 0x60},
    {"CJK Symbols and Punctuation", 0x3000, 0x40},
    {"Fullwidth Forms", 0xFF01, 0x5D},
    {"Mathematical Alphanumeric (sample)", 0x1D400, 0x30},
};

constexpr int kColumns = 16;

} // namespace

CharacterMapPanel::CharacterMapPanel(QWidget* parent) : QWidget(parent) {
    m_blocks = new QComboBox(this);
    m_grid = new QTableWidget(this);
    m_status = new QLabel(tr("Double-click a character to insert it"), this);

    m_grid->horizontalHeader()->setVisible(false);
    m_grid->verticalHeader()->setVisible(false);
    m_grid->setShowGrid(false);
    m_grid->setSelectionMode(QAbstractItemView::SingleSelection);
    m_grid->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_grid->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_grid->setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));

    connect(m_blocks, &QComboBox::currentIndexChanged, this, &CharacterMapPanel::populateBlock);
    connect(m_grid, &QTableWidget::itemDoubleClicked, this, &CharacterMapPanel::insertItem);
    connect(m_grid, &QTableWidget::itemActivated, this, &CharacterMapPanel::insertItem);
    connect(m_grid, &QTableWidget::currentCellChanged, this, &CharacterMapPanel::updateStatus);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);
    layout->addWidget(m_blocks);
    layout->addWidget(m_grid, 1);
    layout->addWidget(m_status);

    populateBlocks();
}

void CharacterMapPanel::populateBlocks() {
    for (const Block& block : kBlocks)
        m_blocks->addItem(QString::fromLatin1(block.name));
    if (m_blocks->count() > 0)
        m_blocks->setCurrentIndex(0);
}

void CharacterMapPanel::populateBlock(int index) {
    if (index < 0 || index >= int(std::size(kBlocks)))
        return;
    const Block& block = kBlocks[index];

    m_codepoints.clear();
    m_grid->clearContents();
    m_grid->setColumnCount(kColumns);
    const int rows = (block.count + kColumns - 1) / kColumns;
    m_grid->setRowCount(rows);

    for (int i = 0; i < block.count; ++i) {
        const uint code = block.start + uint(i);
        m_codepoints.append(code);
        auto* item = new QTableWidgetItem(QString::fromUcs4(&code, 1));
        item->setTextAlignment(Qt::AlignCenter);
        item->setToolTip(QStringLiteral("U+%1").arg(code, 4, 16, QLatin1Char('0')).toUpper());
        m_grid->setItem(i / kColumns, i % kColumns, item);
    }

    const int cell = m_grid->fontMetrics().height() + 10;
    m_grid->horizontalHeader()->setDefaultSectionSize(cell);
    m_grid->verticalHeader()->setDefaultSectionSize(cell);
}

void CharacterMapPanel::insertItem() {
    QTableWidgetItem* item = m_grid->currentItem();
    if (!item)
        return;
    const int index = item->row() * kColumns + item->column();
    if (index < 0 || index >= m_codepoints.size())
        return;
    const uint code = m_codepoints.at(index);
    emit characterSelected(QString::fromUcs4(&code, 1));
}

void CharacterMapPanel::updateStatus() {
    QTableWidgetItem* item = m_grid->currentItem();
    if (!item) {
        m_status->clear();
        return;
    }
    const int index = item->row() * kColumns + item->column();
    if (index < 0 || index >= m_codepoints.size()) {
        m_status->clear();
        return;
    }
    m_status->setText(
        QStringLiteral("U+%1").arg(m_codepoints.at(index), 4, 16, QLatin1Char('0')).toUpper());
}

} // namespace PadForge
