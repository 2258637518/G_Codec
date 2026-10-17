#ifndef MUXER_H
#define MUXER_H

#include <QObject>
#include "ffmpeg_global.h"
#include "save_queue.h"

class Muxer : public QObject
{
    Q_OBJECT
public:
    explicit Muxer(SaveQueue<AVPacket*>& pkt_queue, QString output_filepath = "");
    ~Muxer();

    bool init(const QString &output_path,
              int in_v_id,
              int in_a_id,
              AVCodecContext *v_codec_ctx,
              AVCodecContext *a_codec_ctx,
              int64_t duration);

public slots:
    void start();
    void stop();
    void pause();
signals:
    void finished();
    void progressUpdated(int percent);
private:
    AVFormatContext *out_fmt_ctx = nullptr;

    int in_video_stream_id = -1;
    int in_audio_stream_id = -1;
    int out_video_stream_id = -1;
    int out_audio_stream_id = -1;

    QString output_filepath;

    AVRational video_time_base;
    AVRational audio_time_base;

    int64_t duration ;
    int last_progess = 0;

    SaveQueue<AVPacket*>& pkt_queue;

    int expected_eof = 0;
    std::atomic<bool> is_stopped{false};

};

#endif // MUXER_H
