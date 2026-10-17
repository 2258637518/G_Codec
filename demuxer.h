#ifndef DEMUXER_H
#define DEMUXER_H
#include "ffmpeg_global.h"
#include "save_queue.h"
#include <QObject>

class Demuxer:public QObject
{
    Q_OBJECT
public:
    Demuxer(SaveQueue<AVPacket*>&video_pkt,SaveQueue<AVPacket*>&audio_pkt);

    bool init(AVFormatContext *fmt_ctx);
    int get_video_stream_id(){return video_stream_id;}
    int get_audio_stream_id(){return audio_stream_id;}

public slots:

    void start();
    void stop();
    void pause();

signals:
    void finished();

private:
    AVFormatContext *fmt_ctx = nullptr;

    std::atomic<bool> is_stopped{false};

    SaveQueue<AVPacket*>&unpack_video_pkt;
    SaveQueue<AVPacket*>&unpack_audio_pkt;

    int video_stream_id = -1;
    int audio_stream_id = -1;
};

#endif // DEMUXER_H
