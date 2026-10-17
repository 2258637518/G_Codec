#include "transcode_dialog.h"
#include "ui_transcode_dialog.h"
#include "window_style_helper.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QCheckBox>

TranscodeDialog::TranscodeDialog(SourcecodeInfo info, QWidget *parent)
    : info(info), QDialog(parent)
    , ui(new Ui::TranscodeDialog)
{
    ui->setupUi(this);

    // 适配 Windows 原生 DWM 沉浸式深色窗口标题栏与边框，消除白色边框不协调感
    WindowStyleHelper::applyDarkWindowFrame(this);

    // 增加批量应用复选框
    m_chkApplyToAll = new QCheckBox("同时将此参数应用到其他未配置任务", this);
    m_chkApplyToAll->setStyleSheet("color: #38bdf8; font-size: 12px; font-weight: 500;");
    ui->bottomActionsLayout->insertWidget(0, m_chkApplyToAll);

    // 1. 初始化封装格式下拉框（主流工业级全覆盖）
    ui->formatCombo->clear();
    ui->formatCombo->addItem("请选择目标封装格式", "");
    ui->formatCombo->addItem("MP4 格式 (*.mp4) - 全球通用流行封装", "mp4");
    ui->formatCombo->addItem("MKV 格式 (*.mkv) - 全能蓝光/多音轨封装", "mkv");
    ui->formatCombo->addItem("MOV 格式 (*.mov) - Apple 专业影视级", "mov");
    ui->formatCombo->addItem("WebM 格式 (*.webm) - 网页无专利高压缩", "webm");
    ui->formatCombo->addItem("TS 格式 (*.ts) - 广播传输与切片流", "ts");
    ui->formatCombo->addItem("FLV 格式 (*.flv) - 网络直播推流标准", "flv");
    ui->formatCombo->addItem("AVI 格式 (*.avi) - 经典多媒体容器", "avi");

    // 2. 格式与音视频编码器专业动态联动
    auto updateCodecs = [this](const QString &format) {
        ui->videoCodecCombo->clear();
        ui->audioCodecCombo->clear();

        if (format.contains("MP4")) {
            ui->videoCodecCombo->addItem("H.264 / AVC ⚡ [GPU 硬件加速 · 极速]", "libx264");
            ui->videoCodecCombo->addItem("H.265 / HEVC ⚡ [GPU 硬件加速 · 高压缩率]", "libx265");
            ui->videoCodecCombo->addItem("AV1 ⏱ [CPU 软编 · 下一代免专利]", "libsvtav1");
            ui->videoCodecCombo->addItem("MPEG-4 Part 2 ⏱ [CPU 软编 · 经典兼容]", "mpeg4");

            ui->audioCodecCombo->addItem("AAC (通用流媒体标准)", "aac");
            ui->audioCodecCombo->addItem("MP3 (广泛跨平台兼容)", "libmp3lame");
            ui->audioCodecCombo->addItem("AC-3 (Dolby Digital 杜比立体声/5.1)", "ac3");
        }
        else if (format.contains("MKV")) {
            ui->videoCodecCombo->addItem("H.264 / AVC ⚡ [GPU 硬件加速 · 极速]", "libx264");
            ui->videoCodecCombo->addItem("H.265 / HEVC ⚡ [GPU 硬件加速 · 蓝光原盘]", "libx265");
            ui->videoCodecCombo->addItem("Apple ProRes 422 ⏱ [CPU 软编 · 影视母带]", "prores_ks");
            ui->videoCodecCombo->addItem("VP9 ⏱ [CPU 软编 · Google 高清]", "libvpx-vp9");
            ui->videoCodecCombo->addItem("AV1 ⏱ [CPU 软编 · 下一代免专利]", "libsvtav1");
            ui->videoCodecCombo->addItem("MPEG-2 Video ⏱ [CPU 软编 · 传统广播]", "mpeg2video");

            ui->audioCodecCombo->addItem("AAC (通用高保真)", "aac");
            ui->audioCodecCombo->addItem("Opus (现代顶级音质 / 低延迟)", "libopus");
            ui->audioCodecCombo->addItem("FLAC (Hi-Res 无损母带)", "flac");
            ui->audioCodecCombo->addItem("AC-3 (Dolby Digital 杜比数字)", "ac3");
            ui->audioCodecCombo->addItem("E-AC-3 (Dolby Digital Plus 杜比视界标配)", "eac3");
            ui->audioCodecCombo->addItem("PCM (16-bit 无损)", "pcm_s16le");
            ui->audioCodecCombo->addItem("MP3 (广泛兼容)", "libmp3lame");
        }
        else if (format.contains("MOV")) {
            ui->videoCodecCombo->addItem("H.264 / AVC ⚡ [GPU 硬件加速 · 极速]", "libx264");
            ui->videoCodecCombo->addItem("H.265 / HEVC ⚡ [GPU 硬件加速 · 极速]", "libx265");
            ui->videoCodecCombo->addItem("Apple ProRes 422 ⏱ [CPU 软编 · 影视剪辑直出]", "prores_ks");
            ui->videoCodecCombo->addItem("MJPEG ⏱ [CPU 软编 · 逐帧采集]", "mjpeg");

            ui->audioCodecCombo->addItem("AAC (通用高质量)", "aac");
            ui->audioCodecCombo->addItem("PCM (16-bit CD 级无损)", "pcm_s16le");
            ui->audioCodecCombo->addItem("PCM (24-bit 录音室母带)", "pcm_s24le");
            ui->audioCodecCombo->addItem("ALAC (Apple Lossless 苹果无损)", "alac");
        }
        else if (format.contains("WebM")) {
            ui->videoCodecCombo->addItem("VP9 ⏱ [CPU 软编 · YouTube 标配]", "libvpx-vp9");
            ui->videoCodecCombo->addItem("AV1 ⏱ [CPU 软编 · 下一代免专利]", "libsvtav1");
            ui->videoCodecCombo->addItem("VP8 ⏱ [CPU 软编 · 传统网页兼容]", "libvpx");

            ui->audioCodecCombo->addItem("Opus (现代低延迟高品质标配)", "libopus");
            ui->audioCodecCombo->addItem("Vorbis (经典开源音频)", "libvorbis");
        }
        else if (format.contains("TS")) {
            ui->videoCodecCombo->addItem("H.264 / AVC ⚡ [GPU 硬件加速 · 广播与直播]", "libx264");
            ui->videoCodecCombo->addItem("H.265 / HEVC ⚡ [GPU 硬件加速 · 4K广播切片]", "libx265");
            ui->videoCodecCombo->addItem("MPEG-2 Video ⏱ [CPU 软编 · 经典广电标准]", "mpeg2video");

            ui->audioCodecCombo->addItem("AAC (广播级流媒体推荐)", "aac");
            ui->audioCodecCombo->addItem("AC-3 (杜比环绕声)", "ac3");
            ui->audioCodecCombo->addItem("MP3 (通用音频)", "libmp3lame");
        }
        else if (format.contains("FLV")) {
            ui->videoCodecCombo->addItem("H.264 / AVC ⚡ [GPU 硬件加速 · RTMP 黄金标准]", "libx264");

            ui->audioCodecCombo->addItem("AAC (直播通用推流)", "aac");
            ui->audioCodecCombo->addItem("MP3 (经典 Flash 流媒体)", "libmp3lame");
        }
        else if (format.contains("AVI")) {
            ui->videoCodecCombo->addItem("H.264 / AVC ⚡ [GPU 硬件加速 · 现代兼容]", "libx264");
            ui->videoCodecCombo->addItem("MPEG-4 Part 2 ⏱ [CPU 软编 · DivX/Xvid 兼容]", "mpeg4");
            ui->videoCodecCombo->addItem("MJPEG ⏱ [CPU 软编 · 逐帧高质量编辑]", "mjpeg");

            ui->audioCodecCombo->addItem("MP3 (经典通用音频)", "libmp3lame");
            ui->audioCodecCombo->addItem("PCM (16-bit 原始波形)", "pcm_s16le");
            ui->audioCodecCombo->addItem("AC-3 (杜比数字音频)", "ac3");
        }
    };
    connect(ui->formatCombo, &QComboBox::currentTextChanged, this, updateCodecs);

    // 3. 初始化帧率下拉框 (包含国际广电标准帧率)
    ui->framerateCombo->clear();
    ui->framerateCombo->addItem("保持源帧率 (推荐)", -1.0);
    ui->framerateCombo->addItem("24 fps (电影胶片标准)", 24.0);
    ui->framerateCombo->addItem("25 fps (PAL / 中国与欧洲广电电视标准)", 25.0);
    ui->framerateCombo->addItem("29.97 fps (NTSC 美日广电标准)", 29.97);
    ui->framerateCombo->addItem("30 fps (网络流媒体标准)", 30.0);
    ui->framerateCombo->addItem("50 fps (PAL 高帧率电视广播)", 50.0);
    ui->framerateCombo->addItem("59.94 fps (NTSC 高帧率广播)", 59.94);
    ui->framerateCombo->addItem("60 fps (流畅高帧率)", 60.0);
    ui->framerateCombo->addItem("120 fps (高刷电竞 / 超慢动作)", 120.0);

    // 4. 初始化视频码率下拉框 (绑定实际 bps 整数值)
    ui->videoBitrateCombo->clear();
    ui->videoBitrateCombo->addItem("自适应智能码率 (推荐)", -1);
    ui->videoBitrateCombo->addItem("1500 kbps (720P 网络省流)", 1500 * 1000);
    ui->videoBitrateCombo->addItem("3000 kbps (1080P 网络轻量)", 3000 * 1000);
    ui->videoBitrateCombo->addItem("6000 kbps (1080P 高清标准)", 6000 * 1000);
    ui->videoBitrateCombo->addItem("10000 kbps (1080P 蓝光高码率)", 10000 * 1000);
    ui->videoBitrateCombo->addItem("15000 kbps (2K 极清画质)", 15000 * 1000);
    ui->videoBitrateCombo->addItem("25000 kbps (4K UHD 超清原盘)", 25000 * 1000);
    ui->videoBitrateCombo->addItem("50000 kbps (4K 影视母带级)", 50000 * 1000);

    // 5. 初始化分辨率下拉框 (绑定 QSize)
    ui->resolutionCombo->clear();
    ui->resolutionCombo->addItem("保持原始分辨率 (推荐)", QSize(-1, -1));
    ui->resolutionCombo->addItem("3840 × 2160 (4K UHD 超高清)", QSize(3840, 2160));
    ui->resolutionCombo->addItem("2560 × 1440 (2K QHD 极清)", QSize(2560, 1440));
    ui->resolutionCombo->addItem("1920 × 1080 (1080P 全高清)", QSize(1920, 1080));
    ui->resolutionCombo->addItem("1280 × 720 (720P 高清)", QSize(1280, 720));
    ui->resolutionCombo->addItem("854 × 480 (480P 标清)", QSize(854, 480));
    ui->resolutionCombo->addItem("1080 × 1920 (竖屏 1080P 短视频)", QSize(1080, 1920));
    ui->resolutionCombo->addItem("720 × 1280 (竖屏 720P 短视频)", QSize(720, 1280));

    // 6. 初始化音频码率下拉框
    ui->audioBitrateCombo->clear();
    ui->audioBitrateCombo->addItem("默认 128 kbps (标准)", -1);
    ui->audioBitrateCombo->addItem("64 kbps (语音高效)", 64000);
    ui->audioBitrateCombo->addItem("96 kbps (网络省流)", 96000);
    ui->audioBitrateCombo->addItem("128 kbps (标准音乐)", 128000);
    ui->audioBitrateCombo->addItem("192 kbps (高品质音乐)", 192000);
    ui->audioBitrateCombo->addItem("256 kbps (广播级高保真)", 256000);
    ui->audioBitrateCombo->addItem("320 kbps (最高品质)", 320000);

    // 7. 初始化音频采样率下拉框
    ui->audioSampleCombo->clear();
    ui->audioSampleCombo->addItem("保持原采样率 (推荐)", -1);
    ui->audioSampleCombo->addItem("44100 Hz (CD 级标准)", 44100);
    ui->audioSampleCombo->addItem("48000 Hz (专业影视广电标准)", 48000);
    ui->audioSampleCombo->addItem("96000 Hz (Hi-Res 高解析母带)", 96000);

    // 8. 初始化音频声道下拉框
    ui->audioChannelCombo->clear();
    ui->audioChannelCombo->addItem("保持原声道 (推荐)", -1);
    ui->audioChannelCombo->addItem("单声道 (Mono 1ch)", 1);
    ui->audioChannelCombo->addItem("立体声 (Stereo 2ch)", 2);
    ui->audioChannelCombo->addItem("5.1 环绕声 (Surround 6ch)", 6);
    ui->audioChannelCombo->addItem("7.1 环绕声 (Surround 8ch)", 8);

    // 9. 显示源视频元数据
    ui->label_2->setText(info.inputFormat);
    ui->label_4->setText(info.videoCodec);
    ui->label_6->setText(QString::number(info.frameRate.den > 0 ? av_q2d(info.frameRate) : 0.0, 'f', 2));
    ui->label_8->setText(QString::number(info.videoBitrate) + " kbps");
    ui->label_10->setText(QString::number(info.width) + " × " + QString::number(info.height));
    ui->label_17->setText(info.audioCodec);
    ui->label_18->setText(QString::number(info.audioBitrate) + " bps");
    ui->label_19->setText(QString::number(info.audioSampleRate) + " Hz");
    ui->label_20->setText(QString::number(info.audioChannels));

    // 10. 优化对话框确定/取消按钮外观
    QPushButton *okBtn = ui->buttonBox->button(QDialogButtonBox::Ok);
    if (okBtn) {
        okBtn->setText("确认并加入转码队列");
        okBtn->setProperty("primary", true);
    }
    QPushButton *cancelBtn = ui->buttonBox->button(QDialogButtonBox::Cancel);
    if (cancelBtn) {
        cancelBtn->setText("取消");
    }
}

TranscodeDialog::~TranscodeDialog()
{
    delete ui;
}

void TranscodeDialog::on_buttonBox_accepted()
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
    } else if (formatVal.contains("TS", Qt::CaseInsensitive)) {
        targetExt = ".ts";
    } else if (formatVal.contains("FLV", Qt::CaseInsensitive)) {
        targetExt = ".flv";
    } else if (formatVal.contains("AVI", Qt::CaseInsensitive)) {
        targetExt = ".avi";
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
    } else if (targetExt.compare(".ts", Qt::CaseInsensitive) == 0) {
        selectedFilter = tr("TS 传输流 (*.ts)");
    } else if (targetExt.compare(".flv", Qt::CaseInsensitive) == 0) {
        selectedFilter = tr("FLV 视频 (*.flv)");
    } else if (targetExt.compare(".avi", Qt::CaseInsensitive) == 0) {
        selectedFilter = tr("AVI 视频 (*.avi)");
    } else {
        selectedFilter = tr("MP4 视频 (*.mp4)");
    }

    // 4. 将 targetExt 拼接到默认文件名中，保证打开对话框时输入框内直接显示对应后缀
    QString defaultFileName = "G_Codec_" + time + targetExt;
    QString filterList = tr("MP4 视频 (*.mp4);;MKV 视频 (*.mkv);;MOV 视频 (*.mov);;WebM 视频 (*.webm);;TS 传输流 (*.ts);;FLV 视频 (*.flv);;AVI 视频 (*.avi);;所有文件 (*.*)");

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
        } else if (selectedFilter.contains("*.ts")) {
            selectedFilePath += ".ts";
        } else if (selectedFilter.contains("*.flv")) {
            selectedFilePath += ".flv";
        } else if (selectedFilter.contains("*.avi")) {
            selectedFilePath += ".avi";
        } else if (selectedFilter.contains("*.mp4")) {
            selectedFilePath += ".mp4";
        } else {
            selectedFilePath += targetExt;
        }
    }

    // 严禁将输出文件直接指定为输入源文件自身（防止打开写入时原文件被截断为 0 字节损坏）
    if (!info.inputPath.isEmpty() && QFileInfo(selectedFilePath).canonicalFilePath() == QFileInfo(info.inputPath).canonicalFilePath()) {
        QMessageBox::critical(
            this,
            "文件路径冲突",
            "目标输出路径不能与源视频文件相同！\n直接覆盖源文件会导致输入数据被立即清空损坏，请指定不同的文件名或目录。"
        );
        return;
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
    m_applyToAll = (m_chkApplyToAll && m_chkApplyToAll->isChecked());
    emit taskDataReady(task_info);
    this->accept();
}

void TranscodeDialog::on_buttonBox_rejected()
{
    this->reject();
}
