#include "audioencoder.h"
#include <QDebug>

AudioEncoder::AudioEncoder(TranscodeTask &task_info,
                           SaveQueue<AVFrame*>& decode_audio_frame,
                           SaveQueue<AVPacket*>& pack_audio_frame)
    : task_info(task_info),
      decode_audio_frame(decode_audio_frame),
      pack_audio_frame(pack_audio_frame),
      audio_filter(std::make_unique<AudioFilter>())
{
}

AudioEncoder::~AudioEncoder()
{
    if (codec_ctx) {
        avcodec_free_context(&codec_ctx);
        codec_ctx = nullptr;
    }
}

bool AudioEncoder::init(int id,
                        AVSampleFormat in_sample_fmt,
                        int in_sample_rate,
                        AVChannelLayout in_ch_layout,
                        AVRational in_time_base)
{
    audio_stream_id = id;
    this->in_time_base = in_time_base;

    // 1. 查找目标音频编码器
    const AVCodec *codec = nullptr;
    if (!task_info.audioCodec.isEmpty()) {
        codec = avcodec_find_encoder_by_name(task_info.audioCodec.toUtf8().constData());
    } else {
        codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    }

    if (!codec) {
        qDebug() << "AudioEncoder: 找不到指定的音频编码器:" << task_info.audioCodec;
        return false;
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        qDebug() << "AudioEncoder: 编码器上下文分配失败";
        return false;
    }

    // 2. 确定目标采样率
    int out_sample_rate = (task_info.audioSampleRate > 0) ? task_info.audioSampleRate : in_sample_rate;
    if (codec->supported_samplerates) {
        bool supported = false;
        for (int i = 0; codec->supported_samplerates[i]; i++) {
            if (codec->supported_samplerates[i] == out_sample_rate) {
                supported = true;
                break;
            }
        }
        if (!supported) {
            out_sample_rate = codec->supported_samplerates[0];
        }
    }
    codec_ctx->sample_rate = out_sample_rate;

    // 3. 确定目标声道布局
    AVChannelLayout out_ch_layout;
    if (task_info.audioChannels > 0) {
        av_channel_layout_default(&out_ch_layout, task_info.audioChannels);
    } else {
        av_channel_layout_copy(&out_ch_layout, &in_ch_layout);
    }
    av_channel_layout_copy(&codec_ctx->ch_layout, &out_ch_layout);

    // 4. 确定目标采样格式 (优先选择编码器支持的最佳格式)
    AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_NONE;
    if (codec->sample_fmts) {
        out_sample_fmt = codec->sample_fmts[0];
    } else {
        out_sample_fmt = AV_SAMPLE_FMT_FLTP;
    }
    codec_ctx->sample_fmt = out_sample_fmt;

    // 5. 确定目标码率
    codec_ctx->bit_rate = (task_info.audioBitrate > 0) ? task_info.audioBitrate : 128000;

    // 6. 时间基与全局头标志
    codec_ctx->time_base = AVRational{1, out_sample_rate};
    codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // 7. 打开编码器
    int ret = avcodec_open2(codec_ctx, codec, nullptr);
    if (ret < 0) {
        char errbuf[128] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        qDebug() << "AudioEncoder: 打开编码器失败:" << errbuf;
        av_channel_layout_uninit(&out_ch_layout);
        return false;
    }

    // 8. 初始化持有的 AudioFilter 滤镜
    AudioFilterParams in_params;
    in_params.sample_fmt = in_sample_fmt;
    in_params.sample_rate = in_sample_rate;
    av_channel_layout_copy(&in_params.ch_layout, &in_ch_layout);
    in_params.time_base = in_time_base;

    AudioFilterParams out_params;
    out_params.sample_fmt = out_sample_fmt;
    out_params.sample_rate = out_sample_rate;
    av_channel_layout_copy(&out_params.ch_layout, &out_ch_layout);
    out_params.time_base = codec_ctx->time_base;

    int target_frame_size = codec_ctx->frame_size > 0 ? codec_ctx->frame_size : 1024;
    if (!audio_filter->init(in_params, out_params, target_frame_size)) {
        qDebug() << "AudioEncoder: 初始化 AudioFilter 失败";
        av_channel_layout_uninit(&in_params.ch_layout);
        av_channel_layout_uninit(&out_params.ch_layout);
        av_channel_layout_uninit(&out_ch_layout);
        return false;
    }

    av_channel_layout_uninit(&in_params.ch_layout);
    av_channel_layout_uninit(&out_params.ch_layout);
    av_channel_layout_uninit(&out_ch_layout);

    qDebug() << "AudioEncoder: 初始化成功! 目标编码器:" << codec->name
             << " 采样率:" << out_sample_rate
             << " 声道数:" << codec_ctx->ch_layout.nb_channels
             << " 码率:" << codec_ctx->bit_rate
             << " frame_size:" << target_frame_size;

    return true;
}

void AudioEncoder::start()
{
    qDebug() << "音频编码线程已启动......";

    AVPacket *pkt = av_packet_alloc();
    if (!pkt) {
        qDebug() << "AudioEncoder: Packet 内存分配失败";
        emit finished();
        return;
    }

    bool is_eof = false;

    while (!is_eof)
    {


        // 1. 从解码队列取出待编码的原始音频帧
        AVFrame *src_frame = decode_audio_frame.pop();

        if(is_stopped)
        {

            av_packet_free(&pkt);
            if (src_frame) av_frame_free(&src_frame);
            qDebug()<<"AudioEncoder线程已取消";
            break;
        }
        // 2. 发送给滤镜 (若 src_frame 为 nullptr，则触发滤镜的 Flush 冲刷)
        int send_ret = audio_filter->send_frame(src_frame);
        if (src_frame) {
            av_frame_free(&src_frame);
        }

        if (send_ret < 0) {
            qDebug() << "AudioEncoder: 发送帧至滤镜失败，错误码:" << send_ret;
            break;
        }

        // 3. 从滤镜拉取所有已完成格式重采样、声道重映射、定长切片及尾部 padding 补齐的音频帧
        while (true)
        {

            AVFrame *filt_frame = av_frame_alloc();
            int ret = audio_filter->receive_frame(filt_frame);

            if (ret == AVERROR(EAGAIN)) {
                // 当前输入数据不足以产出一个完整的 frame_size，需要继续从上游队列 pop 原始帧
                av_frame_free(&filt_frame);
                break;
            } else if (ret == AVERROR_EOF) {
                // 滤镜已经完全冲刷完毕，不再有任何输出
                av_frame_free(&filt_frame);
                is_eof = true;
                break;
            } else if (ret < 0) {
                qDebug() << "AudioEncoder: 从滤镜接收音频帧失败，错误码:" << ret;
                av_frame_free(&filt_frame);
                is_eof = true;
                break;
            }

            // 4. 将规整好的帧送入音频编码器
            ret = avcodec_send_frame(codec_ctx, filt_frame);
            av_frame_free(&filt_frame);

            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                qDebug() << "AudioEncoder: 发送帧到编码器失败，错误码:" << ret;
                is_eof = true;
                break;
            }

            // 5. 循环提取编码好的 AVPacket
            while (true) {
                ret = avcodec_receive_packet(codec_ctx, pkt);
                if (ret == AVERROR(EAGAIN)) {
                    break;
                } else if (ret == AVERROR_EOF) {
                    is_eof = true;
                    break;
                } else if (ret < 0) {
                    qDebug() << "AudioEncoder: 接收编码 Packet 失败，错误码:" << ret;
                    is_eof = true;
                    break;
                }

                pkt->stream_index = audio_stream_id;
                pack_audio_frame.push(av_packet_clone(pkt));
                av_packet_unref(pkt);
            }
        }
    }

    if(!is_stopped)
        {
        // 6. 冲刷编码器自身的余包 (Flush 编码器)
        avcodec_send_frame(codec_ctx, nullptr);
        while (avcodec_receive_packet(codec_ctx, pkt) == 0) {
            pkt->stream_index = audio_stream_id;
            pack_audio_frame.push(av_packet_clone(pkt));
            av_packet_unref(pkt);
        }

        // 资源清理与向下游（封装 Muxer 线程）传递哨兵
        av_packet_free(&pkt);
        pack_audio_frame.push(nullptr); // 告知 Muxer 线程：音频流全部编码结束
    }

    qDebug() << "音频编码线程安全退出。";
    emit finished();
}

void AudioEncoder::stop()
{
    is_stopped = true;
}

void AudioEncoder::pause()
{
    qDebug() << "AudioEncoder::pause";
}
