#pragma once

#include "Settings.h"

#include <QDialog>
#include <QStringList>

class QCheckBox;
class QComboBox;
class QFontComboBox;
class QSpinBox;

namespace PadForge {

// Modal dialog for editing AppSettings.
class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(const AppSettings& settings, QWidget* parent = nullptr);

    AppSettings resultSettings() const;

private:
    void buildEditorTab();
    void buildViewTab();

    const AppSettings m_initial;

    QComboBox* m_theme = nullptr;
    QFontComboBox* m_fontFamily = nullptr;
    QSpinBox* m_fontSize = nullptr;
    QSpinBox* m_tabWidth = nullptr;
    QComboBox* m_tabMode = nullptr;
    QSpinBox* m_edgeColumn = nullptr;

    QCheckBox* m_wordWrap = nullptr;
    QCheckBox* m_lineNumbers = nullptr;
    QCheckBox* m_foldMargin = nullptr;
    QCheckBox* m_bookmarkMargin = nullptr;
    QCheckBox* m_whitespace = nullptr;
    QCheckBox* m_eol = nullptr;
    QCheckBox* m_indentGuides = nullptr;
    QCheckBox* m_highlightLine = nullptr;
    QCheckBox* m_autoCompletion = nullptr;
    QCheckBox* m_autoClose = nullptr;
    QCheckBox* m_autoCloseTags = nullptr;
    QCheckBox* m_calltips = nullptr;
    QCheckBox* m_changeHistory = nullptr;
    QCheckBox* m_backupOnSave = nullptr;
    QCheckBox* m_backupEnabled = nullptr;
};

} // namespace PadForge