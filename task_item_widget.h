#ifndef TASK_ITEM_WIDGET_H
#define TASK_ITEM_WIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QDialog>
#include <QDesktopServices>
#include "transcode_dialog.h"
#include "transcode_task.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class TaskItemWidget;
}
QT_END_NAMESPACE

class TaskItemWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TaskItemWidget(QString filepath, QWidget *parent = nullptr);
    ~TaskItemWidget();
    void setItemInfo(SourcecodeInfo info);
    int getTaskId(){return taskId;};
    void setTaskId(int id) { this->taskId = id; }
    void status(TaskStatus status);
    void progress_updata(int taskId, int val);

    // 错误诊断与提示
    void setErrorInfo(const QString &title, const QString &detail);
    void showErrorDialog();

    // 批量配置预设
    void setPendingPreset(const BatchPreset &preset);
    void applyBatchPreset(const BatchPreset &preset);
    bool isConfigured() const { return m_isConfigured; }

    // 获取文件路径与元数据
    QString getFilePath() const { return filepath; }
    QString getSavePath() const { return save_path; }
    const SourcecodeInfo& getSourceInfo() const { return info; }
    bool isParsed() const { return hasInfo; }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    int taskId = -1;
    Ui::TaskItemWidget *ui;
    QString filepath;
    QString save_path;
    SourcecodeInfo info;
    bool hasInfo = false;
    bool m_isConfigured = false;
    TaskStatus m_currentStatus = TaskStatus::Waiting;

    // 错误信息缓存
    QString m_errorTitle;
    QString m_errorDetail;
    TranscodeTask m_lastTask;

    // 挂起的批量预设
    bool m_hasPendingPreset = false;
    BatchPreset m_pendingPreset;

signals:
    void delete_current_item();
    void send_task_info(TaskItemWidget *itemPtr, TranscodeTask task_info);
    void requestParseMetadata();
    void configAppliedToAllRequested(const TranscodeTask &task);

public slots:
    void triggerConfigureOrStart();

private slots:
    void on_pushButton_2_clicked();
    void on_pushButton_clicked();
    void onDialogDataReady(const TranscodeTask &basicTask);
};

#endif // TASK_ITEM_WIDGET_H
