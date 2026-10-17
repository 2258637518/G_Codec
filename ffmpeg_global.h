#ifndef FFMPEG_GLOBAL_H
#define FFMPEG_GLOBAL_H

// 消除 Qt Creator 内置 Clang Code Model (clangd) 索引 MSVC 最新 STL 时的静态断言误报
#ifndef _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH
#define _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH
#endif

#include <QString>
#include <QDebug>

extern "C"{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/pixdesc.h>
#include <libavutil/samplefmt.h>
#include <libavutil/imgutils.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/hwcontext.h>
}

#endif // FFMPEG_GLOBAL_H


