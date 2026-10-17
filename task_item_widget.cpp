#include "task_item_widget.h"
#include "./ui_task_item_widget.h"

#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QStyle>
#include <QDebug>
#include "window_style_helper.h"

TaskItemWidget::TaskItemWidget(QString filepath, QWidget *parent)
    : filepath(filepath), QWidget(parent), ui(new Ui::TaskItemWidget)
{
    ui->setupUi(this);

    QFileInfo fi(filepath);
    QString ext = fi.suffix().toUpper();
    ui->badgeFormat->setText(ext.isEmpty() ? "VID" : ext);

    // 根据扩展名呈现不同的专业配色角标
    if (ext == "MP4") {
        ui->badgeFormat->setStyleSheet("background-color: #0c4a6e; color: #38bdf8; border: 1px solid #0284c7; border-radius: 5px; font-weight: 700;");
    } else if (ext == "MKV") {
        ui->badgeFormat->setStyleSheet("background-color: #3b0764; color: #c084fc; border: 1px solid #6b21a8; border-radius: 5px; font-weight: 700;");
    } else if (ext == "MOV") {
        ui->badgeFormat->setStyleSheet("background-color: #1e1b4b; color: #818cf8; border: 1px solid #3730a3; border-radius: 5px; font-weight: 700;");
    } else if (ext == "WEBM") {
        ui->badgeFormat->setStyleSheet("background-color: #14532d; color: #4ade80; border: 1px solid #15803d; border-radius: 5px; font-weight: 700;");
    }

    ui->labelFileName->setText(fi.fileName());
    ui->labelFileName->setToolTip(filepath);
    ui->labelSpecs->setText("待解析源规格参数...");
    ui->labelStatus->setText("● 就绪");
    ui->labelProgressVal->setText("0%");

    // 安装状态标签事件过滤器以支持点击诊断错误
    ui->labelStatus->installEventFilter(this);

    connect(ui->btnConfig, &QPushButton::clicked, this, &TaskItemWidget::triggerConfigureOrStart);
}

TaskItemWidget::~TaskItemWidget()
{
    delete ui;
}

void TaskItemWidget::setItemInfo(SourcecodeInfo info)
{
    this->info = info;
    hasInfo = true;

    double fps = (info.frameRate.den > 0) ? av_q2d(info.frameRate) : 0.0;
    QString specStr = QString("%1 × %2  |  %3 fps  |  %4  |  %5")
                          .arg(info.width)
                          .arg(info.height)
                          .arg(QString::number(fps, 'f', 0))
                          .arg(info.videoCodec.isEmpty() ? "未知视频编码" : info.videoCodec)
                          .arg(info.audioCodec.isEmpty() ? "无音频流" : info.audioCodec);
    ui->labelSpecs->setText(specStr);

    ui->pushButton->setEnabled(true);
    ui->btnConfig->setEnabled(true);

    if (m_hasPendingPreset) {
        // 如果存在批量预设，直接套用预设，避免弹窗打扰
        m_hasPendingPreset = false;
        applyBatchPreset(m_pendingPreset);
    } else {
        ui->labelStatus->setText("● 待配置");
        ui->labelStatus->setStyleSheet("color: #f59e0b; font-weight: 600;");
        // 解析完成后触发转码配置窗口
        triggerConfigureOrStart();
    }
}

void TaskItemWidget::status(TaskStatus status)
{
    m_currentStatus = status;
    switch (status) {
    case TaskStatus::Waiting:
        ui->labelStatus->setText("● 排队等待");
        ui->labelStatus->setStyleSheet("color: #94a3b8; font-weight: 600;");
        ui->labelStatus->setCursor(Qt::ArrowCursor);
        ui->labelStatus->setToolTip("");
        ui->pushButton->setEnabled(false);
        ui->pushButton->setText("排队中");
        break;
    case TaskStatus::Running:
        ui->labelStatus->setText("● 正在硬件转码");
        ui->labelStatus->setStyleSheet("color: #38bdf8; font-weight: 600;");
        ui->labelStatus->setCursor(Qt::ArrowCursor);
        ui->labelStatus->setToolTip("");
        ui->pushButton->setEnabled(false);
        ui->pushButton->setText("转码中");
        break;
    case TaskStatus::Finished:
        ui->labelStatus->setText("✔ 已完成");
        ui->labelStatus->setStyleSheet("color: #34d399; font-weight: 600;");
        ui->labelStatus->setCursor(Qt::ArrowCursor);
        ui->labelStatus->setToolTip("");
        ui->pushButton->setEnabled(true);
        ui->pushButton->setText("查看文件");
        ui->pushButton->setProperty("success", true);
        ui->pushButton->style()->unpolish(ui->pushButton);
        ui->pushButton->style()->polish(ui->pushButton);
        ui->progressBar->setValue(100);
        ui->labelProgressVal->setText("100%");
        break;
    case TaskStatus::Error:
        ui->labelStatus->setText("✕ 转码失败 [点击诊断]");
        ui->labelStatus->setStyleSheet("color: #f87171; font-weight: 700; text-decoration: underline;");
        ui->labelStatus->setCursor(Qt::PointingHandCursor);
        if (!m_errorTitle.isEmpty() || !m_errorDetail.isEmpty()) {
            ui->labelStatus->setToolTip(QString("【%1】\n%2\n\n💡 点击可查看详细错误原因与排查建议").arg(m_errorTitle.isEmpty() ? "转码失败" : m_errorTitle, m_errorDetail));
        } else {
            ui->labelStatus->setToolTip("转码发生错误，点击查看详情或尝试重新配置。");
        }
        ui->pushButton->setEnabled(true);
        ui->pushButton->setText("重试配置");
        break;
    }
}

void TaskItemWidget::setErrorInfo(const QString &title, const QString &detail)
{
    m_errorTitle = title;
    m_errorDetail = detail;
    if (m_currentStatus == TaskStatus::Error) {
        ui->labelStatus->setToolTip(QString("【%1】\n%2\n\n💡 点击可查看详细错误原因与排查建议").arg(m_errorTitle, m_errorDetail));
    }
}

void TaskItemWidget::showErrorDialog()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("G-Codec 错误诊断详情");
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(QString("<h3>转码任务失败：%1</h3>").arg(m_errorTitle.isEmpty() ? "未知转码异常" : m_errorTitle));
    QString detailHtml = QString(
        "<p><b>源文件路径：</b><br>%1</p>"
        "<p><b>底层原因与诊断：</b><br><span style='color: #f87171;'>%2</span></p>"
        "<hr>"
        "<p><b>💡 常见排查指引：</b><br>"
        "• <b>编码器不兼容：</b> 若选用了 GPU 硬编但硬件不支持此规格，请改选 CPU 编码或选择通用 1080P 分辨率。<br>"
        "• <b>磁盘与写权限：</b> 请检查目标盘空间是否充足、是否有写权限，且文件未被播放器占用。<br>"
        "• <b>封装格式限制：</b> 部分封装容器 (如 .flv) 不支持现代音频格式，建议改用 .mp4 或 .mkv 格式。"
        "</p>"
    ).arg(filepath, m_errorDetail.toHtmlEscaped().replace("\n", "<br>"));
    msgBox.setInformativeText(detailHtml);
    msgBox.setStandardButtons(QMessageBox::Ok);
    WindowStyleHelper::applyDarkWindowFrame(&msgBox);
    msgBox.exec();
}

bool TaskItemWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->labelStatus && event->type() == QEvent::MouseButtonPress) {
        if (m_currentStatus == TaskStatus::Error) {
            showErrorDialog();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void TaskItemWidget::setPendingPreset(const BatchPreset &preset)
{
    m_pendingPreset = preset;
    m_hasPendingPreset = true;
    if (!hasInfo) {
        ui->pushButton->setEnabled(false);
        ui->labelStatus->setText("● 批量解析中...");
        ui->labelStatus->setStyleSheet("color: #38bdf8; font-weight: 600;");
        emit requestParseMetadata();
    } else {
        applyBatchPreset(preset);
    }
}

void TaskItemWidget::applyBatchPreset(const BatchPreset &preset)
{
    QFileInfo fi(filepath);
    QString targetExt = "." + preset.outputFormat.toLower();

    // 确定目标输出文件夹
    QString targetDir = preset.customOutputDir;
    if (preset.useSourceDir || targetDir.isEmpty()) {
        targetDir = fi.absolutePath();
    }

    // 确定目标文件名，避免覆盖源文件
    QString baseName = fi.completeBaseName();
    QString suffix = preset.fileSuffix;
    QString targetFileName = baseName + suffix + targetExt;
    QString fullOutputPath = QDir(targetDir).filePath(targetFileName);

    // 严禁输出路径与输入源路径完全相同
    if (QFileInfo(fullOutputPath).canonicalFilePath() == fi.canonicalFilePath()) {
        targetFileName = baseName + "_transcoded" + targetExt;
        fullOutputPath = QDir(targetDir).filePath(targetFileName);
    }

    TranscodeTask task;
    task.inputPath = filepath;
    task.outputPath = fullOutputPath;
    task.outputFormat = preset.outputFormat;
    task.videoCodec = preset.videoCodec;
    task.audioCodec = preset.audioCodec;
    task.width = preset.width;
    task.height = preset.height;
    task.frameRate = preset.frameRate;
    task.videoBitrate = preset.videoBitrate;
    task.audioBitrate = preset.audioBitrate;
    task.audioSampleRate = preset.audioSampleRate;
    task.audioChannels = preset.audioChannels;

    save_path = fullOutputPath;
    m_isConfigured = true;

    // 更新 UI 规格显示
    QString currentSpecs = ui->labelSpecs->text();
    if (currentSpecs.contains("➔")) {
        currentSpecs = currentSpecs.section("➔", 0, 0).trimmed();
    }
    QString targetDesc = QString("  ➔  目标: %1 (%2)").arg(task.outputFormat.toUpper(), task.videoCodec);
    ui->labelSpecs->setText(currentSpecs + targetDesc);

    emit send_task_info(this, task);
}

void TaskItemWidget::on_pushButton_2_clicked()
{
    emit delete_current_item();
}

void TaskItemWidget::triggerConfigureOrStart()
{
    if (ui->pushButton->text() == "查看文件") {
        on_pushButton_clicked();
        return;
    }

    if (!hasInfo) {
        ui->pushButton->setEnabled(false);
        ui->labelStatus->setText("● 解析中...");
        ui->labelStatus->setStyleSheet("color: #38bdf8; font-weight: 600;");
        emit requestParseMetadata();
        return;
    }

    TranscodeDialog dialog(info, this);
    connect(&dialog, &TranscodeDialog::taskDataReady, this, &TaskItemWidget::onDialogDataReady);

    int ret = dialog.exec();
    if (ret == QDialog::Accepted) {
        m_isConfigured = true;
        if (dialog.applyToAll()) {
            emit configAppliedToAllRequested(m_lastTask);
        }
        qDebug() << "任务参数配置完成，就绪并排队......";
    } else {
        qDebug() << "用户取消配置";
    }
}

void TaskItemWidget::on_pushButton_clicked()
{
    if (ui->pushButton->text() == "查看文件") {
        if (!save_path.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(save_path).absolutePath()));
        }
        return;
    }

    triggerConfigureOrStart();
}

void TaskItemWidget::onDialogDataReady(const TranscodeTask &basicTask)
{
    m_lastTask = basicTask;
    save_path = basicTask.outputPath;
    m_isConfigured = true;

    QString currentSpecs = ui->labelSpecs->text();
    if (currentSpecs.contains("➔")) {
        currentSpecs = currentSpecs.section("➔", 0, 0).trimmed();
    }
    QString targetDesc = QString("  ➔  目标: %1 (%2)").arg(basicTask.outputFormat.toUpper(), basicTask.videoCodec);
    ui->labelSpecs->setText(currentSpecs + targetDesc);

    emit send_task_info(this, basicTask);
}

void TaskItemWidget::progress_updata(int taskId, int val)
{
    if (this->taskId == taskId) {
        ui->progressBar->setValue(val);
        ui->labelProgressVal->setText(QString::number(val) + "%");
        ui->labelStatus->setText(QString("● 转码中 %1%").arg(val));
    }
}
