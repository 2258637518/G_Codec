#include "video_decoder.h"

VideoDecoder::VideoDecoder(int id,SaveQueue<AVPacket*>&unpack_video_pkt,SaveQueue<AVFrame*>&decode_video_pkt,AVFormatContext *fmt)
    :unpack_video_pkt(unpack_video_pkt),decode_video_pkt(decode_video_pkt){
    video_stream_id = id;
    codec_par = fmt->streams[id]->codecpar;
}

VideoDecoder::~VideoDecoder()
{
    if(codec_ctx)
    {
        avcodec_free_context(&codec_ctx);
    }
    if(codec_par)
    {
        avcodec_parameters_free(&codec_par);
    }
}

bool VideoDecoder::init()
{
    const AVCodec *codec = nullptr;
    codec = avcodec_find_decoder(codec_par->codec_id);
    if(!codec)
    {
        qDebug()<<"未找到解码器";
        return false;
    }
    codec_ctx = avcodec_alloc_context3(codec);

    if (avcodec_parameters_to_context(codec_ctx, codec_par) < 0) {
        avcodec_free_context(&this->codec_ctx);
        return false;
    }

    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        avcodec_free_context(&this->codec_ctx);
        return false;
    }
    return true;
}

void VideoDecoder::start()
{
    qDebug()<<"视频解码线程启动";
    AVFrame *frame = av_frame_alloc();
    AVPacket *pkt = nullptr;
    while(true)
    {
        // while(decode_video_pkt.exceed())
        // {
        //     std::this_thread::sleep_for(std::chrono::milliseconds(10));
        // }
        pkt = unpack_video_pkt.pop();
        int ret = avcodec_send_packet(codec_ctx,pkt);
        av_packet_free(&pkt);
        if(ret < 0)
        {
            qDebug()<<"发送pkt到解码器失败";
            return;
        }
        while(ret >= 0)
        {
            ret = avcodec_receive_frame(codec_ctx,frame);
            if (ret == AVERROR(EAGAIN)) {
                // EAGAIN 代表：解码器里没有存货了，需要送入更多的 Packet 才能解出下一帧
                break;
            } else if (ret == AVERROR_EOF) {
                // AVERROR_EOF 代表：解码器已经完全冲刷（Flush）完毕，所有残留帧都已全部输出
                break;
            } else if (ret < 0) {
                // 真正的解码错误
                qDebug() << "接收解码 Frame 失败，错误码:" << ret;
                break;
            }
            decode_video_pkt.push(av_frame_clone(frame));
            av_frame_unref(frame);
        }
    }
    av_frame_free(&frame);
    emit finished();
}

void VideoDecoder::stop()
{

}

void VideoDecoder::pause()
{

}

