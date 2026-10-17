#include "batch_transcode_dialog.h"
#include "window_style_helper.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>

BatchTranscodeDialog::BatchTranscodeDialog(int waitingCount, int totalCount, QWidget *parent)
    : QDialog(parent)
{
    setupUi(waitingCount, totalCount);
    WindowStyleHelper::applyDarkWindowFrame(this);

    // 默认应用第一个推荐预设：H.264 硬件加速 1080P
    applyQuickPreset(0);
}

void BatchTranscodeDialog::setupUi(int waitingCount, int totalCount)
{
    this->setWindowTitle("批量转码参数配置中心");
    this->resize(680, 620);
    this->setMinimumSize(600, 560);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 20, 24, 20);

    // 1. 顶部 Header 提示栏
    QWidget *headerWidget = new QWidget(this);
    QVBoxLayout *headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(4);

    QLabel *titleLabel = new QLabel("⚡ 批量转码参数配置预设", headerWidget);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: 700; color: #38bdf8;");

    QLabel *descLabel = new QLabel(
        QString("当前队列共有 %1 项任务（其中 %2 项待配置/就绪）。一次性统一设置编码格式与输出目标，告别重复弹窗。")
            .arg(totalCount).arg(waitingCount),
        headerWidget);
    descLabel->setStyleSheet("font-size: 12px; color: #94a3b8;");
    descLabel->setWordWrap(true);

    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(descLabel);
    mainLayout->addWidget(headerWidget);

    // 2. 预设快捷方案 GroupBox
    QGroupBox *presetGroup = new QGroupBox("一键应用专业方案 (Quick Presets)", this);
    QHBoxLayout *presetLayout = new QHBoxLayout(presetGroup);
    presetLayout->setContentsMargins(14, 14, 14, 14);

    m_presetCombo = new QComboBox(presetGroup);
    m_presetCombo->addItem("⚡ [默认推荐] 高速通用压制 (MP4 · H.264 硬件加速 · AAC · 1080P)");
    m_presetCombo->addItem("⚡ 蓝光高清高压缩 (MP4 · H.265 硬件加速 · AAC · 保持原分辨率)");
    m_presetCombo->addItem("🎬 影视剪辑母带 (MOV · Apple ProRes 422 ⏱ · 原画画质)");
    m_presetCombo->addItem("🌐 网络流媒体免专利 (WebM · VP9 ⏱ · 720P · Opus)");
    m_presetCombo->addItem("📻 广播切片标准 (TS · H.264 ⚡ · 1080P · MP3)");
    m_presetCombo->addItem("🎵 高品质音频提取 (MP3 · 320 kbps 立体声)");
    m_presetCombo->addItem("🎵 Hi-Res 无损音频 (FLAC · 原始解析度)");
    m_presetCombo->addItem("🛠 自定义高级参数");
    m_presetCombo->setMinimumHeight(32);

    presetLayout->addWidget(m_presetCombo);
    mainLayout->addWidget(presetGroup);

    // 3. 详细参数设置 GroupBox
    QGroupBox *paramGroup = new QGroupBox("目标编码器与画质参数", this);
    QGridLayout *paramGrid = new QGridLayout(paramGroup);
    paramGrid->setContentsMargins(14, 14, 14, 14);
    paramGrid->setHorizontalSpacing(14);
    paramGrid->setVerticalSpacing(10);

    // 封装格式
    QLabel *lblFormat = new QLabel("封装容器格式:", paramGroup);
    m_formatCombo = new QComboBox(paramGroup);
    m_formatCombo->addItem("MP4 格式 (*.mp4) - 全球通用流行封装", "mp4");
    m_formatCombo->addItem("MKV 格式 (*.mkv) - 全能蓝光/多音轨封装", "mkv");
    m_formatCombo->addItem("MOV 格式 (*.mov) - Apple 专业影视级", "mov");
    m_formatCombo->addItem("WebM 格式 (*.webm) - 网页无专利高压缩", "webm");
    m_formatCombo->addItem("TS 格式 (*.ts) - 广播传输与切片流", "ts");
    m_formatCombo->addItem("FLV 格式 (*.flv) - 网络直播推流标准", "flv");
    m_formatCombo->addItem("AVI 格式 (*.avi) - 经典多媒体容器", "avi");
    m_formatCombo->addItem("MP3 音频 (*.mp3) - 独立音频文件", "mp3");
    m_formatCombo->addItem("FLAC 音频 (*.flac) - Hi-Res 无损音频", "flac");
    m_formatCombo->addItem("WAV 音频 (*.wav) - 原始无损未压缩", "wav");
    m_formatCombo->addItem("AAC 音频 (*.aac) - 高保真音频流", "aac");
    m_formatCombo->setMinimumHeight(30);

    // 视频编码器
    QLabel *lblVideoCodec = new QLabel("视频编码格式:", paramGroup);
    m_videoCodecCombo = new QComboBox(paramGroup);
    m_videoCodecCombo->setMinimumHeight(30);

    // 音频编码器
    QLabel *lblAudioCodec = new QLabel("音频编码格式:", paramGroup);
    m_audioCodecCombo = new QComboBox(paramGroup);
    m_audioCodecCombo->setMinimumHeight(30);

    // 目标分辨率
    QLabel *lblRes = new QLabel("目标视频分辨率:", paramGroup);
    m_resolutionCombo = new QComboBox(paramGroup);
    m_resolutionCombo->addItem("保持原始分辨率 (推荐)", QSize(-1, -1));
    m_resolutionCombo->addItem("3840 × 2160 (4K UHD 超清)", QSize(3840, 2160));
    m_resolutionCombo->addItem("2560 × 1440 (2K QHD 极清)", QSize(2560, 1440));
    m_resolutionCombo->addItem("1920 × 1080 (1080P 全高清)", QSize(1920, 1080));
    m_resolutionCombo->addItem("1280 × 720 (720P 高清标配)", QSize(1280, 720));
    m_resolutionCombo->addItem("854 × 480 (480P 标清省流)", QSize(854, 480));
    m_resolutionCombo->setMinimumHeight(30);

    // 目标帧率
    QLabel *lblFps = new QLabel("目标输出帧率:", paramGroup);
    m_framerateCombo = new QComboBox(paramGroup);
    m_framerateCombo->addItem("保持源帧率 (推荐)", -1.0);
    m_framerateCombo->addItem("60 fps (流畅高帧率)", 60.0);
    m_framerateCombo->addItem("30 fps (网络主流标准)", 30.0);
    m_framerateCombo->addItem("24 fps (电影胶片质感)", 24.0);
    m_framerateCombo->addItem("25 fps (广电电视标准)", 25.0);
    m_framerateCombo->setMinimumHeight(30);

    // 视频码率
    QLabel *lblBitrate = new QLabel("视频码率控制:", paramGroup);
    m_videoBitrateCombo = new QComboBox(paramGroup);
    m_videoBitrateCombo->addItem("自适应智能码率 (推荐)", -1);
    m_videoBitrateCombo->addItem("2000 kbps (1080P 网络轻量)", 2000 * 1000);
    m_videoBitrateCombo->addItem("4000 kbps (1080P 推荐画质)", 4000 * 1000);
    m_videoBitrateCombo->addItem("8000 kbps (1080P 高码率蓝光)", 8000 * 1000);
    m_videoBitrateCombo->addItem("15000 kbps (2K 极清超清)", 15000 * 1000);
    m_videoBitrateCombo->addItem("25000 kbps (4K UHD 超清原盘)", 25000 * 1000);
    m_videoBitrateCombo->setMinimumHeight(30);

    paramGrid->addWidget(lblFormat, 0, 0);
    paramGrid->addWidget(m_formatCombo, 0, 1);
    paramGrid->addWidget(lblVideoCodec, 0, 2);
    paramGrid->addWidget(m_videoCodecCombo, 0, 3);

    paramGrid->addWidget(lblRes, 1, 0);
    paramGrid->addWidget(m_resolutionCombo, 1, 1);
    paramGrid->addWidget(lblAudioCodec, 1, 2);
    paramGrid->addWidget(m_audioCodecCombo, 1, 3);

    paramGrid->addWidget(lblFps, 2, 0);
    paramGrid->addWidget(m_framerateCombo, 2, 1);
    paramGrid->addWidget(lblBitrate, 2, 2);
    paramGrid->addWidget(m_videoBitrateCombo, 2, 3);

    mainLayout->addWidget(paramGroup);

    // 4. 输出目录与命名规则 GroupBox
    QGroupBox *outputGroup = new QGroupBox("输出目录与命名规则", this);
    QVBoxLayout *outputLayout = new QVBoxLayout(outputGroup);
    outputLayout->setContentsMargins(14, 14, 14, 14);
    outputLayout->setSpacing(10);

    m_radioSourceDir = new QRadioButton("保存在源文件同级目录下 (推荐)", outputGroup);
    m_radioSourceDir->setChecked(true);
    outputLayout->addWidget(m_radioSourceDir);

    QHBoxLayout *customDirLayout = new QHBoxLayout();
    m_radioCustomDir = new QRadioButton("统一保存至指定文件夹:", outputGroup);
    m_editCustomDir = new QLineEdit(outputGroup);
    m_editCustomDir->setPlaceholderText("请选择统一输出目录...");
    m_editCustomDir->setEnabled(false);
    m_editCustomDir->setMinimumHeight(28);

    m_btnBrowseDir = new QPushButton("浏览...", outputGroup);
    m_btnBrowseDir->setEnabled(false);
    m_btnBrowseDir->setMinimumHeight(28);

    customDirLayout->addWidget(m_radioCustomDir);
    customDirLayout->addWidget(m_editCustomDir, 1);
    customDirLayout->addWidget(m_btnBrowseDir);
    outputLayout->addLayout(customDirLayout);

    connect(m_radioCustomDir, &QRadioButton::toggled, this, [this](bool checked) {
        m_editCustomDir->setEnabled(checked);
        m_btnBrowseDir->setEnabled(checked);
    });

    QHBoxLayout *suffixLayout = new QHBoxLayout();
    QLabel *lblSuffix = new QLabel("输出文件后缀追加 (避免覆盖):", outputGroup);
    m_editSuffix = new QLineEdit("_transcoded", outputGroup);
    m_editSuffix->setPlaceholderText("例如: _transcoded");
    m_editSuffix->setMaximumWidth(160);
    m_editSuffix->setMinimumHeight(28);
    suffixLayout->addWidget(lblSuffix);
    suffixLayout->addWidget(m_editSuffix);
    suffixLayout->addStretch();
    outputLayout->addLayout(suffixLayout);

    mainLayout->addWidget(outputGroup);

    // 5. 应用范围 GroupBox
    QGroupBox *scopeGroup = new QGroupBox("预设应用范围", this);
    QHBoxLayout *scopeLayout = new QHBoxLayout(scopeGroup);
    scopeLayout->setContentsMargins(14, 10, 14, 10);
    m_radioOnlyUnconfigured = new QRadioButton("仅应用到未配置/就绪的任务 (推荐)", scopeGroup);
    m_radioOnlyUnconfigured->setChecked(true);
    m_radioAllNonRunning = new QRadioButton("强制覆盖列表中所有非运行中的任务", scopeGroup);
    scopeLayout->addWidget(m_radioOnlyUnconfigured);
    scopeLayout->addWidget(m_radioAllNonRunning);
    mainLayout->addWidget(scopeGroup);

    // 6. 底部操作按钮栏
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);

    QPushButton *btnCancel = new QPushButton("取消", this);
    btnCancel->setMinimumHeight(36);
    btnCancel->setMinimumWidth(80);

    QPushButton *btnApplyOnly = new QPushButton("✓ 仅批量应用预设", this);
    btnApplyOnly->setMinimumHeight(36);
    btnApplyOnly->setMinimumWidth(140);

    QPushButton *btnApplyAndStart = new QPushButton("🚀 批量应用并立即全部开始", this);
    btnApplyAndStart->setProperty("primary", true);
    btnApplyAndStart->setMinimumHeight(36);
    btnApplyAndStart->setMinimumWidth(180);

    btnLayout->addStretch();
    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnApplyOnly);
    btnLayout->addWidget(btnApplyAndStart);
    mainLayout->addLayout(btnLayout);

    // 连接事件
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BatchTranscodeDialog::onPresetComboChanged);
    connect(m_formatCombo, &QComboBox::currentTextChanged, this, &BatchTranscodeDialog::onFormatChanged);
    connect(m_btnBrowseDir, &QPushButton::clicked, this, &BatchTranscodeDialog::onBrowseOutputDir);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnApplyOnly, &QPushButton::clicked, this, &BatchTranscodeDialog::onApplyOnlyClicked);
    connect(btnApplyAndStart, &QPushButton::clicked, this, &BatchTranscodeDialog::onApplyAndStartClicked);

    // 触发初始编码器联动
    updateCodecCombos(m_formatCombo->currentText());
}

void BatchTranscodeDialog::onBrowseOutputDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择转码统一输出目录", m_editCustomDir->text());
    if (!dir.isEmpty()) {
        m_editCustomDir->setText(dir);
    }
}

void BatchTranscodeDialog::onFormatChanged(const QString &formatText)
{
    updateCodecCombos(formatText);
}

void BatchTranscodeDialog::updateCodecCombos(const QString &formatText)
{
    m_videoCodecCombo->clear();
    m_audioCodecCombo->clear();

    if (formatText.contains("MP4")) {
        m_videoCodecCombo->addItem("H.264 / AVC ⚡ [GPU 硬件加速 · 极速]", "libx264");
        m_videoCodecCombo->addItem("H.265 / HEVC ⚡ [GPU 硬件加速 · 高压缩率]", "libx265");
        m_videoCodecCombo->addItem("AV1 ⏱ [CPU 软编 · 下一代免专利]", "libsvtav1");
        m_videoCodecCombo->addItem("MPEG-4 Part 2 ⏱ [CPU 软编 · 经典兼容]", "mpeg4");

        m_audioCodecCombo->addItem("AAC (通用流媒体标准)", "aac");
        m_audioCodecCombo->addItem("MP3 (广泛跨平台兼容)", "libmp3lame");
        m_audioCodecCombo->addItem("AC-3 (Dolby Digital 杜比立体声/5.1)", "ac3");
    }
    else if (formatText.contains("MKV")) {
        m_videoCodecCombo->addItem("H.264 / AVC ⚡ [GPU 硬件加速 · 极速]", "libx264");
        m_videoCodecCombo->addItem("H.265 / HEVC ⚡ [GPU 硬件加速 · 蓝光原盘]", "libx265");
        m_videoCodecCombo->addItem("Apple ProRes 422 ⏱ [CPU 软编 · 影视母带]", "prores_ks");
        m_videoCodecCombo->addItem("VP9 ⏱ [CPU 软编 · Google 高清]", "libvpx-vp9");
        m_videoCodecCombo->addItem("AV1 ⏱ [CPU 软编 · 下一代免专利]", "libsvtav1");
        m_videoCodecCombo->addItem("MPEG-2 Video ⏱ [CPU 软编 · 传统广播]", "mpeg2video");

        m_audioCodecCombo->addItem("AAC (通用高保真)", "aac");
        m_audioCodecCombo->addItem("Opus (现代顶级音质 / 低延迟)", "libopus");
        m_audioCodecCombo->addItem("FLAC (Hi-Res 无损母带)", "flac");
        m_audioCodecCombo->addItem("AC-3 (Dolby Digital 杜比数字)", "ac3");
        m_audioCodecCombo->addItem("PCM (16-bit 无损)", "pcm_s16le");
        m_audioCodecCombo->addItem("MP3 (广泛兼容)", "libmp3lame");
    }
    else if (formatText.contains("MOV")) {
        m_videoCodecCombo->addItem("H.264 / AVC ⚡ [GPU 硬件加速 · 极速]", "libx264");
        m_videoCodecCombo->addItem("H.265 / HEVC ⚡ [GPU 硬件加速 · 极速]", "libx265");
        m_videoCodecCombo->addItem("Apple ProRes 422 ⏱ [CPU 软编 · 影视剪辑直出]", "prores_ks");
        m_videoCodecCombo->addItem("MJPEG ⏱ [CPU 软编 · 逐帧采集]", "mjpeg");

        m_audioCodecCombo->addItem("AAC (通用高质量)", "aac");
        m_audioCodecCombo->addItem("PCM (16-bit CD 级无损)", "pcm_s16le");
        m_audioCodecCombo->addItem("PCM (24-bit 录音室母带)", "pcm_s24le");
        m_audioCodecCombo->addItem("ALAC (Apple Lossless 苹果无损)", "alac");
    }
    else if (formatText.contains("WebM")) {
        m_videoCodecCombo->addItem("VP9 ⏱ [CPU 软编 · YouTube 标配]", "libvpx-vp9");
        m_videoCodecCombo->addItem("AV1 ⏱ [CPU 软编 · 下一代免专利]", "libsvtav1");
        m_videoCodecCombo->addItem("VP8 ⏱ [CPU 软编 · 传统网页兼容]", "libvpx");

        m_audioCodecCombo->addItem("Opus (现代低延迟高品质标配)", "libopus");
        m_audioCodecCombo->addItem("Vorbis (经典开源音频)", "libvorbis");
    }
    else if (formatText.contains("TS")) {
        m_videoCodecCombo->addItem("H.264 / AVC ⚡ [GPU 硬件加速 · 广播与直播]", "libx264");
        m_videoCodecCombo->addItem("H.265 / HEVC ⚡ [GPU 硬件加速 · 4K广播切片]", "libx265");
        m_videoCodecCombo->addItem("MPEG-2 Video ⏱ [CPU 软编 · 经典广电标准]", "mpeg2video");

        m_audioCodecCombo->addItem("AAC (广播级流媒体推荐)", "aac");
        m_audioCodecCombo->addItem("AC-3 (杜比环绕声)", "ac3");
        m_audioCodecCombo->addItem("MP3 (通用音频)", "libmp3lame");
    }
    else if (formatText.contains("FLV")) {
        m_videoCodecCombo->addItem("H.264 / AVC ⚡ [GPU 硬件加速 · RTMP 黄金标准]", "libx264");

        m_audioCodecCombo->addItem("AAC (直播通用推流)", "aac");
        m_audioCodecCombo->addItem("MP3 (经典 Flash 流媒体)", "libmp3lame");
    }
    else if (formatText.contains("AVI")) {
        m_videoCodecCombo->addItem("H.264 / AVC ⚡ [GPU 硬件加速 · 现代兼容]", "libx264");
        m_videoCodecCombo->addItem("MPEG-4 Part 2 ⏱ [CPU 软编 · DivX/Xvid 兼容]", "mpeg4");
        m_videoCodecCombo->addItem("MJPEG ⏱ [CPU 软编 · 逐帧高质量编辑]", "mjpeg");

        m_audioCodecCombo->addItem("MP3 (经典通用音频)", "libmp3lame");
        m_audioCodecCombo->addItem("PCM (16-bit 原始波形)", "pcm_s16le");
    }
    else {
        // 纯音频格式
        m_videoCodecCombo->addItem("无视频流 (仅保留音频)", "");
        if (formatText.contains("MP3")) {
            m_audioCodecCombo->addItem("MP3 (libmp3lame 320k)", "libmp3lame");
        } else if (formatText.contains("FLAC")) {
            m_audioCodecCombo->addItem("FLAC (Hi-Res 无损母带)", "flac");
        } else if (formatText.contains("WAV")) {
            m_audioCodecCombo->addItem("PCM 16-bit (未压缩)", "pcm_s16le");
        } else if (formatText.contains("AAC")) {
            m_audioCodecCombo->addItem("AAC (通用高保真)", "aac");
        }
    }
}

void BatchTranscodeDialog::onPresetComboChanged(int index)
{
    if (index == 7) {
        // 自定义模式，不修改当前用户调节
        return;
    }
    applyQuickPreset(index);
}

void BatchTranscodeDialog::applyQuickPreset(int presetIndex)
{
    QSignalBlocker blockerFormat(m_formatCombo);

    switch (presetIndex) {
    case 0: // ⚡ 高速通用压制 (MP4 · H.264 ⚡ · 1080P · AAC)
        m_formatCombo->setCurrentIndex(0); // MP4
        updateCodecCombos(m_formatCombo->currentText());
        m_videoCodecCombo->setCurrentIndex(0); // H.264
        m_audioCodecCombo->setCurrentIndex(0); // AAC
        m_resolutionCombo->setCurrentIndex(3); // 1080P
        m_framerateCombo->setCurrentIndex(0); // 保持源帧率
        m_videoBitrateCombo->setCurrentIndex(2); // 4000 kbps
        break;
    case 1: // ⚡ 蓝光高清高压缩 (MP4 · H.265 ⚡ · 原分辨率 · AAC)
        m_formatCombo->setCurrentIndex(0); // MP4
        updateCodecCombos(m_formatCombo->currentText());
        m_videoCodecCombo->setCurrentIndex(1); // H.265
        m_audioCodecCombo->setCurrentIndex(0); // AAC
        m_resolutionCombo->setCurrentIndex(0); // 保持原始分辨率
        m_framerateCombo->setCurrentIndex(0);
        m_videoBitrateCombo->setCurrentIndex(0); // 自适应
        break;
    case 2: // 🎬 影视后期母带 (MOV · ProRes 422 · 原画质)
        m_formatCombo->setCurrentIndex(2); // MOV
        updateCodecCombos(m_formatCombo->currentText());
        m_videoCodecCombo->setCurrentIndex(2); // ProRes
        m_audioCodecCombo->setCurrentIndex(1); // PCM 16bit
        m_resolutionCombo->setCurrentIndex(0); // 原分辨率
        m_framerateCombo->setCurrentIndex(0);
        m_videoBitrateCombo->setCurrentIndex(0);
        break;
    case 3: // 🌐 WebM (WebM · VP9 · 720P)
        m_formatCombo->setCurrentIndex(3); // WebM
        updateCodecCombos(m_formatCombo->currentText());
        m_videoCodecCombo->setCurrentIndex(0); // VP9
        m_audioCodecCombo->setCurrentIndex(0); // Opus
        m_resolutionCombo->setCurrentIndex(4); // 720P
        m_framerateCombo->setCurrentIndex(0);
        m_videoBitrateCombo->setCurrentIndex(1); // 2000 kbps
        break;
    case 4: // 📻 TS 广播切片
        m_formatCombo->setCurrentIndex(4); // TS
        updateCodecCombos(m_formatCombo->currentText());
        m_videoCodecCombo->setCurrentIndex(0); // H.264
        m_audioCodecCombo->setCurrentIndex(0); // AAC
        m_resolutionCombo->setCurrentIndex(3); // 1080P
        m_framerateCombo->setCurrentIndex(0);
        m_videoBitrateCombo->setCurrentIndex(2); // 4000 kbps
        break;
    case 5: // 🎵 MP3 音频提取
        m_formatCombo->setCurrentIndex(7); // MP3
        updateCodecCombos(m_formatCombo->currentText());
        m_resolutionCombo->setCurrentIndex(0);
        m_framerateCombo->setCurrentIndex(0);
        m_videoBitrateCombo->setCurrentIndex(0);
        break;
    case 6: // 🎵 FLAC 无损
        m_formatCombo->setCurrentIndex(8); // FLAC
        updateCodecCombos(m_formatCombo->currentText());
        m_resolutionCombo->setCurrentIndex(0);
        m_framerateCombo->setCurrentIndex(0);
        m_videoBitrateCombo->setCurrentIndex(0);
        break;
    default:
        break;
    }
}

void BatchTranscodeDialog::onApplyOnlyClicked()
{
    m_startImmediately = false;

    // 校验自定义输出路径
    if (m_radioCustomDir->isChecked()) {
        QString dir = m_editCustomDir->text().trimmed();
        if (dir.isEmpty() || !QDir(dir).exists()) {
            QMessageBox::warning(this, "输出目录无效", "请指定真实有效的统一输出目标文件夹！");
            return;
        }
    }

    m_preset.outputFormat = m_formatCombo->currentData().toString();
    m_preset.videoCodec = m_videoCodecCombo->currentData().toString();
    m_preset.audioCodec = m_audioCodecCombo->currentData().toString();

    QSize res = m_resolutionCombo->currentData().toSize();
    m_preset.width = (res.isValid() && res.width() > 0) ? res.width() : -1;
    m_preset.height = (res.isValid() && res.height() > 0) ? res.height() : -1;

    m_preset.frameRate = m_framerateCombo->currentData().toDouble();
    m_preset.videoBitrate = m_videoBitrateCombo->currentData().toInt();

    m_preset.useSourceDir = m_radioSourceDir->isChecked();
    m_preset.customOutputDir = m_editCustomDir->text().trimmed();
    m_preset.fileSuffix = m_editSuffix->text().trimmed();
    if (m_preset.fileSuffix.isEmpty()) {
        m_preset.fileSuffix = "_transcoded";
    }

    m_preset.onlyUnconfigured = m_radioOnlyUnconfigured->isChecked();
    m_preset.startImmediately = false;

    this->accept();
}

void BatchTranscodeDialog::onApplyAndStartClicked()
{
    onApplyOnlyClicked();
    if (this->result() == QDialog::Accepted) {
        m_startImmediately = true;
        m_preset.startImmediately = true;
    }
}
