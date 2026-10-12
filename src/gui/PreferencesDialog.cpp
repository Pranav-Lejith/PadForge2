#include "PreferencesDialog.h"

#include "Theme.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFont>
#include <QFontComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

namespace PadForge {

PreferencesDialog::PreferencesDialog(const AppSettings& settings, QWidget* parent)
    : QDialog(parent), m_initial(settings) {
    setWindowTitle(tr("Preferences"));

    m_theme = new QComboBox(this);
    for (const Theme& theme : ThemeManager::themes()) {
        m_theme->addItem(theme.name, theme.id);
        if (theme.id == m_initial.theme)
            m_theme->setCurrentIndex(m_theme->count() - 1);
    }

    m_fontFamily = new QFontComboBox(this);
    m_fontFamily->setCurrentFont(QFont(m_initial.fontFamily));
    m_fontSize = new QSpinBox(this);
    m_fontSize->setRange(6, 48);
    m_fontSize->setValue(m_initial.fontSize);
    m_tabWidth = new QSpinBox(this);
    m_tabWidth->setRange(1, 16);
    m_tabWidth->setValue(m_initial.tabWidth);
    m_tabMode = new QComboBox(this);
    m_tabMode->addItem(tr("Spaces"), QStringLiteral("spaces"));
    m_tabMode->addItem(tr("Tabs"), QStringLiteral("tabs"));
    m_tabMode->setCurrentIndex(m_initial.tabMode == QStringLiteral("tabs") ? 1 : 0);

    m_wordWrap = new QCheckBox(tr("Word wrap"), this);
    m_lineNumbers = new QCheckBox(tr("Line numbers"), this);
    m_foldMargin = new QCheckBox(tr("Folding margin"), this);
    m_bookmarkMargin = new QCheckBox(tr("Bookmark margin"), this);
    m_whitespace = new QCheckBox(tr("Show all characters"), this);
    m_eol = new QCheckBox(tr("Show line endings"), this);
    m_indentGuides = new QCheckBox(tr("Indentation guides"), this);
    m_highlightLine = new QCheckBox(tr("Highlight current line"), this);
    m_autoCompletion = new QCheckBox(tr("Word completion (Ctrl+Space)"), this);
    m_autoClose = new QCheckBox(tr("Auto-close brackets"), this);
    m_autoCloseTags = new QCheckBox(tr("Auto-close HTML/XML tags"), this);
    m_calltips = new QCheckBox(tr("Parameter calltips"), this);
    m_changeHistory = new QCheckBox(tr("Change history markers"), this);
    m_backupOnSave = new QCheckBox(tr("Backup file on save (.bak)"), this);
    m_backupEnabled = new QCheckBox(tr("Session snapshot && periodic backup"), this);

    m_wordWrap->setChecked(m_initial.wordWrap);
    m_lineNumbers->setChecked(m_initial.showLineNumbers);
    m_foldMargin->setChecked(m_initial.showFoldMargin);
    m_bookmarkMargin->setChecked(m_initial.showBookmarkMargin);
    m_whitespace->setChecked(m_initial.showWhitespace);
    m_eol->setChecked(m_initial.showEol);
    m_indentGuides->setChecked(m_initial.showIndentGuides);
    m_highlightLine->setChecked(m_initial.highlightCurrentLine);
    m_autoCompletion->setChecked(m_initial.autoCompletion);
    m_autoClose->setChecked(m_initial.autoCloseBrackets);
    m_autoCloseTags->setChecked(m_initial.autoCloseTags);
    m_calltips->setChecked(m_initial.calltips);
    m_changeHistory->setChecked(m_initial.showChangeHistory);
    m_backupOnSave->setChecked(m_initial.backupOnSave);
    m_backupEnabled->setChecked(m_initial.backupEnabled);

    m_edgeColumn = new QSpinBox(this);
    m_edgeColumn->setRange(0, 1000);
    m_edgeColumn->setValue(m_initial.edgeColumn);
    m_edgeColumn->setSpecialValueText(tr("Off"));

    auto* tabs = new QTabWidget(this);
    buildEditorTab();
    buildViewTab();
    tabs->addTab(findChild<QWidget*>(QStringLiteral("editorTab")), tr("Editor"));
    tabs->addTab(findChild<QWidget*>(QStringLiteral("viewTab")), tr("View"));

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* root = new QVBoxLayout(this);
    root->addWidget(tabs);
    root->addWidget(buttons);
}

void PreferencesDialog::buildEditorTab() {
    auto* group = new QGroupBox(tr("Editing"), this);
    auto* form = new QFormLayout(group);
    form->addRow(tr("Tab width:"), m_tabWidth);
    form->addRow(tr("Tab mode:"), m_tabMode);
    form->addRow(tr("Long line edge:"), m_edgeColumn);
    form->addRow(tr(""), m_wordWrap);
    form->addRow(tr(""), m_autoCompletion);
    form->addRow(tr(""), m_autoClose);
    form->addRow(tr(""), m_autoCloseTags);
    form->addRow(tr(""), m_calltips);
    form->addRow(tr(""), m_changeHistory);
    form->addRow(tr(""), m_backupOnSave);
    form->addRow(tr(""), m_backupEnabled);
    group->setObjectName(QStringLiteral("editorTab"));
}

void PreferencesDialog::buildViewTab() {
    auto* group = new QGroupBox(tr("Display"), this);
    auto* form = new QFormLayout(group);
    form->addRow(tr("Theme:"), m_theme);
    form->addRow(tr("Font:"), m_fontFamily);
    form->addRow(tr("Font size:"), m_fontSize);
    form->addRow(tr(""), m_lineNumbers);
    form->addRow(tr(""), m_foldMargin);
    form->addRow(tr(""), m_bookmarkMargin);
    form->addRow(tr(""), m_whitespace);
    form->addRow(tr(""), m_eol);
    form->addRow(tr(""), m_indentGuides);
    form->addRow(tr(""), m_highlightLine);
    group->setObjectName(QStringLiteral("viewTab"));
}

AppSettings PreferencesDialog::resultSettings() const {
    AppSettings settings;
    settings.theme = m_theme->currentData().toString();
    settings.fontFamily = m_fontFamily->currentFont().family();
    settings.fontSize = m_fontSize->value();
    settings.tabWidth = m_tabWidth->value();
    settings.tabMode = m_tabMode->currentData().toString();
    settings.edgeColumn = m_edgeColumn->value();

    settings.wordWrap = m_wordWrap->isChecked();
    settings.showLineNumbers = m_lineNumbers->isChecked();
    settings.showFoldMargin = m_foldMargin->isChecked();
    settings.showBookmarkMargin = m_bookmarkMargin->isChecked();
    settings.showWhitespace = m_whitespace->isChecked();
    settings.showEol = m_eol->isChecked();
    settings.showIndentGuides = m_indentGuides->isChecked();
    settings.highlightCurrentLine = m_highlightLine->isChecked();
    settings.autoCompletion = m_autoCompletion->isChecked();
    settings.autoCloseBrackets = m_autoClose->isChecked();
    settings.autoCloseTags = m_autoCloseTags->isChecked();
    settings.calltips = m_calltips->isChecked();
    settings.showChangeHistory = m_changeHistory->isChecked();
    settings.backupOnSave = m_backupOnSave->isChecked();
    settings.backupEnabled = m_backupEnabled->isChecked();
    settings.splitOrientation = m_initial.splitOrientation;
    settings.alwaysOnTop = m_initial.alwaysOnTop;
    settings.syncScroll = m_initial.syncScroll;
    return settings;
}

} // namespace PadForge