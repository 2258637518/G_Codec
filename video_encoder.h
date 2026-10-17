#ifndef VIDEO_ENCODER_H
#define VIDEO_ENCODER_H

#include <QObject>
#include <memory>
#include "ffmpeg_global.h"
#include "save_queue.h"
#include "transcode_task.h"
#include "video_filter.h"

class VideoEncoder : public QObject
{
    Q_OBJECT
public:
    explicit VideoEncoder(TranscodeTask &task_info,
                          SaveQueue<AVFrame*>& decode_video_frame,
                          SaveQueue<AVPacket*>& pack_video_frame);
    ~VideoEncoder();

    /**
     * @brief 初始化视频编码器及内部持有的 VideoFilter 滤镜
     * @param id 视频流索引
     * @param in_pix_fmt 源视频像素格式
     * @param in_w 源视频宽度
     * @param in_h 源视频高度
     * @param in_time_base 源视频时间基
     * @param in_frame_rate 源视频帧率
     * @param in_sar 源视频像素宽高比
     * @return 成功返回 true
     */
    bool init(int id,
              AVPixelFormat in_pix_fmt,
              int in_w,
              int in_h,
              AVRational in_time_base,
              AVRational in_frame_rate = {30, 1},
              AVRational in_sar = {1, 1},
              AVBufferRef *hw_device_ctx = nullptr);

    AVCodecContext* get_codec_ctx() const { return codec_ctx; }

public slots:
    void start();
    void stop();
    void pause();

signals:
    void finished();

private:
    int video_stream_id = -1;
    TranscodeTask &task_info;

    SaveQueue<AVFrame*>& decode_video_frame;
    SaveQueue<AVPacket*>& pack_video_frame;

    AVCodecContext *codec_ctx = nullptr;
    std::unique_ptr<VideoFilter> video_filter;

    AVRational in_time_base;
    AVRational in_frame_rate{30, 1};
    AVRational in_sar{1, 1};
    AVPixelFormat in_pix_fmt = AV_PIX_FMT_NONE;

    std::atomic<bool> is_stopped{false};

    AVBufferRef *hw_device_ctx = nullptr;
    AVBufferRef *hw_frames_ctx = nullptr;
    bool pass_filter = false;
    bool is_gpu_enc = false;
    bool filter_inited_with_real_frame = false;
};

#endif // VIDEO_ENCODER_H
