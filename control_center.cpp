#include "control_center.h"

#include <QFileInfo>
#include <QThread>

ControlCenter::ControlCenter(QString filepath, int taskID, TranscodeTask task_info, QObject *parent)
    : filepath(filepath), taskID(taskID), task_info(task_info), QObject(parent) {
}

ControlCenter::~ControlCenter()
{
    stop();

    if (demuxer_thread) {
        demuxer_thread->quit();
        demuxer_thread->wait();
    }
    if(video_decoder_thread){
        video_decoder_thread->quit();
        video_decoder_thread->wait();
    }
    if(audio_decoder_thread){
        audio_decoder_thread->quit();
        audio_decoder_thread->wait();
    }
    if(video_encode_thread){
        video_encode_thread->quit();
        video_encode_thread->wait();
    }
    if(audio_encode_thread){
        audio_encode_thread->quit();
        audio_encode_thread->wait();
    }
    if(muxer_thread){
        muxer_thread->quit();
        muxer_thread->wait();
    }

    if(fmt_ctx)
    {
        avformat_close_input(&fmt_ctx);
        fmt_ctx = nullptr;
    }
    if(hw_device_ctx)
    {
        av_buffer_unref(&hw_device_ctx);
        hw_device_ctx = nullptr;
    }
}
void ControlCenter::reportError(const QString &title, const QString &detail)
{
    qWarning() << "[转码错误] 任务" << taskID << ":" << title << "\n" << detail;
    emit transcodeError(taskID, title, detail);
    emit metadataError(taskID);
}

bool ControlCenter::init(){
    int ret = 0;
    ret = avformat_open_input(&fmt_ctx,filepath.toUtf8(),NULL,NULL);
    if(ret < 0)
    {
        char errbuf[128] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        reportError("无法打开输入媒体文件",
                    QString("FFmpeg 无法解封装源文件: %1\n底层错误: %2 (错误码: %3)\n建议排查: 检查文件是否损坏、是否正被其他软件占用，或路径中是否包含特殊非法字符。")
                    .arg(filepath).arg(errbuf).arg(ret));
        avformat_close_input(&fmt_ctx);
        return false;
    }
    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if(ret<0) {
        char errbuf[128] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        reportError("无法读取媒体流信息",
                    QString("解析媒体流头部失败: %1 (错误码: %2)\n建议排查: 媒体文件可能被截断或存在格式损坏。")
                    .arg(errbuf).arg(ret));
        return false;
    }

    int ret_hw = av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0);
    if (ret_hw < 0) {
        qDebug() << "control_center: 创建 CUDA 硬件设备上下文失败，将无法使用 GPU 硬解硬编";
        hw_device_ctx = nullptr;
    } else {
        qDebug() << "control_center: CUDA 硬件设备上下文初始化成功！";
    }

    unpack_video_pkt.set_size(200);
    unpack_audio_pkt.set_size(400);
    demuxer = std::make_unique<Demuxer>(unpack_video_pkt, unpack_audio_pkt);
    demuxer_thread = new QThread(this);
    demuxer->moveToThread(demuxer_thread);
    connect(this,&ControlCenter::start_demuxer,demuxer.get(),&Demuxer::start);
    connect(this,&ControlCenter::stop_demuxer,demuxer.get(),&Demuxer::stop);
    connect(this,&ControlCenter::pause_demuxer,demuxer.get(),&Demuxer::pause);
    demuxer_thread->start();

    decode_video_frame.set_size(16);
    video_decoder = std::make_unique<Decoder>(unpack_video_pkt,decode_video_frame);
    video_decoder_thread = new QThread(this);
    video_decoder->moveToThread(video_decoder_thread);
    connect(this,&ControlCenter::start_video_decoder,video_decoder.get(),&Decoder::start);
    connect(this,&ControlCenter::stop_decoder,video_decoder.get(),&Decoder::stop);
    connect(this,&ControlCenter::pause_decoder,video_decoder.get(),&Decoder::pause);
    video_decoder_thread->start();

    decode_audio_frame.set_size(75);
    audio_decoder = std::make_unique<Decoder>(unpack_audio_pkt,decode_audio_frame);
    audio_decoder_thread = new QThread(this);
    audio_decoder->moveToThread(audio_decoder_thread);
    connect(this,&ControlCenter::start_audio_decoder,audio_decoder.get(),&Decoder::start);
    connect(this,&ControlCenter::stop_decoder,audio_decoder.get(),&Decoder::stop);
    connect(this,&ControlCenter::pause_decoder,audio_decoder.get(),&Decoder::pause);
    audio_decoder_thread->start();

    pack_pkt.set_size(400);
    video_encoder = std::make_unique<VideoEncoder>(task_info,decode_video_frame,pack_pkt);
    video_encode_thread = new QThread(this);
    video_encoder->moveToThread(video_encode_thread);
    connect(this,&ControlCenter::start_video_encoder,video_encoder.get(),&VideoEncoder::start);
    connect(this,&ControlCenter::stop_encoder,video_encoder.get(),&VideoEncoder::stop);
    connect(this,&ControlCenter::pause_encoder,video_encoder.get(),&VideoEncoder::pause);
    video_encode_thread->start();

    audio_encoder = std::make_unique<AudioEncoder>(task_info,decode_audio_frame,pack_pkt);
    audio_encode_thread = new QThread(this);
    audio_encoder->moveToThread(audio_encode_thread);
    connect(this,&ControlCenter::start_audio_encoder,audio_encoder.get(),&AudioEncoder::start);
    connect(this,&ControlCenter::stop_encoder,audio_encoder.get(),&AudioEncoder::stop);
    connect(this,&ControlCenter::pause_encoder,audio_encoder.get(),&AudioEncoder::pause);
    audio_encode_thread->start();

    muxer = std::make_unique<Muxer>(pack_pkt);
    muxer_thread = new QThread(this);
    muxer->moveToThread(muxer_thread);
    connect(this,&ControlCenter::start_muxer,muxer.get(),&Muxer::start);
    connect(this,&ControlCenter::stop_muxer,muxer.get(),&Muxer::stop);
    connect(this,&ControlCenter::pause_muxer,muxer.get(),&Muxer::pause);
    connect(muxer.get(),&Muxer::finished,this,&ControlCenter::finished);
    connect(muxer.get(),&Muxer::progressUpdated,this,&ControlCenter::progress_updata);
    muxer_thread->start();

    return true;
}
void ControlCenter::open(){
    if(!init())
    {
        return;
    }
    this->print_metadata();
    if(!demuxer->init(fmt_ctx))
    {
        reportError("媒体流解析失败", "输入媒体文件中未检测到任何有效的视频或音频流，无法进行转码。");
        return;
    }

    if(demuxer->get_video_stream_id() < 0 || !video_decoder->init(fmt_ctx,demuxer->get_video_stream_id(),this->hw_device_ctx))
    {
        reportError("视频解码器启动失败",
                    QString("未能成功初始化视频解码管线 (源编码格式: %1)。\n建议排查: 显卡硬解与软解均未识别此特殊编码，请确认文件是否完整。")
                    .arg(info.videoCodec.isEmpty() ? "未知格式" : info.videoCodec));
        return;
    }

    if(demuxer->get_audio_stream_id() >= 0 && !audio_decoder->init(fmt_ctx,demuxer->get_audio_stream_id()))
    {
        qDebug()<<"audio_decoder启动失败，将仅转码视频流";
    }
    emit metadataReady(taskID,info);
}

void ControlCenter::start(){
    //ffmpeg_start
    qDebug() << "ControlCenter::start 被成功调用!";
    is_cancelled = false;

    if(!video_encoder->init(video_stream_id,info.in_pix_fmt,info.width,info.height,info.time_base,info.frameRate,info.sar,hw_device_ctx))
    {
        reportError("视频编码器初始化失败",
                    QString("无法启动目标视频编码器: %1\n目标尺寸: %2x%3, 帧率: %4 fps\n建议排查:\n1. NVENC 硬件编码器可能不支持奇数分辨率或超大画幅，请尝试在配置中指定 1080P/720P\n2. 若显卡驱动未就绪，请尝试切换为 CPU 软件编码器。")
                    .arg(task_info.videoCodec)
                    .arg(task_info.width > 0 ? QString::number(task_info.width) : QString::number(info.width))
                    .arg(task_info.height > 0 ? QString::number(task_info.height) : QString::number(info.height))
                    .arg(task_info.frameRate > 0 ? QString::number(task_info.frameRate) : "源帧率"));
        return;
    }
    if(audio_stream_id >= 0 && !audio_encoder->init(audio_stream_id,info.in_sample_fmt,info.audioSampleRate,info.in_ch_layout,info.audio_time_base))
    {
        reportError("音频编码器初始化失败",
                    QString("无法启动目标音频编码器: %1 (采样率: %2 Hz, 声道数: %3)\n建议排查: 采样率或声道数与选定编码格式不匹配，建议切换为通用 AAC 编码。")
                    .arg(task_info.audioCodec)
                    .arg(info.audioSampleRate)
                    .arg(info.audioChannels));
        return;
    }
    AVCodecContext *a_ctx = (audio_stream_id >= 0) ? audio_encoder->get_codec_ctx() : nullptr;
    if(!muxer->init(task_info.outputPath,video_stream_id,audio_stream_id,video_encoder->get_codec_ctx(),a_ctx,duration))
    {
        reportError("输出文件创建或封装器初始化失败",
                    QString("无法创建并写入目标文件: %1\n建议排查:\n1. 检查目标磁盘是否有写权限或存储空间已满\n2. 目标文件是否被播放器等程序独占锁定\n3. 目标封装格式 (如 .%2) 与所选流参数冲突。")
                    .arg(task_info.outputPath)
                    .arg(task_info.outputFormat));
        return;
    }

    emit start_demuxer();
    emit start_video_decoder();
    emit start_video_encoder();

    if (audio_stream_id >= 0) {
        emit start_audio_decoder();
        emit start_audio_encoder();
    }

    emit start_muxer();
}

void ControlCenter::stop(){

    if (demuxer) demuxer->stop();
    if (video_decoder) video_decoder->stop();
    if (audio_decoder) audio_decoder->stop();
    if (video_encoder) video_encoder->stop();
    if (audio_encoder) audio_encoder->stop();
    if (muxer) muxer->stop();

    unpack_video_pkt.abort();
    unpack_audio_pkt.abort();
    decode_video_frame.abort();
    decode_audio_frame.abort();
    pack_pkt.abort();

}


void ControlCenter::print_metadata(){
    AVDictionaryEntry *tag = NULL;
    while((tag = av_dict_get( fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX )))
    {
        qDebug()<<tag->key<<"="<<tag->value;
    }
    QFileInfo fileInfo(filepath);
    info.inputPath = filepath;
    info.inputFormat = fileInfo.suffix().
                       toUpper();
    qDebug()<<"编码格式:"<<info.inputFormat;
    qDebug()<<"时长:"<<fmt_ctx->duration/AV_TIME_BASE<<"秒";
    duration = fmt_ctx->duration/AV_TIME_BASE;
    info.videoBitrate = fmt_ctx->bit_rate/1000;
    qDebug()<<"码率:"<<info.videoBitrate<<"kbps";
    qDebug()<<"流个数:"<<fmt_ctx->nb_streams;

    int best_v = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    int best_a = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    if (best_v >= 0) {
        video_stream_id = best_v;
        AVStream *stream = fmt_ctx->streams[best_v];
        AVCodecParameters *codec_par = stream->codecpar;

        info.in_pix_fmt = static_cast<AVPixelFormat>(codec_par->format);
        info.time_base = stream->time_base;

        AVRational frameRateRatio = stream->avg_frame_rate;
        if (frameRateRatio.num <= 0 || frameRateRatio.den <= 0) {
            frameRateRatio = stream->r_frame_rate;
        }
        if (frameRateRatio.num <= 0 || frameRateRatio.den <= 0) {
            frameRateRatio = AVRational{30, 1};
        }
        info.frameRate = frameRateRatio;

        AVRational sar = stream->sample_aspect_ratio;
        if (sar.num <= 0 || sar.den <= 0) {
            sar = codec_par->sample_aspect_ratio;
        }
        if (sar.num <= 0 || sar.den <= 0) {
            sar = AVRational{1, 1};
        }
        info.sar = sar;

        info.videoCodec = avcodec_get_name(codec_par->codec_id);
        info.width = codec_par->width;
        info.height = codec_par->height;

        qDebug() << "视频编码格式:" << info.videoCodec
                 << " 帧率:" << av_q2d(info.frameRate)
                 << " 分辨率:" << info.width << "x" << info.height;
    }

    if (best_a >= 0) {
        audio_stream_id = best_a;
        AVStream *stream = fmt_ctx->streams[best_a];
        AVCodecParameters *codec_par = stream->codecpar;

        info.in_sample_fmt = static_cast<AVSampleFormat>(codec_par->format);
        info.audio_time_base = stream->time_base;
        av_channel_layout_copy(&info.in_ch_layout, &codec_par->ch_layout);

        info.audioCodec = avcodec_get_name(codec_par->codec_id);
        info.audioChannels = codec_par->ch_layout.nb_channels;
        info.audioSampleRate = codec_par->sample_rate;
        info.audioBitrate = codec_par->bit_rate;

        qDebug() << "音频编码格式:" << info.audioCodec
                 << " 通道数:" << info.audioChannels
                 << " 采样率:" << info.audioSampleRate;
    }
}

void ControlCenter::cancel()
{
    is_cancelled = true;
    stop();
}
