#ifndef VIDEO_FILTER_H
#define VIDEO_FILTER_H

#include "ffmpeg_global.h"
#include <QString>

struct VideoFilterParams {
    int width = 0;
    int height = 0;
    AVPixelFormat pix_fmt = AV_PIX_FMT_NONE;
    AVRational time_base = {1, 1000};
    AVRational frame_rate = {30, 1};
    AVRational sample_aspect_ratio = {1, 1};
};

class VideoFilter
{
public:
    VideoFilter();
    ~VideoFilter();

    /**
     * @brief 初始化视频滤镜图 (buffer -> scale -> fps -> format -> buffersink)
     * @param in_params 源视频参数
     * @param out_params 目标视频参数（含目标宽高、目标帧率、目标像素格式）
     * @param extra_filters 附加滤镜（可选，如旋转、锐化、去噪等）
     * @return 成功返回 true
     */
    bool init(const VideoFilterParams &in_params,
              const VideoFilterParams &out_params,
              const QString &extra_filters = "",
              AVBufferRef *hw_device_ctx = nullptr);

    /**
     * @brief 向视频滤镜输入待处理帧
     * @param frame 原始解码视频帧。若传入 nullptr，表示通知滤镜开启 EOF Flush 冲刷
     * @return >= 0 成功，负数表示 FFmpeg 错误码
     */
    int send_frame(AVFrame *frame);

    /**
     * @brief 从滤镜提取处理完毕、严格对齐帧率与分辨率的标准视频帧
     * @param frame 接收输出数据的 AVFrame
     * @return 0 成功，AVERROR(EAGAIN) 需更多输入，AVERROR_EOF 滤镜已彻底冲刷，其他负数为错误
     */
    int receive_frame(AVFrame *frame);

    /**
     * @brief 释放滤镜图资源
     */
    void close();

    bool is_valid() const { return is_inited; }

    AVRational get_time_base() const {
        return buffersink_ctx ? av_buffersink_get_time_base(buffersink_ctx) : AVRational{1, 1000};
    }

private:
    AVFilterGraph *filter_graph = nullptr;
    AVFilterContext *buffersrc_ctx = nullptr;  // 输入端 (buffer)
    AVFilterContext *buffersink_ctx = nullptr; // 输出端 (buffersink)
    bool is_inited = false;
};

#endif // VIDEO_FILTER_H
