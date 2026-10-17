#include "control_center.h"

#include <QFileInfo>
#include <QThread>

control_center::control_center( QString filepath,int taskID,TranscodeTask task_info, QObject *parent)
    :filepath(filepath),taskID(taskID),task_info(task_info),QObject(parent) {
}

control_center::~control_center()
{
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
bool control_center::init(){
    int ret = 0;
    ret = avformat_open_input(&fmt_ctx,filepath.toUtf8(),NULL,NULL);
    if(ret < 0)
    {
        avformat_close_input(&fmt_ctx);
        return false;
    }
    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if(ret<0) return false;

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
    connect(this,&control_center::start_demuxer,demuxer.get(),&Demuxer::start);
    connect(this,&control_center::stop_demuxer,demuxer.get(),&Demuxer::stop);
    connect(this,&control_center::pause_demuxer,demuxer.get(),&Demuxer::pause);
    demuxer_thread->start();

    decode_video_frame.set_size(16);
    video_decoder = std::make_unique<Decoder>(unpack_video_pkt,decode_video_frame);
    video_decoder_thread = new QThread(this);
    video_decoder->moveToThread(video_decoder_thread);
    connect(this,&control_center::start_decoder,video_decoder.get(),&Decoder::start);
    connect(this,&control_center::stop_decoder,video_decoder.get(),&Decoder::stop);
    connect(this,&control_center::pause_decoder,video_decoder.get(),&Decoder::pause);
    video_decoder_thread->start();

    decode_audio_frame.set_size(75);
    audio_decoder = std::make_unique<Decoder>(unpack_audio_pkt,decode_audio_frame);
    audio_decoder_thread = new QThread(this);
    audio_decoder->moveToThread(audio_decoder_thread);
    connect(this,&control_center::start_decoder,audio_decoder.get(),&Decoder::start);
    connect(this,&control_center::stop_decoder,audio_decoder.get(),&Decoder::stop);
    connect(this,&control_center::pause_decoder,audio_decoder.get(),&Decoder::pause);
    audio_decoder_thread->start();

    pack_pkt.set_size(400);
    video_encoder = std::make_unique<VideoEncoder>(task_info,decode_video_frame,pack_pkt);
    video_encode_thread = new QThread(this);
    video_encoder->moveToThread(video_encode_thread);
    connect(this,&control_center::start_encoder,video_encoder.get(),&VideoEncoder::start);
    connect(this,&control_center::stop_encoder,video_encoder.get(),&VideoEncoder::stop);
    connect(this,&control_center::pause_encoder,video_encoder.get(),&VideoEncoder::pause);
    video_encode_thread->start();

    audio_encoder = std::make_unique<AudioEncoder>(task_info,decode_audio_frame,pack_pkt);
    audio_encode_thread = new QThread(this);
    audio_encoder->moveToThread(audio_encode_thread);
    connect(this,&control_center::start_encoder,audio_encoder.get(),&AudioEncoder::start);
    connect(this,&control_center::stop_encoder,audio_encoder.get(),&AudioEncoder::stop);
    connect(this,&control_center::pause_encoder,audio_encoder.get(),&AudioEncoder::pause);
    audio_encode_thread->start();

    muxer = std::make_unique<Muxer>(pack_pkt);
    muxer_thread = new QThread(this);
    muxer->moveToThread(muxer_thread);
    connect(this,&control_center::start_muxer,muxer.get(),&Muxer::start);
    connect(this,&control_center::stop_muxer,muxer.get(),&Muxer::stop);
    connect(this,&control_center::pause_muxer,muxer.get(),&Muxer::pause);
    connect(muxer.get(),&Muxer::finished,this,&control_center::finished);
    connect(muxer.get(),&Muxer::progressUpdated,this,&control_center::progress_updata);
    muxer_thread->start();


    return true;
}
void control_center::open(){
    if(!init())
    {
        emit metadataError(taskID);
        return;
    }
    this->print_metadata();
    if(!demuxer->init(fmt_ctx))
    {
        qDebug()<<"Demuxer启动失败";
        emit metadataError(taskID);
        return;
    }

    if(demuxer->get_video_stream_id() < 0 || !video_decoder->init(fmt_ctx,demuxer->get_video_stream_id(),this->hw_device_ctx))
    {
        qDebug()<<"video_decoder启动失败或无视频流";
        emit metadataError(taskID);
        return;
    }

    if(demuxer->get_audio_stream_id() >= 0 && !audio_decoder->init(fmt_ctx,demuxer->get_audio_stream_id()))
    {
        qDebug()<<"audio_decoder启动失败";
    }
    emit metadataReady(taskID,info);
}

void control_center::start(){
    //ffmpeg_start
    qDebug() << "control_center::start 被成功调用!";
    is_cancelled = false;

    if(!video_encoder->init(video_stream_id,info.in_pix_fmt,info.width,info.height,info.time_base,info.frameRate,info.sar,hw_device_ctx))
    {
        qDebug()<<"video_encoder启动失败";
        return;
    }
    if(!audio_encoder->init(audio_stream_id,info.in_sample_fmt,info.audioSampleRate,info.in_ch_layout,info.audio_time_base))
    {
        qDebug()<<"audio_encoder启动失败";
        return;
    }
    if(!muxer->init(task_info.outputPath,video_stream_id,audio_stream_id,video_encoder->get_codec_ctx(),audio_encoder->get_codec_ctx(),duration))
    {
        qDebug()<<"muxer启动失败";
        return;
    }

    emit start_demuxer();
    emit start_decoder();
    emit start_encoder();
    emit start_muxer();
}

void control_center::stop(){

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


void control_center::print_metadata(){
    AVDictionaryEntry *tag = NULL;
    while((tag = av_dict_get( fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX )))
    {
        qDebug()<<tag->key<<"="<<tag->value;
    }
    QFileInfo fileInfo(filepath);
    info.inputFormat = fileInfo.suffix().
                       toUpper();
    qDebug()<<"编码格式:"<<info.inputFormat;
    qDebug()<<"时长:"<<fmt_ctx->duration/AV_TIME_BASE<<"秒";
    duration = fmt_ctx->duration/AV_TIME_BASE;
    info.videoBitrate = fmt_ctx->bit_rate/1000;
    qDebug()<<"码率:"<<info.videoBitrate<<"kbps";
    qDebug()<<"流个数:"<<fmt_ctx->nb_streams;

    for(uint i=0;i<fmt_ctx->nb_streams;i++)
    {
        AVStream *stream=fmt_ctx->streams[i];
        AVCodecParameters *codec_par=stream->codecpar;


        if(codec_par->codec_type==AVMEDIA_TYPE_AUDIO)
        {
            info.in_sample_fmt = static_cast<AVSampleFormat>(codec_par->format);
            info.audio_time_base = stream->time_base;
            av_channel_layout_copy(&info.in_ch_layout, &codec_par->ch_layout);

            info.audioCodec = avcodec_get_name(codec_par->codec_id);
            qDebug()<<"音频编码格式:"<<info.audioCodec;
            info.audioChannels = codec_par->ch_layout.nb_channels;
            qDebug()<<"通道数:"<<info.audioChannels;
            info.audioSampleRate = codec_par->sample_rate;
            qDebug()<<"采样率:"<<info.audioSampleRate;
            info.audioBitrate = codec_par->bit_rate;
            audio_stream_id = i;
        }
        else if(codec_par->codec_type==AVMEDIA_TYPE_VIDEO)
        {
            info.in_pix_fmt = static_cast<AVPixelFormat>(stream->codecpar->format);
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
                sar = stream->codecpar->sample_aspect_ratio;
            }
            if (sar.num <= 0 || sar.den <= 0) {
                sar = AVRational{1, 1};
            }
            info.sar = sar;

            info.videoCodec = avcodec_get_name(codec_par->codec_id);
            qDebug()<<"视频编码格式:"<<info.videoCodec;
            qDebug()<<"帧率:"<<av_q2d(info.frameRate);
            info.width = codec_par->width;
            info.height = codec_par->height;
            qDebug()<<"分辨率:"<<info.width<<"x"<<info.height;
            video_stream_id = i;
        }
    }
}

void control_center::cancel()
{
    is_cancelled = true;
    stop();
}
