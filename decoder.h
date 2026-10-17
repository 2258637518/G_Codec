#ifndef DECODER_H
#define DECODER_H

#include "ffmpeg_global.h"
#include <QObject>
#include "save_queue.h"

class Decoder : public QObject
{
    Q_OBJECT
public:
    Decoder(SaveQueue<AVPacket*>& in_pkt_queue, SaveQueue<AVFrame*>& out_frame_queue);
    ~Decoder();

    bool init(AVFormatContext *fmt,int id, AVBufferRef *hw_device_ctx = nullptr);

public slots:
    void start();
    void stop();
    void pause();

signals:
    void finished();

private:
    int stream_id;
    AVCodecContext *codec_ctx = nullptr;
    AVBufferRef *hw_device_ctx = nullptr;
    
    SaveQueue<AVPacket*>& in_pkt_queue;
    SaveQueue<AVFrame*>& out_frame_queue;

    std::atomic<bool> is_stopped{false};


    static enum AVPixelFormat get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts);
};

#endif // DECODER_H
