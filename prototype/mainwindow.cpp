#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setAutoFillBackground(true);
    this->setAcceptDrops(true);
    ui->stackedWidget->setCurrentIndex(0);
    ui->listWidget->setAcceptDrops(false);
    taskManager = new TaskManager(this);
    connect(taskManager,&TaskManager::metadataReady,this,&MainWindow::onMetadataReady);
}
MainWindow::~MainWindow()
{
    this->disconnect();
    delete ui;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event){
    if(event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
        qDebug()<<"接受拖拽文件";
    }
    else
    {
        event->ignore();
    }
}

void MainWindow::dropEvent(QDropEvent *event){
    QList<QUrl> urls = event->mimeData()->urls();
    if(urls.isEmpty()) {
        return;
    }

    const QStringList supportedExtensions = {
        "mp4", "mkv", "avi", "mov", "wmv", "flv", "webm", "ts", "rmvb", "m4v"
    };

    for (const QUrl &url : urls) {
        QString localFile = url.toLocalFile();

        if (!localFile.isEmpty()) {
            QFileInfo fileInfo(localFile);
            if (fileInfo.isDir()) {
                continue;
            }

            QString suffix = fileInfo.suffix().toLower();
            if (supportedExtensions.contains(suffix)) {
                filePath.append(localFile);
                addFileToList(localFile);
                qDebug() << "添加了文件:" << localFile;
            } else {
                qDebug() << "忽略非支持的视频格式文件:" << localFile;
            }
        }
    }
    event->acceptProposedAction();
}

void MainWindow::on_actionopen_triggered()
{
    QStringList selectedFiles = QFileDialog::getOpenFileNames(
        this,
        "选择视频文件",
        "",
        "视频文件 (*.mp4 *.avi *.mkv *.mov *.wmv *.flv *.webm *.ts *.rmvb);;所有文件 (*.*)"
        );

    if (!selectedFiles.isEmpty()) {
        filePath.append(selectedFiles);
        for (const QString &file : selectedFiles) {
            qDebug() << "添加了文件:" << file;
            addFileToList(file);
        }
    }
}
void MainWindow::addFileToList(const QString &filePath){
    QFileInfo fileInfo(filePath);
    if(fileInfo.isDir())
    {
        return;
    }
    QListWidgetItem *item = new QListWidgetItem(ui->listWidget);
    int taskId = taskManager->add_task(filePath);
    item->setData(Qt::UserRole, taskId);
    LineItemWidget *new_item = new LineItemWidget(filePath);
    new_item->setTaskId(taskId);
    item->setSizeHint(new_item->sizeHint());
    ui->listWidget->setItemWidget(item,new_item);

    connect(new_item,&LineItemWidget::requestParseMetadata, this,[this,taskId](){
                taskManager->startParseMetadata(taskId);
            });

    connect(new_item,&LineItemWidget::delete_current_item,this,[this,item,new_item]{
        int row = ui->listWidget->row(item);
        QListWidgetItem *takenItem = ui->listWidget->takeItem(row);
        taskManager->delete_task(new_item->getTaskId());
        delete takenItem;
    });
    connect(new_item, &LineItemWidget::send_task_info,taskManager, &TaskManager::handleNewTask);
    connect(taskManager,&TaskManager::taskStatusChanged,new_item,[new_item, taskId](int id,TaskStatus status)
            {
                if(id == taskId)
            {
            new_item->status(status);
        }
    });
    connect(taskManager,&TaskManager::taskProgressUpdated,new_item,&LineItemWidget::progress_updata);
    this->updatePageBasedOnList();
}
void MainWindow::updatePageBasedOnList()
{
    int count = ui->listWidget->count();

    qDebug() << "当前列表项数量：" << count;

    if (count == 0) {
        ui->stackedWidget->setCurrentIndex(0);
    }
    else {
        ui->stackedWidget->setCurrentIndex(1);
    }
}

QListWidgetItem *MainWindow::findItemByTaskId(int taskId)
{
    for(int i=0;i<ui->listWidget->count();i++)
    {
        QListWidgetItem* item = ui->listWidget->item(i);

        // 取出我们在添加任务时用 setData 存进去的 TaskID
        if(!item){
            qDebug()<<"寻找item失败";
        }
        int itemTaskId = item->data(Qt::UserRole).toInt();

        // 如果匹配成功，说明找到了
        if (itemTaskId == taskId) {
            qDebug()<<"寻找item成功";
            return item;
        }
    }
    return nullptr;
}


void MainWindow::on_actionquit_triggered()
{
    this->close();
}

void MainWindow::onMetadataReady(int taskId, SourcecodeInfo info)
{
    // 1. 根据 taskId 找到对应的 QListWidgetItem
    QListWidgetItem* item = findItemByTaskId(taskId);
    if(!item){
        qDebug()<<"无法找到对应的item";
    }
    // 2. 拿到你的自定义界面指针
    LineItemWidget* widget = qobject_cast<LineItemWidget*>(ui->listWidget->itemWidget(item));
    if (widget) {
        widget->setItemInfo(info);
        qDebug() << "成功将元数据绑定到Task ID" << taskId;
    }
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    //此处执行清理代码
    // 1. 创建一个询问对话框
    QMessageBox::StandardButton resBtn = QMessageBox::question(
        this,
        "确认退出",
        "确定要退出程序吗？\n如果有未完成的任务将会被中止。",
        QMessageBox::No | QMessageBox::Yes,
        QMessageBox::No // 默认聚焦在“否”按钮，防止用户连续敲回车误操作
        );

    // 2. 根据用户选择决定是否放行
    if (resBtn != QMessageBox::Yes) {
        event->ignore(); // 核心：忽略事件，窗口不会关闭，程序继续运行
    } else {
        event->accept(); // 核心：接受事件，窗口关闭，开始进入销毁流程
    }
}

