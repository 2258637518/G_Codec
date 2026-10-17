#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <QObject>
#include <QQueue>
#include <QMap>
#include <QThread>
#include "transcodetask.h"
#include "control_center.h"
#include "lineitemwidget.h"

class TaskManager : public QObject
{
    Q_OBJECT
public:
    explicit TaskManager(QObject *parent = nullptr);
    ~TaskManager();
    int  add_task(const QString &filePath);
    void start_task(int taskId);
    void stop_task(int taskId);
    int  get_next_id(){return taskID;};
    void delete_task(int taskId);
    void startParseMetadata(int taskId);
signals:
    void taskStatusChanged(int taskId, TaskStatus status);
    void taskProgressUpdated(int taskId, int progress);
    void metadataReady(int taskId,SourcecodeInfo info);
public slots:
    void handleNewTask(LineItemWidget* itemPtr, const TranscodeTask &task);
    void handleTaskFinished(int taskId,TaskStatus status);

private:
    void check_queue();
    int taskID = 0;
    const int max_task_num = 3;
    QMap<int, TranscodeTask> m_taskMap;            // 记录所有任务的状态映射表
    QQueue<int> m_waitingQueue;                    // 等待执行的任务 ID 队列
    QMap<int, QThread*> m_runningThreads;          // 正在运行的线程（Key: taskId）
    QMap<int, control_center*> m_runningWorkers;   // 正在运行的 Worker（Key: taskId）
};

#endif // TASKMANAGER_H
