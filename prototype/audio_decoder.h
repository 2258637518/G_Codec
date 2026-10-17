#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

#include <QObject>
#include "ffmpeg_global.h"
#include "savequeue.h"

class audio_decoder : public QObject
{
    Q_OBJECT
public:
    explicit audio_decoder(int id,SaveQueue<AVPacket*>&unpack_audio_pkt,SaveQueue<AVFrame*>&decode_audio_pkt,AVFormatContext *fmt);
    bool init();
public slots:
    void start();
    void stop();
    void pause();
signals:
    void finished();

private:
    int audio_stream_id;
    AVCodecContext *codec_ctx;
    AVCodecParameters *codec_par;
    SaveQueue<AVPacket*>&unpack_audio_pkt;
    SaveQueue<AVFrame*>&decode_audio_pkt;
};

#endif // AUDIO_DECODER_H
