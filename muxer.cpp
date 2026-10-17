#include "muxer.h"

Muxer::Muxer(SaveQueue<AVPacket*>& pkt_queue,QString output_filepath) : pkt_queue(pkt_queue),output_filepath(output_filepath)
{}

Muxer::~Muxer()
{
    if(out_fmt_ctx)
    {
        if (!(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&out_fmt_ctx->pb);
        }
        avformat_free_context(out_fmt_ctx);
        out_fmt_ctx = nullptr;
    }
}

bool Muxer::init(const QString &output_path, int in_v_id, int in_a_id,
                 AVCodecContext *v_codec_ctx, AVCodecContext *a_codec_ctx,
                int64_t duration)
{
    if (!output_path.isEmpty()) {
        output_filepath = output_path;
    }
    in_video_stream_id = in_v_id;
    in_audio_stream_id = in_a_id;

    if(duration!=-1)
    {
        this->duration = duration;
    }

    if (output_filepath.isEmpty()) {
        qDebug() << "Muxer: 输出文件路径为空！";
        return false;
    }

    avformat_alloc_output_context2(&out_fmt_ctx, nullptr, nullptr, output_filepath.toUtf8().constData());
    if (!out_fmt_ctx){
        qDebug()<<"封装时AVFormatContext打开失败.....";
        return false;
    }
    expected_eof = 0;
    if (v_codec_ctx && in_video_stream_id >= 0) {
        AVStream *video_stream = avformat_new_stream(out_fmt_ctx, nullptr);
        out_video_stream_id = video_stream->index;
        avcodec_parameters_from_context(video_stream->codecpar, v_codec_ctx);
        video_time_base = v_codec_ctx->time_base;
        expected_eof++;
    }

    if (a_codec_ctx && in_audio_stream_id >= 0) {
        AVStream *audio_stream = avformat_new_stream(out_fmt_ctx, nullptr);
        out_audio_stream_id = audio_stream->index;
        avcodec_parameters_from_context(audio_stream->codecpar, a_codec_ctx);
        audio_time_base = a_codec_ctx->time_base;
        expected_eof++;
    }

    // 打开磁盘文件以供写入
    if (!(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open2(&out_fmt_ctx->pb, output_filepath.toUtf8().constData(), AVIO_FLAG_WRITE, nullptr, nullptr) < 0) {
            qDebug()<<"磁盘文件打开失败";
            return false;
        }
    }

    // 写入封装文件头（Header）
    if (avformat_write_header(out_fmt_ctx, nullptr) < 0)
    {
        qDebug()<<"封装文件头写入失败";
        return false;
    }
    return true;
}

void Muxer::start()
{
    qDebug() << "Muxer已启动,正在封装视频数据......";
    AVPacket *pkt = nullptr;
    int eof_count = 0;
    while (true)
    {
        if(is_stopped)
        {
            break;
        }
        pkt = pkt_queue.pop();
        if (pkt)
        {
            if (pkt->stream_index == in_video_stream_id)
            {
                pkt->stream_index = out_video_stream_id;
                pkt->pos = -1;
                av_packet_rescale_ts(pkt, video_time_base, out_fmt_ctx->streams[out_video_stream_id]->time_base);
            }
            else if (pkt->stream_index == in_audio_stream_id)
            {
                pkt->stream_index = out_audio_stream_id;
                pkt->pos = -1;
                av_packet_rescale_ts(pkt, audio_time_base, out_fmt_ctx->streams[out_audio_stream_id]->time_base);
            }

            if(pkt->stream_index == out_video_stream_id){
                double current_sec = pkt->pts*av_q2d(out_fmt_ctx->streams[pkt->stream_index]->time_base);
                if (duration > 0) {
                    int current_progress = static_cast<int>((current_sec / duration) * 100);
                    // ... 限制在 0 ~ 100 之间并节流 emit
                    if(current_progress > last_progess && current_progress <=100){
                    last_progess = current_progress;
                    emit progressUpdated(last_progess);
                    }
                }

            }

            if (av_interleaved_write_frame(out_fmt_ctx, pkt) < 0)
            {
                qDebug() << "写入文件时出现错误(中途)";
                av_packet_free(&pkt);
                break;
            }
            av_packet_free(&pkt);
            pkt = nullptr;

        }
        else
        {
            eof_count++;
        }

        if (expected_eof > 0 && eof_count >= expected_eof) break;
    }
    if(!is_stopped)
    {
        if (av_write_trailer(out_fmt_ctx) < 0)
        {
            qDebug() << "写入文件时出现错误(末尾)";
            return;
        }

        emit progressUpdated(100);
        emit finished();
    }
    if (out_fmt_ctx && !(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&out_fmt_ctx->pb); // 释放 Windows 文件锁并将 pb 置空
    }
}

void Muxer::stop()
{
    qDebug()<<"Muxer已停止";
    is_stopped = true;
}

void Muxer::pause()
{
    qDebug()<<"Muxer已暂停";
}


