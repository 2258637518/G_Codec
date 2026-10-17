#include "decoder.h"
#include <QDebug>

Decoder::Decoder( SaveQueue<AVPacket*>& in_pkt_queue, SaveQueue<AVFrame*>& out_frame_queue)
    :in_pkt_queue(in_pkt_queue), out_frame_queue(out_frame_queue){}
Decoder::~Decoder()
{
    if (codec_ctx) {
        avcodec_free_context(&codec_ctx);
    }
    if(hw_device_ctx)
    {
        av_buffer_unref(&hw_device_ctx);
    }
}

bool Decoder::init(AVFormatContext *fmt,int id, AVBufferRef *hw_device_ctx)
{
    if (!fmt || id < 0 || (unsigned int)id >= fmt->nb_streams) {
        qDebug() << "Decoder: 非法的流 ID:" << id;
        return false;
    }
    stream_id = id;
    AVCodecParameters *codec_par = fmt->streams[stream_id]->codecpar;
    if (!codec_par) {
        qDebug() << "Decoder: 未获取到流 ID" << stream_id << "的编解码参数";
        return false;
    }

    const AVCodec *codec = avcodec_find_decoder(codec_par->codec_id);
    if (!codec) {
        qDebug() << "未找到流 ID" << stream_id << "对应的解码器";
        return false;
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) return false;

    if (avcodec_parameters_to_context(codec_ctx, codec_par) < 0) {
        avcodec_free_context(&codec_ctx);
        return false;
    }

    if(hw_device_ctx&&codec_par->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        this->hw_device_ctx = av_buffer_ref(hw_device_ctx);
        codec_ctx->hw_device_ctx = av_buffer_ref(this->hw_device_ctx);
        codec_ctx->get_format = get_hw_format;
        codec_ctx->opaque = this;
    }

    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        avcodec_free_context(&codec_ctx);
        return false;
    }
    return true;
}

void Decoder::start()
{
    qDebug() << "流 ID" << stream_id << "的解码线程已启动...";

    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        return;
    }

    bool is_eof = false;

    while (!is_eof)
    {
        if(is_stopped)
        {
            qDebug()<<"Decoder"<<stream_id<<"线程已退出";
            break;
        }
        // 1. 从队列取出待解码包（无需提前 av_packet_alloc）
        AVPacket *pkt = in_pkt_queue.pop();
        bool is_flush_pkt = (pkt == nullptr);

        // 2. 发送 pkt 到解码器（pkt 为 nullptr 时开启 Flush）
        int ret = avcodec_send_packet(codec_ctx, pkt);

        // 释放已消费的 pkt
        if (pkt) {
            av_packet_free(&pkt);
        }

        // 过滤正常流控状态（EAGAIN / AVERROR_EOF 均非致命错误）
        if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            qDebug() << "发送 pkt 到解码器致命失败，错误码:" << ret;
            break;
        }

        // 3. 循环提取解码后的 Frame
        while (true) {
            ret = avcodec_receive_frame(codec_ctx, frame);
            if (ret == AVERROR(EAGAIN)) {
                break; // 需要更多的 pkt
            } else if (ret == AVERROR_EOF) {
                qDebug() << "流 ID" << stream_id << "解码器完全冲刷完毕(EOF)";
                is_eof = true;
                break;
            } else if (ret < 0) {
                qDebug() << "接收解码 Frame 失败，错误码:" << ret;
                is_eof = true;
                break;
            }

            // 成功提取到有效 Frame，浅拷贝引用计数的 Frame 传给下游
            out_frame_queue.push(av_frame_clone(frame));
            av_frame_unref(frame);
        }

        // 4. 处理 Flush 状态：如果刚刚发送了 nullptr，不再从队列 pop，全力收尾拉取余帧
        if (is_flush_pkt && !is_eof) {
            while (!is_eof) {
                ret = avcodec_receive_frame(codec_ctx, frame);
                if (ret == AVERROR_EOF || ret < 0) {
                    qDebug() << "流 ID" << stream_id << "解码器末尾余帧提取完毕";
                    is_eof = true;
                    break;
                }
                out_frame_queue.push(av_frame_clone(frame));
                av_frame_unref(frame);
            }
        }
    }

    // 资源清理与哨兵传递
    av_frame_free(&frame);
    if(!is_stopped)
    {
        out_frame_queue.push(nullptr); // 传递空指针告知下游编码器/渲染器：解码完全结束
        qDebug() << "流 ID" << stream_id << "解码线程安全退出。";
        emit finished();
    }
}

void Decoder::stop() {
    is_stopped = true;
}
void Decoder::pause() {}

AVPixelFormat Decoder::get_hw_format(AVCodecContext *ctx, const AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;
    for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
        if (*p == AV_PIX_FMT_CUDA) {
            qDebug() << "Decoder: 成功协商并命中硬解格式 AV_PIX_FMT_CUDA";
            return *p;
        }
    }
    qDebug() << "Decoder: 该视频流未支持 CUDA 硬件像素格式，回退至软解";
    return AV_PIX_FMT_NONE;
}
