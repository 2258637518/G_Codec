#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "window_style_helper.h"
#include "batch_transcode_dialog.h"
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QUrl>
#include <QMessageBox>
#include <QDebug>

static const QStringList kSupportedExtensions = {
    "mp4", "mkv", "avi", "mov", "wmv", "flv", "webm", "ts", "m2ts", "mts",
    "vob", "mpg", "mpeg", "m4v", "rmvb", "mp3", "aac", "flac", "wav", "m4a",
    "ogg", "opus", "wma"
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 适配 Windows 原生 DWM 沉浸式深色窗口标题栏与边框，与黑曜石 UI 主题融为一体
    WindowStyleHelper::applyDarkWindowFrame(this);

    this->setAutoFillBackground(true);
    this->setAcceptDrops(true);
    ui->stackedWidget->setCurrentIndex(0);
    ui->listWidget->setAcceptDrops(false);

    taskManager = new TaskManager(this);
    connect(taskManager, &TaskManager::metadataReady, this, &MainWindow::onMetadataReady);
    connect(taskManager, &TaskManager::taskStatusChanged, this, &MainWindow::onTaskStatusChanged);
    connect(taskManager, &TaskManager::taskErrorOccurred, this, &MainWindow::onTaskErrorOccurred);

    // 隐藏传统老式菜单栏，保留顶栏作为主要工作流操作区，将动作挂载到主窗口以支持全局快捷键
    this->addAction(ui->actionopen);
    this->addAction(ui->actionOpenFolder);
    this->addAction(ui->actionquit);
    this->addAction(ui->actionBatchPreset);
    this->addAction(ui->actionStartAll);
    this->addAction(ui->actionStopAll);
    this->addAction(ui->actionClearFinished);
    this->addAction(ui->actionAbout);
    ui->menubar->setVisible(false);

    updateStatusBar();
}

MainWindow::~MainWindow()
{
    this->disconnect();
    delete ui;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) {
        return;
    }

    for (const QUrl &url : urls) {
        QString localFile = url.toLocalFile();
        if (localFile.isEmpty()) continue;

        QFileInfo fi(localFile);
        if (fi.isDir()) {
            addFolderToList(localFile);
        } else {
            QString suffix = fi.suffix().toLower();
            if (kSupportedExtensions.contains(suffix)) {
                filePath.append(localFile);
                addFileToList(localFile);
            }
        }
    }
    event->acceptProposedAction();
}

void MainWindow::addFileToList(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || fileInfo.isDir()) {
        return;
    }

    QListWidgetItem *item = new QListWidgetItem(ui->listWidget);
    int taskId = taskManager->add_task(filePath);
    item->setData(Qt::UserRole, taskId);

    TaskItemWidget *newItem = new TaskItemWidget(filePath);
    newItem->setTaskId(taskId);
    item->setSizeHint(QSize(ui->listWidget->width() - 24, 84));
    ui->listWidget->setItemWidget(item, newItem);

    connect(newItem, &TaskItemWidget::requestParseMetadata, this, [this, taskId]() {
        taskManager->startParseMetadata(taskId);
    });

    connect(newItem, &TaskItemWidget::delete_current_item, this, [this, item, newItem]() {
        int row = ui->listWidget->row(item);
        QListWidgetItem *takenItem = ui->listWidget->takeItem(row);
        taskManager->delete_task(newItem->getTaskId());
        delete takenItem;
        updatePageBasedOnList();
        updateStatusBar();
    });

    connect(newItem, &TaskItemWidget::send_task_info, taskManager, &TaskManager::handleNewTask);

    connect(taskManager, &TaskManager::taskStatusChanged, newItem, [newItem, taskId](int id, TaskStatus status) {
        if (id == taskId) {
            newItem->status(status);
        }
    });

    connect(taskManager, &TaskManager::taskProgressUpdated, newItem, &TaskItemWidget::progress_updata);

    connect(newItem, &TaskItemWidget::configAppliedToAllRequested, this, [this](const TranscodeTask &task) {
        BatchPreset preset;
        preset.outputFormat = task.outputFormat;
        preset.videoCodec = task.videoCodec;
        preset.audioCodec = task.audioCodec;
        preset.width = task.width;
        preset.height = task.height;
        preset.frameRate = task.frameRate;
        preset.videoBitrate = task.videoBitrate;
        preset.audioBitrate = task.audioBitrate;
        preset.audioSampleRate = task.audioSampleRate;
        preset.audioChannels = task.audioChannels;
        preset.onlyUnconfigured = true;
        applyBatchPreset(preset);
        ui->statusbar->showMessage("⚡ 已将当前配置一键应用至列表中所有待处理任务！", 4000);
    });

    updatePageBasedOnList();
    updateStatusBar();
}

void MainWindow::addFolderToList(const QString &dirPath)
{
    QDirIterator it(dirPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString currentPath = it.next();
        QString suffix = QFileInfo(currentPath).suffix().toLower();
        if (kSupportedExtensions.contains(suffix)) {
            addFileToList(currentPath);
        }
    }
}

void MainWindow::updatePageBasedOnList()
{
    int count = ui->listWidget->count();
    if (count == 0) {
        ui->stackedWidget->setCurrentIndex(0);
    } else {
        ui->stackedWidget->setCurrentIndex(1);
    }
}

void MainWindow::updateStatusBar()
{
    int total = ui->listWidget->count();
    QString errText = (m_errorCount > 0) ? QString("  |  <span style='color: #f87171; font-weight: bold;'>✕ 失败: %1 项</span>").arg(m_errorCount) : "";
    ui->statusbar->showMessage(QString("任务队列: %1 项  |  正在转码: %2 项  |  已完成: %3 项%4")
                                   .arg(total)
                                   .arg(m_runningCount)
                                   .arg(m_finishedCount)
                                   .arg(errText));
}

QListWidgetItem *MainWindow::findItemByTaskId(int taskId)
{
    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem *item = ui->listWidget->item(i);
        if (item && item->data(Qt::UserRole).toInt() == taskId) {
            return item;
        }
    }
    return nullptr;
}

void MainWindow::onMetadataReady(int taskId, SourcecodeInfo info)
{
    QListWidgetItem *item = findItemByTaskId(taskId);
    if (!item) return;

    TaskItemWidget *widget = qobject_cast<TaskItemWidget*>(ui->listWidget->itemWidget(item));
    if (widget) {
        widget->setItemInfo(info);
    }
}

void MainWindow::onTaskStatusChanged(int taskId, TaskStatus status)
{
    Q_UNUSED(taskId);
    if (status == TaskStatus::Running) {
        m_runningCount++;
    } else if (status == TaskStatus::Finished) {
        if (m_runningCount > 0) m_runningCount--;
        m_finishedCount++;
    } else if (status == TaskStatus::Error) {
        if (m_runningCount > 0) m_runningCount--;
        m_errorCount++;
    }
    updateStatusBar();
}

void MainWindow::onTaskErrorOccurred(int taskId, const QString &title, const QString &detail)
{
    QListWidgetItem *item = findItemByTaskId(taskId);
    if (item) {
        TaskItemWidget *widget = qobject_cast<TaskItemWidget*>(ui->listWidget->itemWidget(item));
        if (widget) {
            widget->setErrorInfo(title, detail);
        }
    }
    ui->statusbar->showMessage(QString("⚠️ 转码失败 (任务 %1): %2 [点击任务卡片状态可查看详细排查指引]").arg(taskId).arg(title), 8000);
}

void MainWindow::on_btnBatchPreset_clicked()
{
    on_actionBatchPreset_triggered();
}

void MainWindow::on_actionBatchPreset_triggered()
{
    if (ui->listWidget->count() == 0) {
        QMessageBox::information(this, "提示", "任务队列当前为空，请先拖入或添加媒体文件。");
        return;
    }

    int waitingCount = 0;
    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem *item = ui->listWidget->item(i);
        if (!item) continue;
        TaskItemWidget *w = qobject_cast<TaskItemWidget*>(ui->listWidget->itemWidget(item));
        if (w && !w->isConfigured()) {
            waitingCount++;
        }
    }

    BatchTranscodeDialog dialog(waitingCount, ui->listWidget->count(), this);
    if (dialog.exec() == QDialog::Accepted) {
        BatchPreset preset = dialog.getPreset();
        applyBatchPreset(preset);

        if (dialog.shouldStartImmediately()) {
            ui->statusbar->showMessage("⚡ 批量预设已应用，正在并发启动硬件加速转码流水线...", 5000);
        } else {
            ui->statusbar->showMessage("✓ 批量预设已成功应用到对应任务，可点击【▶ 全部开始】启动", 5000);
        }
    }
}

void MainWindow::applyBatchPreset(const BatchPreset &preset)
{
    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem *item = ui->listWidget->item(i);
        if (!item) continue;
        TaskItemWidget *widget = qobject_cast<TaskItemWidget*>(ui->listWidget->itemWidget(item));
        if (!widget) continue;

        // 如果用户选择仅对未配置任务生效，且该任务已手动配置，则跳过
        if (preset.onlyUnconfigured && widget->isConfigured()) {
            continue;
        }

        widget->setPendingPreset(preset);
    }
}

void MainWindow::on_btnOpen_clicked()
{
    on_actionopen_triggered();
}

void MainWindow::on_btnCenterSelect_clicked()
{
    on_actionopen_triggered();
}

void MainWindow::on_actionopen_triggered()
{
    QStringList selectedFiles = QFileDialog::getOpenFileNames(
        this,
        "选择音视频文件",
        "",
        "音视频媒体文件 (*.mp4 *.mkv *.avi *.mov *.wmv *.flv *.webm *.ts *.rmvb *.m4v *.mp3 *.aac *.flac *.wav);;所有文件 (*.*)"
    );

    if (!selectedFiles.isEmpty()) {
        for (const QString &file : selectedFiles) {
            addFileToList(file);
        }
    }
}

void MainWindow::on_btnOpenFolder_clicked()
{
    on_actionOpenFolder_triggered();
}

void MainWindow::on_actionOpenFolder_triggered()
{
    QString selectedDir = QFileDialog::getExistingDirectory(
        this,
        "选择包含媒体文件的文件夹",
        ""
    );

    if (!selectedDir.isEmpty()) {
        addFolderToList(selectedDir);
    }
}

void MainWindow::on_btnStartAll_clicked()
{
    on_actionStartAll_triggered();
}

void MainWindow::on_actionStartAll_triggered()
{
    if (ui->listWidget->count() == 0) {
        QMessageBox::information(this, "提示", "任务列表为空，请先添加待转码文件。");
        return;
    }

    int unconfiguredCount = 0;
    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem *item = ui->listWidget->item(i);
        if (!item) continue;
        TaskItemWidget *w = qobject_cast<TaskItemWidget*>(ui->listWidget->itemWidget(item));
        if (w && !w->isConfigured()) {
            unconfiguredCount++;
        }
    }

    // 若存在未配置参数的任务，智能引导使用批量预设，杜绝逐个弹窗轰炸
    if (unconfiguredCount > 0) {
        QMessageBox::StandardButton btn = QMessageBox::question(
            this,
            "批量转码设置提醒",
            QString("当前列表中有 %1 个任务尚未配置转码参数。\n\n是否打开【⚡ 批量预设】一键统一配置并立即开始？\n（点击【No】将只执行已配置就绪的任务）")
                .arg(unconfiguredCount),
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
            QMessageBox::Yes
        );

        if (btn == QMessageBox::Yes) {
            on_actionBatchPreset_triggered();
            return;
        } else if (btn == QMessageBox::Cancel) {
            return;
        }
    }

    // 针对每个未配置任务拉起解析与启动
    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem *item = ui->listWidget->item(i);
        if (!item) continue;
        TaskItemWidget *widget = qobject_cast<TaskItemWidget*>(ui->listWidget->itemWidget(item));
        if (widget) {
            widget->triggerConfigureOrStart();
        }
    }
}

void MainWindow::on_btnStopAll_clicked()
{
    on_actionStopAll_triggered();
}

void MainWindow::on_actionStopAll_triggered()
{
    // 遍历任务停止
    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem *item = ui->listWidget->item(i);
        if (!item) continue;
        int taskId = item->data(Qt::UserRole).toInt();
        taskManager->stop_task(taskId);
    }
    m_runningCount = 0;
    updateStatusBar();
}

void MainWindow::on_btnClearFinished_clicked()
{
    on_actionClearFinished_triggered();
}

void MainWindow::on_actionClearFinished_triggered()
{
    for (int i = ui->listWidget->count() - 1; i >= 0; --i) {
        QListWidgetItem *item = ui->listWidget->item(i);
        if (!item) continue;
        TaskItemWidget *widget = qobject_cast<TaskItemWidget*>(ui->listWidget->itemWidget(item));
        if (widget) {
            // 取下该 item
            QListWidgetItem *taken = ui->listWidget->takeItem(i);
            taskManager->delete_task(widget->getTaskId());
            delete taken;
        }
    }
    m_finishedCount = 0;
    m_errorCount = 0;
    updatePageBasedOnList();
    updateStatusBar();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(
        this,
        "关于 G-Codec",
        "<h3>G-Codec v1.0</h3>"
        "<p><b>高性能硬件加速音视频转码工具</b></p>"
        "<p>核心特性：</p>"
        "<ul>"
        "<li>全链路 NVDEC 硬解 ➔ scale_cuda 硬件滤镜 ➔ NVENC 硬编（GDDR6 显存全程零拷贝）</li>"
        "<li>基于双条件变量的有界阻塞队列架构，彻底杜绝 CPU 忙等空转</li>"
        "<li>多格式硬件编码器动态自适应选择与混合降级安全桥接通道</li>"
        "<li>现代化深色系极简设计与高吞吐并发控制流水线</li>"
        "</ul>"
    );
}

void MainWindow::on_actionquit_triggered()
{
    this->close();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn = QMessageBox::question(
        this,
        "确认退出",
        "确定要退出 G-Codec 吗？\n如果有正在进行的转码任务将会被中止。",
        QMessageBox::No | QMessageBox::Yes,
        QMessageBox::No
    );

    if (resBtn != QMessageBox::Yes) {
        event->ignore();
    } else {
        event->accept();
    }
}
