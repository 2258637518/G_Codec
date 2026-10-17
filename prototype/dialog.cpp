#include "dialog.h"
#include "ui_dialog.h"

#include <QFileDialog>
#include <QMessageBox>

Dialog::Dialog(SourcecodeInfo info,QWidget *parent)
    : info(info),QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);

    // 1. 初始化封装格式下拉框
    ui->formatCombo->clear();
    ui->formatCombo->addItem("请选择目标封装格式", "");
    ui->formatCombo->addItem("MP4", "mp4");
    ui->formatCombo->addItem("MKV", "mkv");
    ui->formatCombo->addItem("MOV", "mov");
    ui->formatCombo->addItem("WebM", "webm");

    // 2. 格式与音视频编码器动态联动
    auto updateCodecs = [this](const QString &format) {
        ui->videoCodecCombo->clear();
        ui->audioCodecCombo->clear();
        if (format == "MP4") {
            ui->videoCodecCombo->addItem("H.264 / AVC", "libx264");
            ui->videoCodecCombo->addItem("H.265 / HEVC", "libx265");
            ui->videoCodecCombo->addItem("AV1", "libsvtav1");

            ui->audioCodecCombo->addItem("AAC", "aac");
            ui->audioCodecCombo->addItem("MP3", "libmp3lame");
        }
        else if (format == "MKV") {
            ui->videoCodecCombo->addItem("H.264 / AVC", "libx264");
            ui->videoCodecCombo->addItem("H.265 / HEVC", "libx265");
            ui->videoCodecCombo->addItem("AV1", "libsvtav1");
            ui->videoCodecCombo->addItem("VP9", "libvpx-vp9");

            ui->audioCodecCombo->addItem("AAC", "aac");
            ui->audioCodecCombo->addItem("MP3", "libmp3lame");
            ui->audioCodecCombo->addItem("Opus", "libopus");
            ui->audioCodecCombo->addItem("FLAC", "flac");
        }
        else if (format == "WebM") {
            ui->videoCodecCombo->addItem("VP9", "libvpx-vp9");
            ui->videoCodecCombo->addItem("AV1", "libsvtav1");

            ui->audioCodecCombo->addItem("Opus", "libopus");
            ui->audioCodecCombo->addItem("Vorbis", "libvorbis");
        }
        else if (format == "MOV") {
            ui->videoCodecCombo->addItem("H.264 / AVC", "libx264");
            ui->videoCodecCombo->addItem("H.265 / HEVC", "libx265");
            ui->videoCodecCombo->addItem("Apple ProRes", "prores_ks");

            ui->audioCodecCombo->addItem("AAC", "aac");
            ui->audioCodecCombo->addItem("PCM (16-bit)", "pcm_s16le");
        }
    };
    connect(ui->formatCombo, &QComboBox::currentTextChanged, this, updateCodecs);

    // 3. 初始化帧率下拉框 (绑定 double)
    ui->framerateCombo->clear();
    ui->framerateCombo->addItem("默认 (保持原帧率)", -1.0);
    ui->framerateCombo->addItem("24 fps", 24.0);
    ui->framerateCombo->addItem("30 fps", 30.0);
    ui->framerateCombo->addItem("60 fps", 60.0);
    ui->framerateCombo->addItem("120 fps", 120.0);

    // 4. 初始化视频码率下拉框 (绑定实际 bps 整数值)
    ui->videoBitrateCombo->clear();
    ui->videoBitrateCombo->addItem("默认 (2 Mbps)", -1);
    ui->videoBitrateCombo->addItem("3000 kbps", 3000 * 1000);
    ui->videoBitrateCombo->addItem("6000 kbps", 6000 * 1000);
    ui->videoBitrateCombo->addItem("10000 kbps", 10000 * 1000);
    ui->videoBitrateCombo->addItem("20000 kbps", 20000 * 1000);

    // 5. 初始化分辨率下拉框 (绑定 QSize)
    ui->resolutionCombo->clear();
    ui->resolutionCombo->addItem("默认 (保持原分辨率)", QSize(-1, -1));
    ui->resolutionCombo->addItem("854 × 480 (480P)", QSize(854, 480));
    ui->resolutionCombo->addItem("1280 × 720 (720P)", QSize(1280, 720));
    ui->resolutionCombo->addItem("1920 × 1080 (1080P)", QSize(1920, 1080));
    ui->resolutionCombo->addItem("2560 × 1440 (2K)", QSize(2560, 1440));
    ui->resolutionCombo->addItem("3840 × 2160 (4K)", QSize(3840, 2160));
    ui->resolutionCombo->addItem("720 × 1280 (竖屏 720P)", QSize(720, 1280));
    ui->resolutionCombo->addItem("1080 × 1920 (竖屏 1080P)", QSize(1080, 1920));
    ui->resolutionCombo->addItem("1440 × 2560 (竖屏 2K)", QSize(1440, 2560));

    // 6. 初始化音频码率下拉框
    ui->audioBitrateCombo->clear();
    ui->audioBitrateCombo->addItem("默认 (128 kbps)", -1);
    ui->audioBitrateCombo->addItem("96 kbps", 96000);
    ui->audioBitrateCombo->addItem("128 kbps", 128000);
    ui->audioBitrateCombo->addItem("192 kbps", 192000);
    ui->audioBitrateCombo->addItem("320 kbps", 320000);

    // 7. 初始化音频采样率下拉框
    ui->audioSampleCombo->clear();
    ui->audioSampleCombo->addItem("默认 (保持原采样率)", -1);
    ui->audioSampleCombo->addItem("8000 Hz", 8000);
    ui->audioSampleCombo->addItem("22050 Hz", 22050);
    ui->audioSampleCombo->addItem("44100 Hz", 44100);
    ui->audioSampleCombo->addItem("48000 Hz", 48000);
    ui->audioSampleCombo->addItem("96000 Hz", 96000);
    ui->audioSampleCombo->addItem("192000 Hz", 192000);

    // 8. 初始化音频声道下拉框
    ui->audioChannelCombo->clear();
    ui->audioChannelCombo->addItem("默认 (保持原声道)", -1);
    ui->audioChannelCombo->addItem("单声道 (1)", 1);
    ui->audioChannelCombo->addItem("立体声 (2)", 2);
    ui->audioChannelCombo->addItem("5.1 环绕声 (6)", 6);
    ui->audioChannelCombo->addItem("7.1 环绕声 (8)", 8);

    // 9. 显示源视频元数据
    ui->label_2->setText(info.inputFormat);
    ui->label_4->setText(info.videoCodec);
    ui->label_6->setText(QString::number(info.frameRate.den > 0 ? av_q2d(info.frameRate) : 0.0, 'f', 2));
    ui->label_8->setText(QString::number(info.videoBitrate) + " kbps");
    ui->label_10->setText(QString::number(info.width) + "x" + QString::number(info.height));
    ui->label_17->setText(info.audioCodec);
    ui->label_18->setText(QString::number(info.audioBitrate) + " bps");
    ui->label_19->setText(QString::number(info.audioSampleRate) + " Hz");
    ui->label_20->setText(QString::number(info.audioChannels));
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_buttonBox_accepted()
{
    task_info.audioBitrate = ui->audioBitrateCombo->currentData().toInt();
    task_info.audioChannels = ui->audioChannelCombo->currentData().toInt();
    task_info.audioCodec = ui->audioCodecCombo->currentData().toString();
    task_info.audioSampleRate = ui->audioSampleCombo->currentData().toInt();
    task_info.frameRate = ui->framerateCombo->currentData().toDouble();
    task_info.videoBitrate = ui->videoBitrateCombo->currentData().toInt();
    task_info.videoCodec = ui->videoCodecCombo->currentData().toString();

    QSize res = ui->resolutionCombo->currentData().toSize();
    if (res.isValid() && res.width() > 0 && res.height() > 0) {
        task_info.width = res.width();
        task_info.height = res.height();
    } else {
        task_info.width = -1;
        task_info.height = -1;
    }

    task_info.outputFormat = ui->formatCombo->currentData().toString();

    QString time  = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");

    // 1. 改用 currentText() 获取下拉框当前显示的文本，并去除两端空格
    QString formatVal = ui->formatCombo->currentText().trimmed();
    QString targetExt = "";

    // 2. 判断选择逻辑（包含 MOV 支持）
    if (formatVal == "请选择目标封装格式" || formatVal.isEmpty()) {
        // 下拉框未选具体格式，回退读取 label_2
        targetExt = ui->label_2->text().trimmed();
        if (targetExt.contains(":")) {
            targetExt = targetExt.section(':', -1).trimmed(); // 过滤类似 "格式: mp4" 中的前缀
        }
        if (!targetExt.isEmpty() && !targetExt.startsWith(".")) {
            targetExt = "." + targetExt;
        }
    } else if (formatVal.contains("MP4", Qt::CaseInsensitive)) {
        targetExt = ".mp4";
    } else if (formatVal.contains("MKV", Qt::CaseInsensitive)) {
        targetExt = ".mkv";
    } else if (formatVal.contains("MOV", Qt::CaseInsensitive)) {
        targetExt = ".mov";
    } else if (formatVal.contains("WebM", Qt::CaseInsensitive)) {
        targetExt = ".webm";
    }

    // 若 label_2 为空或解析失败，默认保底为 .mp4
    if (targetExt.isEmpty()) {
        targetExt = ".mp4";
    }

    // 3. 根据 targetExt 匹配对话框默认高亮的过滤器（必须与 filterList 选项一字不差）
    QString selectedFilter = tr("MP4 视频 (*.mp4)");
    if (targetExt.compare(".mkv", Qt::CaseInsensitive) == 0) {
        selectedFilter = tr("MKV 视频 (*.mkv)");
    } else if (targetExt.compare(".mov", Qt::CaseInsensitive) == 0) {
        selectedFilter = tr("MOV 视频 (*.mov)");
    } else if (targetExt.compare(".webm", Qt::CaseInsensitive) == 0) {
        selectedFilter = tr("WebM 视频 (*.webm)");
    } else if (targetExt.compare(".mp4", Qt::CaseInsensitive) == 0) {
        selectedFilter = tr("MP4 视频 (*.mp4)");
    } else {
        selectedFilter = tr("所有文件 (*.*)");
    }

    // 4. 将 targetExt 拼接到默认文件名中，保证打开对话框时输入框内直接显示对应后缀
    QString defaultFileName = "SlientCodec" + time + targetExt;
    QString filterList = tr("MP4 视频 (*.mp4);;MKV 视频 (*.mkv);;MOV 视频 (*.mov);;WebM 视频 (*.webm);;所有文件 (*.*)");

    // 5. 弹出保存文件对话框
    QString selectedFilePath = QFileDialog::getSaveFileName(
        this,
        tr("选择转码视频保存位置"),
        defaultFileName,
        filterList,
        &selectedFilter
        );

    // 6. 用户取消则退出
    if (selectedFilePath.isEmpty()) {
        return;
    }

    // 7. 兜底补齐（防止用户在弹出的输入框中手动删除了后缀名）
    QFileInfo fileInfo(selectedFilePath);
    if (fileInfo.suffix().isEmpty()) {
        if (selectedFilter.contains("*.mkv")) {
            selectedFilePath += ".mkv";
        } else if (selectedFilter.contains("*.mov")) {
            selectedFilePath += ".mov";
        } else if (selectedFilter.contains("*.webm")) {
            selectedFilePath += ".webm";
        } else if (selectedFilter.contains("*.mp4")) {
            selectedFilePath += ".mp4";
        } else {
            selectedFilePath += targetExt;
        }
    }

    //如果文件已经存在，弹出警告框让用户二次确认
    if(QFile::exists(selectedFilePath)) {
        QMessageBox::StandardButton
        resBtn = QMessageBox::warning(
            this,
            "文件已存在",
            "目标视频文件已存在，确定要覆盖它吗？",
            QMessageBox::No |QMessageBox::Yes,
            QMessageBox::No
            );
        if (resBtn != QMessageBox::Yes)
        {
            return; //用户选择“否”，不保存，退回配置界面（或让其重新选择）
        }
    }
    task_info.outputPath = selectedFilePath;
    emit taskDataReady(task_info);
    this->accept();
}

void Dialog::on_buttonBox_rejected()
{
    this->reject();
}
