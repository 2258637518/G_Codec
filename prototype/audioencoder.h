#ifndef AUDIOENCODER_H
#define AUDIOENCODER_H

#include <QObject>
#include <memory>
#include "ffmpeg_global.h"
#include "savequeue.h"
#include "transcodetask.h"
#include "audio_filter.h"

class AudioEncoder : public QObject
{
    Q_OBJECT
public:
    explicit AudioEncoder(TranscodeTask &task_info,
                          SaveQueue<AVFrame*>& decode_audio_frame,
                          SaveQueue<AVPacket*>& pack_audio_frame);
    ~AudioEncoder();

    /**
     * @brief 初始化音频编码器及内部持有的 AudioFilter
     * @param id 音频流索引
     * @param in_sample_fmt 源音频采样格式
     * @param in_sample_rate 源音频采样率
     * @param in_ch_layout 源音频声道布局
     * @param in_time_base 源音频时间基
     * @return 成功返回 true
     */
    bool init(int id,
              AVSampleFormat in_sample_fmt,
              int in_sample_rate,
              AVChannelLayout in_ch_layout,
              AVRational in_time_base);

    AVCodecContext* get_codec_ctx() const { return codec_ctx; }

public slots:
    void start();
    void stop();
    void pause();

signals:
    void finished();

private:
    int audio_stream_id = -1;
    TranscodeTask &task_info;

    SaveQueue<AVFrame*>& decode_audio_frame;
    SaveQueue<AVPacket*>& pack_audio_frame;

    AVCodecContext *codec_ctx = nullptr;
    std::unique_ptr<AudioFilter> audio_filter;

    AVRational in_time_base;

    std::atomic<bool> is_stopped{false};
};

#endif // AUDIOENCODER_H
