#ifndef BATCH_TRANSCODE_DIALOG_H
#define BATCH_TRANSCODE_DIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QCheckBox>
#include <QPushButton>
#include "transcode_task.h"

class BatchTranscodeDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BatchTranscodeDialog(int waitingCount, int totalCount, QWidget *parent = nullptr);
    ~BatchTranscodeDialog() override = default;

    BatchPreset getPreset() const { return m_preset; }
    bool shouldStartImmediately() const { return m_startImmediately; }

private slots:
    void onPresetComboChanged(int index);
    void onFormatChanged(const QString &formatText);
    void onBrowseOutputDir();
    void onApplyAndStartClicked();
    void onApplyOnlyClicked();

private:
    void setupUi(int waitingCount, int totalCount);
    void applyQuickPreset(int presetIndex);
    void updateCodecCombos(const QString &formatText);

    QComboBox *m_presetCombo = nullptr;
    QComboBox *m_formatCombo = nullptr;
    QComboBox *m_videoCodecCombo = nullptr;
    QComboBox *m_audioCodecCombo = nullptr;
    QComboBox *m_resolutionCombo = nullptr;
    QComboBox *m_framerateCombo = nullptr;
    QComboBox *m_videoBitrateCombo = nullptr;

    QRadioButton *m_radioSourceDir = nullptr;
    QRadioButton *m_radioCustomDir = nullptr;
    QLineEdit *m_editCustomDir = nullptr;
    QPushButton *m_btnBrowseDir = nullptr;
    QLineEdit *m_editSuffix = nullptr;

    QRadioButton *m_radioOnlyUnconfigured = nullptr;
    QRadioButton *m_radioAllNonRunning = nullptr;

    BatchPreset m_preset;
    bool m_startImmediately = false;
};

#endif // BATCH_TRANSCODE_DIALOG_H
