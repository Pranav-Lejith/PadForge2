#pragma once

#include <QWidget>

class QFileSystemModel;
class QLineEdit;
class QModelIndex;
class QToolButton;
class QTreeView;

namespace PadForge {

// Dockable explorer panel: a QFileSystemModel tree rooted at a chosen folder.
class ExplorerPanel : public QWidget {
    Q_OBJECT

public:
    explicit ExplorerPanel(QWidget* parent = nullptr);

    void setRoot(const QString& path);
    QString root() const { return m_root; }

    void revealPath(const QString& path);

    bool isFollowing() const;
    void setFollowing(bool on);

signals:
    void fileActivated(const QString& path);
    void followToggled(bool on);

private slots:
    void browse();
    void refresh();
    void onActivated(const QModelIndex& index);
    void onFollowChanged(bool on);

private:
    QLineEdit* m_path = nullptr;
    QTreeView* m_tree = nullptr;
    QFileSystemModel* m_model = nullptr;
    QToolButton* m_followButton = nullptr;
    QString m_root;
    bool m_follow = true;
};

} // namespace PadForge