#ifndef CONTROL_CENTER_H
#define CONTROL_CENTER_H
#include "decoder.h"
#include "demuxer.h"
#include "video_encoder.h"
#include "audio_encoder.h"
#include "ffmpeg_global.h"
#include "transcode_task.h"
#include <QObject>
#include <QThread>
#include <QTimer>
#include "save_queue.h"
#include "muxer.h"

class ControlCenter : public QObject
{
    Q_OBJECT
public:
    ControlCenter(QString filepath, int taskID, TranscodeTask task_info, QObject *parent = nullptr);
    ~ControlCenter();
    bool init();
    void open();
    void print_metadata();
    void cancel();
    void set_task_info(const TranscodeTask &task_info) { this->task_info = task_info; }
    SourcecodeInfo info;
public slots:

    void start();
    void stop();

signals:
    void metadataReady(int taskID,SourcecodeInfo info);
    void metadataError(int taskID);
    void transcodeError(int taskID, const QString &title, const QString &detail);
    void progress_updata(int percent);
    void finished();

    void start_demuxer();
    void stop_demuxer();
    void pause_demuxer();

    void start_video_decoder();
    void start_audio_decoder();
    void stop_decoder();
    void pause_decoder();

    void start_video_encoder();
    void start_audio_encoder();
    void stop_encoder();
    void pause_encoder();

    void start_muxer();
    void stop_muxer();
    void pause_muxer();

private:
    std::atomic_bool is_cancelled = false;
    int taskID;
    int video_stream_id = -1;
    int audio_stream_id = -1;
    int64_t duration = -1 ;
    QString filepath;
    TranscodeTask task_info;

    AVFormatContext *fmt_ctx = nullptr;
    AVBufferRef *hw_device_ctx = nullptr;

    std::unique_ptr<Demuxer> demuxer;
    SaveQueue<AVPacket*>unpack_video_pkt;
    SaveQueue<AVPacket*>unpack_audio_pkt;
    QThread *demuxer_thread = nullptr;

    std::unique_ptr<Decoder> video_decoder;
    std::unique_ptr<Decoder> audio_decoder;
    SaveQueue<AVFrame*>decode_video_frame;
    SaveQueue<AVFrame*>decode_audio_frame;
    QThread *video_decoder_thread = nullptr;
    QThread *audio_decoder_thread = nullptr;

    std::unique_ptr<VideoEncoder> video_encoder;
    std::unique_ptr<AudioEncoder> audio_encoder;
    SaveQueue<AVPacket*> pack_pkt;
    QThread *video_encode_thread = nullptr;
    QThread *audio_encode_thread = nullptr;

    std::unique_ptr<Muxer> muxer;
    QThread *muxer_thread = nullptr;

    void reportError(const QString &title, const QString &detail);
};

using control_center = ControlCenter;

#endif // CONTROL_CENTER_H
