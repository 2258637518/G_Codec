#include "video_filter.h"
#include <QDebug>
#include <QStringList>

VideoFilter::VideoFilter() {}

VideoFilter::~VideoFilter()
{
    close();
}

void VideoFilter::close()
{
    if (filter_graph) {
        avfilter_graph_free(&filter_graph);
        filter_graph = nullptr;
    }
    buffersrc_ctx = nullptr;
    buffersink_ctx = nullptr;
    is_inited = false;
}

bool VideoFilter::init(const VideoFilterParams &in_params,
                       const VideoFilterParams &out_params,
                       const QString &extra_filters,
                       AVBufferRef *hw_device_ctx,
                       AVBufferRef *hw_frames_ctx)
{
    close();

    filter_graph = avfilter_graph_alloc();
    if (!filter_graph) {
        qDebug() << "VideoFilter: avfilter_graph_alloc 失败";
        return false;
    }

    const AVFilter *buffer = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    if (!buffer || !buffersink) {
        qDebug() << "VideoFilter: 找不到 buffer 或 buffersink 滤镜";
        close();
        return false;
    }

    bool is_gpu = (hw_device_ctx != nullptr && in_params.pix_fmt == AV_PIX_FMT_CUDA);

    // 1. 创建并初始化输入端 buffersrc_ctx
    if (is_gpu) {
        // GPU 硬件模式：通过 AVBufferSrcParameters 配置硬件帧上下文
        buffersrc_ctx = avfilter_graph_alloc_filter(filter_graph, buffer, "in");
        if (!buffersrc_ctx) {
            qDebug() << "VideoFilter: 分配 GPU buffersrc 失败";
            close();
            return false;
        }

        // 优先使用真实解码帧附带的 hw_frames_ctx，彻底避免 hwdownload 时上下文指针不匹配
        AVBufferRef *actual_hw_frames_ctx = nullptr;
        if (hw_frames_ctx) {
            actual_hw_frames_ctx = av_buffer_ref(hw_frames_ctx);
        } else {
            actual_hw_frames_ctx = av_hwframe_ctx_alloc(hw_device_ctx);
            if (actual_hw_frames_ctx) {
                AVHWFramesContext *frames_ctx = (AVHWFramesContext *)(actual_hw_frames_ctx->data);
                frames_ctx->format = AV_PIX_FMT_CUDA;
                frames_ctx->sw_format = AV_PIX_FMT_NV12;
                frames_ctx->width = in_params.width;
                frames_ctx->height = in_params.height;
                frames_ctx->initial_pool_size = 0;
                if (av_hwframe_ctx_init(actual_hw_frames_ctx) < 0) {
                    qDebug() << "VideoFilter: 兜底 av_hwframe_ctx_init 失败";
                    av_buffer_unref(&actual_hw_frames_ctx);
                    close();
                    return false;
                }
            }
        }

        AVBufferSrcParameters *par = av_buffersrc_parameters_alloc();
        par->width = in_params.width;
        par->height = in_params.height;
        par->format = AV_PIX_FMT_CUDA;
        par->time_base = (in_params.time_base.num > 0 && in_params.time_base.den > 0) ? in_params.time_base : AVRational{1, 1000};
        par->sample_aspect_ratio = in_params.sample_aspect_ratio;
        if (actual_hw_frames_ctx) {
            par->hw_frames_ctx = av_buffer_ref(actual_hw_frames_ctx);
            av_buffer_unref(&actual_hw_frames_ctx);
        }
        av_buffersrc_parameters_set(buffersrc_ctx, par);
        av_free(par);

        if (avfilter_init_str(buffersrc_ctx, nullptr) < 0) {
            qDebug() << "VideoFilter: GPU buffersrc 初始化失败";
            close();
            return false;
        }
    } else {
        // CPU 软件模式：使用原有参数字符串
        char args[512] = {0};
        snprintf(args, sizeof(args),
                 "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
                 in_params.width,
                 in_params.height,
                 in_params.pix_fmt,
                 in_params.time_base.num > 0 ? in_params.time_base.num : 1,
                 in_params.time_base.den > 0 ? in_params.time_base.den : 1000,
                 in_params.sample_aspect_ratio.num > 0 ? in_params.sample_aspect_ratio.num : 1,
                 in_params.sample_aspect_ratio.den > 0 ? in_params.sample_aspect_ratio.den : 1);

        int ret = avfilter_graph_create_filter(&buffersrc_ctx, buffer, "in", args, NULL, filter_graph);
        if (ret < 0) {
            char errbuf[128] = {0};
            av_strerror(ret, errbuf, sizeof(errbuf));
            qDebug() << "VideoFilter: 创建 CPU buffer 失败:" << errbuf << " args:" << args;
            close();
            return false;
        }
    }

    // 2. 创建 buffersink 输出滤镜上下文
    int ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, NULL, filter_graph);
    if (ret < 0) {
        char errbuf[128] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        qDebug() << "VideoFilter: 创建 buffersink 失败:" << errbuf;
        close();
        return false;
    }

    // 3. 构建滤镜链描述
    QStringList filter_items;

    if (!extra_filters.isEmpty()) {
        filter_items << extra_filters;
    }

    bool in_is_gpu = (hw_device_ctx != nullptr && in_params.pix_fmt == AV_PIX_FMT_CUDA);
    bool out_is_gpu = (out_params.pix_fmt == AV_PIX_FMT_CUDA);

    if (in_is_gpu && out_is_gpu) {
        // 模式 1：全 GPU 零拷贝滤镜链 (scale_cuda)
        if (out_params.width > 0 && out_params.height > 0 &&
            (out_params.width != in_params.width || out_params.height != in_params.height)) {
            filter_items << QString("scale_cuda=%1:%2").arg(out_params.width).arg(out_params.height);
        }

        if (out_params.frame_rate.num > 0 && out_params.frame_rate.den > 0 &&
            (out_params.frame_rate.num != in_params.frame_rate.num || out_params.frame_rate.den != in_params.frame_rate.den)) {
            double fps_val = static_cast<double>(out_params.frame_rate.num) / out_params.frame_rate.den;
            filter_items << QString("fps=fps=%1").arg(fps_val);
        }
    } else if (in_is_gpu && !out_is_gpu) {
        // 模式 2：GPU 解码 -> CPU 编码桥接 (hwdownload -> scale -> format -> CPU)
        filter_items << "hwdownload" << "format=nv12";

        // 插入 scale 滤镜完成分辨率缩放与向目标像素格式 (如 ProRes 的 yuv422p10le) 的色彩空间转换
        if (out_params.width > 0 && out_params.height > 0 &&
            (out_params.width != in_params.width || out_params.height != in_params.height)) {
            filter_items << QString("scale=%1:%2:flags=bicubic").arg(out_params.width).arg(out_params.height);
        } else {
            filter_items << "scale";
        }

        if (out_params.pix_fmt != AV_PIX_FMT_NONE) {
            filter_items << QString("format=pix_fmts=%1").arg(av_get_pix_fmt_name(out_params.pix_fmt));
        }

        if (out_params.frame_rate.num > 0 && out_params.frame_rate.den > 0 &&
            (out_params.frame_rate.num != in_params.frame_rate.num || out_params.frame_rate.den != in_params.frame_rate.den)) {
            double fps_val = static_cast<double>(out_params.frame_rate.num) / out_params.frame_rate.den;
            filter_items << QString("fps=fps=%1").arg(fps_val);
        }
    } else {
        // 模式 3：纯 CPU 软件滤镜链 (scale, fps, format)
        if (out_params.width > 0 && out_params.height > 0) {
            filter_items << QString("scale=%1:%2:flags=bicubic").arg(out_params.width).arg(out_params.height);
        }

        if (out_params.frame_rate.num > 0 && out_params.frame_rate.den > 0) {
            double fps_val = static_cast<double>(out_params.frame_rate.num) / out_params.frame_rate.den;
            filter_items << QString("fps=fps=%1").arg(fps_val);
        }

        if (out_params.pix_fmt != AV_PIX_FMT_NONE) {
            filter_items << QString("format=pix_fmts=%1").arg(av_get_pix_fmt_name(out_params.pix_fmt));
        }
    }

    QString filter_desc = filter_items.isEmpty() ? "null" : filter_items.join(",");

    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();

    if (!outputs || !inputs) {
        avfilter_inout_free(&outputs);
        avfilter_inout_free(&inputs);
        close();
        return false;
    }

    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    inputs->name        = av_strdup("out");
    inputs->filter_ctx  = buffersink_ctx;
    inputs->pad_idx     = 0;
    inputs->next        = NULL;

    ret = avfilter_graph_parse_ptr(filter_graph, filter_desc.toUtf8().constData(), &inputs, &outputs, NULL);
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    if (ret < 0) {
        char errbuf[128] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        qDebug() << "VideoFilter: 解析滤镜链失败:" << errbuf << " desc:" << filter_desc;
        close();
        return false;
    }

    ret = avfilter_graph_config(filter_graph, NULL);
    if (ret < 0) {
        char errbuf[128] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        qDebug() << "VideoFilter: 配置滤镜图失败:" << errbuf;
        close();
        return false;
    }

    is_inited = true;
    QString mode_str = (in_is_gpu && out_is_gpu) ? "全GPU(scale_cuda)" : 
                       (in_is_gpu && !out_is_gpu) ? "混合桥接(GPU解->hwdownload->CPU编)" : "纯CPU(scale)";
    qDebug() << "VideoFilter: 初始化成功! 模式:" << mode_str << " 滤镜描述:" << filter_desc;
    return true;
}

int VideoFilter::send_frame(AVFrame *frame)
{
    if (!is_inited || !buffersrc_ctx) {
        return AVERROR(EINVAL);
    }
    return av_buffersrc_add_frame_flags(buffersrc_ctx, frame, frame ? AV_BUFFERSRC_FLAG_KEEP_REF : 0);
}

int VideoFilter::receive_frame(AVFrame *frame)
{
    if (!is_inited || !buffersink_ctx || !frame) {
        return AVERROR(EINVAL);
    }
    return av_buffersink_get_frame(buffersink_ctx, frame);
}
