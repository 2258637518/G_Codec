#include "demuxer.h"

Demuxer::Demuxer(SaveQueue<AVPacket*>&video_pkt,SaveQueue<AVPacket*>&audio_pkt)
    :unpack_video_pkt(video_pkt),unpack_audio_pkt(audio_pkt) {

}


bool Demuxer::init(AVFormatContext *fmt_ctx){
    if(!fmt_ctx) return false;
    this->fmt_ctx = fmt_ctx;
    video_stream_id = av_find_best_stream(fmt_ctx,AVMEDIA_TYPE_VIDEO,-1,-1,nullptr,0);
    audio_stream_id = av_find_best_stream(fmt_ctx,AVMEDIA_TYPE_AUDIO,-1,-1,nullptr,0);
    if (video_stream_id < 0 && audio_stream_id < 0) {
        qDebug() << "Demuxer: 文件中未发现有效的音视频流";
        return false;
    }
    return true;
}

void Demuxer::start(){
    qDebug()<<"开始解封装------";
    AVPacket *pkt = av_packet_alloc();
    while(true)
    {
        if(is_stopped)
        {
            qDebug()<<"Demuxer线程退出";
            break;
        }
        int ret = av_read_frame(fmt_ctx, pkt);
        if (ret >= 0)
        {
            if (pkt->stream_index == video_stream_id)
            {
                unpack_video_pkt.push(av_packet_clone(pkt));
            }
            else if (pkt->stream_index == audio_stream_id)
            {
                unpack_audio_pkt.push(av_packet_clone(pkt));
            }
            av_packet_unref(pkt);
        }
        else {
            if (ret == AVERROR_EOF) {
                qDebug() << "Demuxer: 视频输入文件已正常读取至末尾 (EOF)，发送收尾哨兵包";
            } else {
                char errbuf[128] = {0};
                av_strerror(ret, errbuf, sizeof(errbuf));
                qDebug() << "Demuxer: 解封装读取遇到异常中断:" << errbuf << " (错误码:" << ret << ")";
            }
            break;
        }

    }
    // 释放最开始 alloc 的临时 pkt，防止内存泄漏
    av_packet_free(&pkt);

    if(!is_stopped)
    {
        // 【关键】：往队列里压入 nullptr，通知解码线程该收尾了
        unpack_video_pkt.push(nullptr);
        if (audio_stream_id >= 0) {
            unpack_audio_pkt.push(nullptr);
        }
        emit finished();
    }
}

void Demuxer::stop()
{
    is_stopped = true;
}

void Demuxer::pause()
{
    printf("dsafajklfga\n");
}
