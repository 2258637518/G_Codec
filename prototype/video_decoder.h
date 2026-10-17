#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H
#include "ffmpeg_global.h"
#include <QObject>
#include "savequeue.h"
class VideoDecoder:public QObject
{
    Q_OBJECT
public:
    VideoDecoder(int id,SaveQueue<AVPacket*>&unpack_video_pkt,SaveQueue<AVFrame*>&decode_video_pkt,AVFormatContext *fmt);
    ~VideoDecoder();
    bool init();
public slots:
    void start();
    void stop();
    void pause();

signals:
    void finished();

private:
    int video_stream_id;
    AVCodecContext *codec_ctx;
    AVCodecParameters *codec_par;
    SaveQueue<AVPacket*>&unpack_video_pkt;
    SaveQueue<AVFrame*>&decode_video_pkt;
};

#endif // VIDEO_DECODER_H
