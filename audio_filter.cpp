#include "audio_filter.h"
#include <QDebug>

AudioFilter::AudioFilter() {}

AudioFilter::~AudioFilter()
{
    close();
}

void AudioFilter::close()
{
    if (filter_graph) {
        avfilter_graph_free(&filter_graph);
        filter_graph = nullptr;
    }
    buffersrc_ctx = nullptr;
    buffersink_ctx = nullptr;
    is_inited = false;
}

bool AudioFilter::init(const AudioFilterParams &in_params,
                       const AudioFilterParams &out_params,
                       int target_frame_size,
                       const QString &extra_filters)
{
    close();

    filter_graph = avfilter_graph_alloc();
    if (!filter_graph) {
        qDebug() << "AudioFilter: avfilter_graph_alloc 失败";
        return false;
    }

    const AVFilter *abuffer = avfilter_get_by_name("abuffer");
    const AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
    if (!abuffer || !abuffersink) {
        qDebug() << "AudioFilter: 找不到 abuffer 或 abuffersink 滤镜";
        close();
        return false;
    }

    // 1. 创建 abuffer 输入滤镜上下文
    char in_ch_layout_str[128] = {0};
    av_channel_layout_describe(&in_params.ch_layout, in_ch_layout_str, sizeof(in_ch_layout_str));

    char args[512] = {0};
    snprintf(args, sizeof(args),
             "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%s",
             in_params.time_base.num > 0 ? in_params.time_base.num : 1,
             in_params.time_base.den > 0 ? in_params.time_base.den : in_params.sample_rate,
             in_params.sample_rate,
             av_get_sample_fmt_name(in_params.sample_fmt),
             in_ch_layout_str);

    int ret = avfilter_graph_create_filter(&buffersrc_ctx, abuffer, "in", args, NULL, filter_graph);
    if (ret < 0) {
        char errbuf[128] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        qDebug() << "AudioFilter: 创建 abuffer 失败:" << errbuf << " args:" << args;
        close();
        return false;
    }

    // 2. 创建 abuffersink 输出滤镜上下文
    ret = avfilter_graph_create_filter(&buffersink_ctx, abuffersink, "out", NULL, NULL, filter_graph);
    if (ret < 0) {
        char errbuf[128] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        qDebug() << "AudioFilter: 创建 abuffersink 失败:" << errbuf;
        close();
        return false;
    }

    // 3. 构建滤镜链 (aformat 转换格式/采样率/声道, asetnsamples 严格固定输出样本数并静音 padding 补齐)
    char out_ch_layout_str[128] = {0};
    av_channel_layout_describe(&out_params.ch_layout, out_ch_layout_str, sizeof(out_ch_layout_str));

    int frame_size = (target_frame_size > 0) ? target_frame_size : 1024;

    QString filter_desc = QString("aformat=sample_fmts=%1:sample_rates=%2:channel_layouts=%3,asetnsamples=n=%4:p=1")
                              .arg(av_get_sample_fmt_name(out_params.sample_fmt))
                              .arg(out_params.sample_rate)
                              .arg(out_ch_layout_str)
                              .arg(frame_size);

    if (!extra_filters.isEmpty()) {
        filter_desc = extra_filters + "," + filter_desc;
    }

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
        qDebug() << "AudioFilter: 解析滤镜链失败:" << errbuf << " desc:" << filter_desc;
        close();
        return false;
    }

    ret = avfilter_graph_config(filter_graph, NULL);
    if (ret < 0) {
        char errbuf[128] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        qDebug() << "AudioFilter: 配置滤镜图失败:" << errbuf;
        close();
        return false;
    }

    is_inited = true;
    qDebug() << "AudioFilter: 初始化成功! 滤镜描述:" << filter_desc;
    return true;
}

int AudioFilter::send_frame(AVFrame *frame)
{
    if (!is_inited || !buffersrc_ctx) {
        return AVERROR(EINVAL);
    }
    return av_buffersrc_add_frame_flags(buffersrc_ctx, frame, frame ? AV_BUFFERSRC_FLAG_KEEP_REF : 0);
}

int AudioFilter::receive_frame(AVFrame *frame)
{
    if (!is_inited || !buffersink_ctx || !frame) {
        return AVERROR(EINVAL);
    }
    return av_buffersink_get_frame(buffersink_ctx, frame);
}
