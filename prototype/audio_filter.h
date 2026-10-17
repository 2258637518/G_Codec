#ifndef AUDIO_FILTER_H
#define AUDIO_FILTER_H

#include "ffmpeg_global.h"
#include <QString>

struct AudioFilterParams {
    int sample_rate = 44100;
    AVSampleFormat sample_fmt = AV_SAMPLE_FMT_FLTP;
    AVChannelLayout ch_layout = AV_CHANNEL_LAYOUT_STEREO;
    AVRational time_base = {1, 44100};
};

class AudioFilter
{
public:
    AudioFilter();
    ~AudioFilter();

    /**
     * @brief 初始化音频滤镜图 (abuffer -> aformat/extra -> asetnsamples -> abuffersink)
     * @param in_params 源音频解码参数
     * @param out_params 目标音频编码参数
     * @param target_frame_size 目标编码器要求的每帧采样点数 (如 AAC 的 1024)
     * @param extra_filters 额外滤镜字符串 (如 "volume=1.5")
     * @return 成功返回 true，失败返回 false
     */
    bool init(const AudioFilterParams &in_params,
              const AudioFilterParams &out_params,
              int target_frame_size = 1024,
              const QString &extra_filters = "");

    /**
     * @brief 向滤镜输入音频帧
     * @param frame 待转换的原始解码帧。若传入 nullptr，表示通知滤镜开启 EOF Flush 冲刷
     * @return >= 0 成功，负数表示 FFmpeg 错误码
     */
    int send_frame(AVFrame *frame);

    /**
     * @brief 从滤镜提取处理完毕、严格定长且已对齐的音频帧
     * @param frame 接收输出数据的 AVFrame
     * @return 0 成功，AVERROR(EAGAIN) 需更多输入，AVERROR_EOF 滤镜已彻底清空，其他负数为错误
     */
    int receive_frame(AVFrame *frame);

    /**
     * @brief 释放滤镜图资源
     */
    void close();

    bool is_valid() const { return is_inited; }

private:
    AVFilterGraph *filter_graph = nullptr;
    AVFilterContext *buffersrc_ctx = nullptr;  // 输入端 (abuffer)
    AVFilterContext *buffersink_ctx = nullptr; // 输出端 (abuffersink)
    bool is_inited = false;
};

#endif // AUDIO_FILTER_H
