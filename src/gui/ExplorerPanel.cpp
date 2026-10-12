#include "ExplorerPanel.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>

namespace PadForge {

ExplorerPanel::ExplorerPanel(QWidget* parent) : QWidget(parent) {
    m_path = new QLineEdit(this);
    m_path->setPlaceholderText(tr("Folder as workspace"));
    auto* browseButton = new QPushButton(tr("..."), this);
    browseButton->setFixedWidth(32);
    auto* homeButton = new QPushButton(tr("Home"), this);
    auto* refreshButton = new QPushButton(tr("Refresh"), this);
    m_followButton = new QToolButton(this);
    m_followButton->setText(tr("Follow"));
    m_followButton->setCheckable(true);
    m_followButton->setChecked(m_follow);
    m_followButton->setToolTip(tr("Reveal the active document in the tree"));

    m_model = new QFileSystemModel(this);
    m_model->setReadOnly(true);
    m_model->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Files);
    m_model->setNameFilterDisables(false);

    m_tree = new QTreeView(this);
    m_tree->setModel(m_model);
    m_tree->setHeaderHidden(true);
    for (int column = 1; column < m_model->columnCount(); ++column)
        m_tree->hideColumn(column);
    m_tree->setUniformRowHeights(true);
    m_tree->setAnimated(false);
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto* pathRow = new QHBoxLayout;
    pathRow->addWidget(m_path, 1);
    pathRow->addWidget(browseButton);
    auto* optionRow = new QHBoxLayout;
    optionRow->addWidget(homeButton);
    optionRow->addWidget(m_followButton);
    optionRow->addWidget(refreshButton);
    optionRow->addStretch();

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(4, 4, 4, 4);
    root->addLayout(pathRow);
    root->addLayout(optionRow);
    root->addWidget(m_tree, 1);

    setRoot(QDir::homePath());

    connect(browseButton, &QPushButton::clicked, this, &ExplorerPanel::browse);
    connect(homeButton, &QPushButton::clicked, this, [this]() { setRoot(QDir::homePath()); });
    connect(refreshButton, &QPushButton::clicked, this, &ExplorerPanel::refresh);
    connect(m_followButton, &QToolButton::toggled, this, &ExplorerPanel::onFollowChanged);
    connect(m_path, &QLineEdit::returnPressed, this,
            [this]() { setRoot(QDir::cleanPath(m_path->text())); });
    connect(m_tree, &QTreeView::activated, this, &ExplorerPanel::onActivated);
}

void ExplorerPanel::setRoot(const QString& path) {
    if (path.isEmpty())
        return;
    const QString clean = QDir::cleanPath(path);
    m_root = clean;
    const QModelIndex index = m_model->setRootPath(clean);
    m_tree->setRootIndex(index);
    m_path->setText(clean);
}

void ExplorerPanel::revealPath(const QString& path) {
    if (path.isEmpty() || m_tree->isHidden())
        return;
    const QFileInfo info(path);
    const QString dir = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
    if (QDir::cleanPath(m_root) != QDir::cleanPath(dir))
        setRoot(dir);
    const QModelIndex index = m_model->index(path);
    if (!index.isValid())
        return;
    m_tree->setCurrentIndex(index);
    m_tree->scrollTo(index, QAbstractItemView::PositionAtCenter);
}

bool ExplorerPanel::isFollowing() const {
    return m_follow && m_followButton && m_followButton->isChecked();
}

void ExplorerPanel::setFollowing(bool on) {
    m_follow = on;
    if (m_followButton)
        m_followButton->setChecked(on);
}

void ExplorerPanel::browse() {
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Folder as workspace"), m_root);
    if (!dir.isEmpty())
        setRoot(dir);
}

void ExplorerPanel::refresh() {
    if (!m_root.isEmpty())
        setRoot(m_root);
}

void ExplorerPanel::onActivated(const QModelIndex& index) {
    if (!index.isValid())
        return;
    const QFileInfo info(m_model->filePath(index));
    if (info.isDir())
        return;
    emit fileActivated(info.absoluteFilePath());
}

void ExplorerPanel::onFollowChanged(bool on) {
    m_follow = on;
    emit followToggled(on);
}

} // namespace PadForge