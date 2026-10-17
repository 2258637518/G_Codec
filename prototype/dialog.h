#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QComboBox>
#include "transcodetask.h"

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(SourcecodeInfo info,QWidget *parent = nullptr);
    ~Dialog();

private slots:
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();
signals:
    void taskDataReady(const TranscodeTask &task);
private:
    Ui::Dialog *ui;
    SourcecodeInfo info;
    TranscodeTask task_info;
};

#endif // DIALOG_H
