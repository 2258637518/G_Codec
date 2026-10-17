#ifndef TRANSCODE_DIALOG_H
#define TRANSCODE_DIALOG_H

#include <QDialog>
#include <QComboBox>
#include "transcode_task.h"

namespace Ui {
class TranscodeDialog;
}

class TranscodeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TranscodeDialog(SourcecodeInfo info, QWidget *parent = nullptr);
    ~TranscodeDialog();
    bool applyToAll() const { return m_applyToAll; }

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

signals:
    void taskDataReady(const TranscodeTask &task);

private:
    Ui::TranscodeDialog *ui;
    SourcecodeInfo info;
    TranscodeTask task_info;
    bool m_applyToAll = false;
    class QCheckBox *m_chkApplyToAll = nullptr;
};

#endif // TRANSCODE_DIALOG_H
