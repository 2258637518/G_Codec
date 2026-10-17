#ifndef LINEITEMWIDGET_H
#define LINEITEMWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QDialog>
#include <QDesktopServices>
#include "dialog.h"
#include "transcodetask.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class LineItemWidget;
}
QT_END_NAMESPACE

class LineItemWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LineItemWidget(QString filepath, QWidget *parent = nullptr);
    ~LineItemWidget();
    void setItemInfo(SourcecodeInfo info);
    int getTaskId(){return taskId;};
    void  setTaskId(int id) { this->taskId = id; }
    void status(TaskStatus status);
    void progress_updata(int taskId,int val);
private:
    int taskId;
    Ui::LineItemWidget *ui;
    QString filepath;
    QString save_path;
    SourcecodeInfo info;
    bool hasInfo = false;
signals:
    void delete_current_item();
    void send_task_info(LineItemWidget *itemPtr,TranscodeTask task_info);
    void requestParseMetadata();

private slots:
    void on_pushButton_2_clicked();
    void on_pushButton_clicked();
    void onDialogDataReady(const TranscodeTask &basicTask);
};

#endif // LINEITEMWIDGET_H
