#ifndef TRANSCODETASK_H
#define TRANSCODETASK_H
#include "ffmpeg_global.h"
#include <QString>

enum class TaskStatus {
    Waiting,    // 排队中
    Running,    // 正在编码
    Finished,   // 已完成
    Error       // 出错
};
struct SourcecodeInfo{
    QString inputFormat;

    AVPixelFormat in_pix_fmt;
    AVRational time_base;
    AVRational frameRate;
    AVRational sar;
    QString videoCodec;
    int videoBitrate;
    int width;
    int height;

    AVSampleFormat in_sample_fmt;
    AVChannelLayout in_ch_layout;
    AVRational audio_time_base;
    QString audioCodec;
    int audioBitrate;
    int audioSampleRate;
    int audioChannels;
};

struct TranscodeTask {
    // 1. 基础路径信息
    QString inputPath;      // 输入文件路径：D:/input.avi
    QString outputPath;     // 输出文件路径：D:/output.mp4

    // 2. 目标封装格式信息
    QString outputFormat = nullptr;   // 输出封装格式：mp4, mkv, flv...

    // 3. 目标视频编码参数 (如果不 copy)
    QString videoCodec = nullptr;     // 视频编码器：libx264, libx265...
    int videoBitrate = -1;       // 视频码率：2000000 (2Mbps)
    int width = -1;              // 宽度：1920
    int height = -1;             // 高度：1080
    int frameRate = -1;          // 帧率：30
    AVPixelFormat out_pix_fmt = AV_PIX_FMT_NONE;

    // 4. 目标音频编码参数 (如果不 copy)
    QString audioCodec = nullptr;     // 音频编码器：aac, mp3...
    int audioBitrate = -1;       // 音频码率：128000
    int audioSampleRate = -1;    // 采样率：44100
    int audioChannels = -1;      // 声道数：2

    // 5. 状态与进度信息 (可与 Controller 结合使用)
    int progress;           // 转码进度：0 ~ 100
    TaskStatus state;
};

#endif // TRANSCODETASK_H
