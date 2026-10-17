#include "taskmanager.h"

#include <QFile>

TaskManager::TaskManager(QObject *parent)
    : QObject{parent}
{}

TaskManager::~TaskManager()
{
    for(control_center *worker:m_runningWorkers.values())
    {
        if(worker)
        {
            worker->cancel();
        }
    }
    for (QThread *thread :
         m_runningThreads.values()) {
        if (thread && thread->isRunning())
        {
            thread->quit();
            thread->wait(); //阻塞等待，确保线程完全退出后，再由父对象析构函数安全释放其内存
        }
    }
}


int TaskManager::add_task(const QString &filePath)
{
    int currentID = taskID++;

    // 初始化任务状态，存入 m_taskMap
    TranscodeTask task;
    task.inputPath = filePath;
    task.state = TaskStatus::Waiting;
    m_taskMap[currentID] = task;

    return currentID; // 立即返回ID，不进行任何阻塞解析
}

void TaskManager::start_task(int taskId)
{
    if (!m_taskMap.contains(taskId))
    {
        qDebug() << "任务列表中找不到 taskId:" << taskId;
        return;
    }
    // 1. 更新任务状态为正在转码，并通知UI 按钮变更为“转码中...”
    qDebug() << "尝试拉起转码，taskId:" << taskId;
    m_taskMap[taskId].state = TaskStatus::Running;
    emit taskStatusChanged(taskId,TaskStatus::Running);

    // 2. 从 Map 容器中取出之前由startParseMetadata 已经建立并保持挂起状态的worker
        if (m_runningWorkers.contains(taskId)) {
            control_center *worker = m_runningWorkers[taskId];
            //跨线程安全调用
            qDebug() << "成功找到对应的 worker，开始跨线程调用 control_center::start";
            QMetaObject::invokeMethod(worker, "start",Qt::QueuedConnection);
        }
        else
        {
            qDebug() << "找不到运行中的 worker，无法启动！";
        }
}

void TaskManager::delete_task(int taskId)
{
    if (m_runningThreads.contains(taskId))
    {
        m_runningWorkers[taskId]->cancel();
        m_runningThreads[taskId]->quit();
        // 精准停止该 taskId 的线程
        m_runningThreads[taskId]->wait();
        // 等待其安全退出
        qDebug()<<"线程"<<taskId<<"安全退出";
        m_runningThreads.remove(taskId);
        m_runningWorkers.remove(taskId);
        QFile::remove(m_taskMap[taskId].outputPath);
    }
    if(m_taskMap.contains(taskId))
    {
        m_taskMap.remove(taskId);
    }
    if(m_waitingQueue.contains(taskId))
    {
        m_waitingQueue.removeOne(taskId);
    }

    check_queue();
}

void TaskManager::startParseMetadata(int taskId)
{
    qDebug()<<taskId;
    if (!m_taskMap.contains(taskId))
        return;
    QString filePath = m_taskMap[taskId].
                       inputPath;
    control_center *worker = new
        control_center(filePath, taskId,m_taskMap[taskId]);
    QThread *thread = new QThread();
    worker->moveToThread(thread);
    connect(thread, &QThread::started, worker,&control_center::open);
    connect(worker, &control_center::metadataReady,this, &TaskManager::metadataReady);
    connect(thread, &QThread::finished, worker,&QObject::deleteLater);
    connect(thread, &QThread::finished, thread,&QObject::deleteLater);
    m_runningWorkers[taskId] = worker;
    m_runningThreads[taskId] = thread;
    connect(worker,&control_center::finished, this, [this,taskId]() {
                handleTaskFinished(taskId,TaskStatus::Finished);
            });
    // 监听解析出错
    connect(worker,&control_center::metadataError, this,[this, taskId]() {
                handleTaskFinished(taskId,TaskStatus::Error);
            });
    connect(worker,&control_center::progress_updata,this,[this,taskId](int percent){
        emit taskProgressUpdated(taskId,percent);
    });
    thread->start();
}


void TaskManager::handleNewTask(LineItemWidget *itemPtr, const TranscodeTask &task)
{
    if (!itemPtr) return;

    // 1. 根据传递过来的 UI控件指针获取对应的任务 ID
        int taskId = itemPtr->getTaskId();

    // 2. 将用户在 Dialog中配置的目标参数更新到全局任务表 m_taskMap中
        if (m_taskMap.contains(taskId)) {
        QString originalInputPath = m_taskMap[taskId].inputPath; //备份输入文件路径

        m_taskMap[taskId] = task;
        // 覆盖为完整的转码参数
        m_taskMap[taskId].inputPath =originalInputPath;         // 恢复输入路径
        m_taskMap[taskId].state =TaskStatus::Waiting;           //更新状态为排队等待
    }

    // 同步最新的配置参数给对应的 worker
    if (m_runningWorkers.contains(taskId) && m_runningWorkers[taskId]) {
        m_runningWorkers[taskId]->set_task_info(m_taskMap[taskId]);
    }

    // 3. 将此任务 ID 压入等待队列
    m_waitingQueue.enqueue(taskId);

    // 4. 发射状态改变信号，通知 UI按钮更新为“排队中...”
        emit taskStatusChanged(taskId,TaskStatus::Waiting);

    // 5. 触发队列检查，尝试启动转码调度
    check_queue();

}

void TaskManager::handleTaskFinished(int taskId, TaskStatus status)
{
    // 1. 更新全局状态，通知 UI 界面
    if (m_taskMap.contains(taskId)) {
        m_taskMap[taskId].state =status;
        emit taskStatusChanged(taskId,status);
    }

    // 2. 让该任务的 QThread线程安全退出
        if (m_runningThreads.contains(taskId)) {
            QThread *thread =m_runningThreads[taskId];
        if (thread && thread->isRunning()) {
            thread->quit(); //退出线程的事件循环
        }
        m_runningThreads.remove(taskId);
        m_runningWorkers.remove(taskId);
    }
    // 3.释放了并发槽位，检查排队队列拉起下一个任务
        check_queue();
}

void TaskManager::check_queue()
{
    // 1.获取当前正在进行的任务数量
    int runningCount = 0;
    for(TranscodeTask &task:m_taskMap.values())
    {
        if(task.state == TaskStatus::Running)
        {
            runningCount++;
        }
    }
    //2.判断
    if(runningCount>=max_task_num||m_waitingQueue.isEmpty()){
        return;
    }
    // 3.从等待队列中取出下一个排队任务的 ID
        int nextTaskId = m_waitingQueue.dequeue();

    // 4. 启动该任务的转码线程
    // 此时 start_task内部会创建线程并加入 m_runningThreads
        start_task(nextTaskId);

    // 5.递归调用，尝试继续填满剩余的并发名额
    check_queue();
}
