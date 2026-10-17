#include "video_encoder.h"
#include <QDebug>

VideoEncoder::VideoEncoder(TranscodeTask &task_info,
                           SaveQueue<AVFrame*>& decode_video_frame,
                           SaveQueue<AVPacket*>& pack_video_frame)
    : task_info(task_info),
      decode_video_frame(decode_video_frame),
      pack_video_frame(pack_video_frame),
      video_filter(std::make_unique<VideoFilter>())
{
}

VideoEncoder::~VideoEncoder()
{
    if (codec_ctx) {
        avcodec_free_context(&codec_ctx);
        codec_ctx = nullptr;
    }
    if(hw_device_ctx){
        av_buffer_unref(&hw_device_ctx);
        hw_device_ctx = nullptr;
    }
    if(hw_frames_ctx){
        av_buffer_unref(&hw_frames_ctx);
        hw_frames_ctx = nullptr;
    }
}

bool VideoEncoder::init(int id,
                        AVPixelFormat in_pix_fmt,
                        int in_w,
                        int in_h,
                        AVRational in_time_base,
                        AVRational in_frame_rate,
                        AVRational in_sar,
                        AVBufferRef *hw_device_ctx)
{
    video_stream_id = id;
    this->in_time_base = in_time_base;
    this->in_frame_rate = in_frame_rate;
    this->in_sar = in_sar;
    this->in_pix_fmt = in_pix_fmt;
    this->filter_inited_with_real_frame = false;
    this->is_gpu_enc = false;

    // 1. 确定目标输出尺寸与像素格式
    int out_w = (task_info.width > 0) ? task_info.width : in_w;
    int out_h = (task_info.height > 0) ? task_info.height : in_h;

    // 查找编码器
    const AVCodec *codec = nullptr;

    QString target_name = task_info.videoCodec.trimmed().toLower();
    QString gpu_encoder_name;

    if (hw_device_ctx) {
        if (target_name.contains("264") || target_name.contains("avc")) {
            gpu_encoder_name = "h264_nvenc";
        } else if (target_name.contains("265") || target_name.contains("hevc")) {
            gpu_encoder_name = "hevc_nvenc";
        } else if (target_name.contains("av1")) {
            gpu_encoder_name = "av1_nvenc";
        }

        if (!gpu_encoder_name.isEmpty()) {
            codec = avcodec_find_encoder_by_name(gpu_encoder_name.toUtf8().constData());
            if (codec) {
                is_gpu_enc = true;
                qDebug() << "VideoEncoder: 成功匹配并启用 GPU 硬件编码器:" << codec->name;
            } else {
                qDebug() << "VideoEncoder: 未找到 GPU 编码器" << gpu_encoder_name << "，准备回退到 CPU 编码";
            }
        }
    }

    if (!codec) {
        if (!task_info.videoCodec.isEmpty()) {
            codec = avcodec_find_encoder_by_name(task_info.videoCodec.toUtf8().constData());
        }
        if (!codec) {
            codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        }
        is_gpu_enc = false;
        qDebug() << "VideoEncoder: 使用 CPU 软件编码器:" << (codec ? codec->name : "空");
    }

    if (!codec) {
        qDebug() << "VideoEncoder: 找不到指定的视频编码器:" << task_info.videoCodec;
        return false;
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        qDebug() << "VideoEncoder: 编码器上下文分配失败";
        return false;
    }

    bool same_res = (out_h == in_h && out_w == in_w);
    bool same_fps = (task_info.frameRate <= 0 || av_q2d(in_frame_rate) == task_info.frameRate);
    this->pass_filter = (hw_device_ctx != nullptr && is_gpu_enc && same_fps && same_res);
    qDebug() << "VideoEncoder: 是否跳过滤镜(Bypass):" << pass_filter;

    AVPixelFormat out_pix_fmt;

    if (hw_device_ctx) {
        this->hw_device_ctx = av_buffer_ref(hw_device_ctx);
    }

    if (is_gpu_enc) {
        codec_ctx->hw_device_ctx = av_buffer_ref(this->hw_device_ctx);
        codec_ctx->pix_fmt = AV_PIX_FMT_CUDA;

        // 构建 hw_frames_ctx 显存参数映射
        hw_frames_ctx = av_hwframe_ctx_alloc(this->hw_device_ctx);
        AVHWFramesContext *frames_ctx = (AVHWFramesContext *)(hw_frames_ctx->data);
        frames_ctx->format = AV_PIX_FMT_CUDA;
        frames_ctx->sw_format = AV_PIX_FMT_NV12; // NVDEC 底层原生大多为 NV12
        frames_ctx->width = out_w;
        frames_ctx->height = out_h;
        frames_ctx->initial_pool_size = 0;
        if (av_hwframe_ctx_init(hw_frames_ctx) < 0) {
            qDebug() << "VideoEncoder: av_hwframe_ctx_init 失败";
            return false;
        }
        codec_ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ctx);

        // 设置 NVENC 专用低延迟/高质量预设
        av_opt_set(codec_ctx->priv_data, "preset", "p4", 0); // p4 为平衡预设
        av_opt_set(codec_ctx->priv_data, "tune", "hq", 0);   // hq 高画质
    } else {
        out_pix_fmt = (task_info.out_pix_fmt != AV_PIX_FMT_NONE) ? task_info.out_pix_fmt : in_pix_fmt;
        if (out_pix_fmt == AV_PIX_FMT_CUDA || out_pix_fmt == AV_PIX_FMT_NONE) {
            out_pix_fmt = AV_PIX_FMT_YUV420P;
        }
        if (codec->pix_fmts) {
            bool supported = false;
            for (int i = 0; codec->pix_fmts[i] != AV_PIX_FMT_NONE; i++) {
                if (codec->pix_fmts[i] == out_pix_fmt) {
                    supported = true;
                    break;
                }
            }
            if (!supported) {
                out_pix_fmt = codec->pix_fmts[0];
            }
        }
        codec_ctx->pix_fmt = out_pix_fmt;
    }

    codec_ctx->width = out_w;
    codec_ctx->height = out_h;
    codec_ctx->bit_rate = (task_info.videoBitrate > 0) ? task_info.videoBitrate : 2000000;

    // 4. 帧率与时间基设置
    AVRational out_frame_rate;
    if (task_info.frameRate > 0) {
        out_frame_rate = av_d2q(task_info.frameRate, 1000000);
    } else if (in_frame_rate.num > 0 && in_frame_rate.den > 0) {
        out_frame_rate = in_frame_rate;
    } else {
        out_frame_rate = AVRational{30, 1};
    }

    codec_ctx->framerate = out_frame_rate;
    codec_ctx->time_base = av_inv_q(out_frame_rate);
    codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    codec_ctx->strict_std_compliance = FF_COMPLIANCE_UNOFFICIAL;
    if (!is_gpu_enc) {
        codec_ctx->thread_count = 0; // 自动榨干所有 CPU 核心并行编码
        codec_ctx->thread_type = FF_THREAD_SLICE | FF_THREAD_FRAME;
    }
    if (codec->id == AV_CODEC_ID_MJPEG && out_pix_fmt == AV_PIX_FMT_YUV420P) {
        codec_ctx->color_range = AVCOL_RANGE_MPEG;
    }

    // 5. 打开编码器
    int ret = avcodec_open2(codec_ctx, codec, nullptr);
    if (ret < 0 && is_gpu_enc) {
        char errbuf[128] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        qDebug() << "VideoEncoder: GPU 硬件编码器打开失败 (" << errbuf << ")，自动回退到 CPU 软件编码器...";

        // 释放当前 GPU 上下文与显存帧引用
        avcodec_free_context(&codec_ctx);
        if (hw_frames_ctx) {
            av_buffer_unref(&hw_frames_ctx);
            hw_frames_ctx = nullptr;
        }

        // 回退查找对应的 CPU 软件编码器
        codec = nullptr;
        if (!task_info.videoCodec.isEmpty()) {
            codec = avcodec_find_encoder_by_name(task_info.videoCodec.toUtf8().constData());
        }
        if (!codec) {
            codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        }
        is_gpu_enc = false;
        pass_filter = false;

        if (codec) {
            codec_ctx = avcodec_alloc_context3(codec);
            if (codec_ctx) {
                out_pix_fmt = (task_info.out_pix_fmt != AV_PIX_FMT_NONE) ? task_info.out_pix_fmt : in_pix_fmt;
                if (out_pix_fmt == AV_PIX_FMT_CUDA || out_pix_fmt == AV_PIX_FMT_NONE) {
                    out_pix_fmt = AV_PIX_FMT_YUV420P;
                }
                if (codec->pix_fmts) {
                    bool supported = false;
                    for (int i = 0; codec->pix_fmts[i] != AV_PIX_FMT_NONE; i++) {
                        if (codec->pix_fmts[i] == out_pix_fmt) {
                            supported = true;
                            break;
                        }
                    }
                    if (!supported) {
                        out_pix_fmt = codec->pix_fmts[0];
                    }
                }
                codec_ctx->pix_fmt = out_pix_fmt;
                codec_ctx->width = out_w;
                codec_ctx->height = out_h;
                codec_ctx->bit_rate = (task_info.videoBitrate > 0) ? task_info.videoBitrate : 2000000;
                codec_ctx->framerate = out_frame_rate;
                codec_ctx->time_base = av_inv_q(out_frame_rate);
                codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
                codec_ctx->strict_std_compliance = FF_COMPLIANCE_UNOFFICIAL;
                codec_ctx->thread_count = 0; // 自动调用全部核心
                codec_ctx->thread_type = FF_THREAD_SLICE | FF_THREAD_FRAME;
                if (codec->id == AV_CODEC_ID_MJPEG && out_pix_fmt == AV_PIX_FMT_YUV420P) {
                    codec_ctx->color_range = AVCOL_RANGE_MPEG;
                }
                ret = avcodec_open2(codec_ctx, codec, nullptr);
            }
        }
    }

    if (ret < 0 || !codec_ctx) {
        char errbuf[128] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        qDebug() << "VideoEncoder: 打开编码器最终失败:" << errbuf;
        return false;
    }

    // 6. 滤镜图初始化推迟至首帧消费时动态绑定 (确保 100% 继承解码帧真实的 hw_frames_ctx)
    this->filter_inited_with_real_frame = false;

    qDebug() << "VideoEncoder: 初始化成功! 目标编码器:" << codec->name
             << " 分辨率:" << out_w << "x" << out_h
             << " 帧率:" << static_cast<double>(out_frame_rate.num) / out_frame_rate.den
             << " 码率:" << codec_ctx->bit_rate;

    return true;
}

void VideoEncoder::start()
{
    qDebug() << "视频编码线程已启动......";

    AVPacket *pkt = av_packet_alloc();
    if (!pkt) {
        qDebug() << "VideoEncoder: Packet 内存分配失败";
        emit finished();
        return;
    }

    bool is_eof = false;

    while (!is_eof)
    {
        if (is_stopped) {
            qDebug() << "VideoEncoder 线程已请求停止";
            break;
        }

        // 1. 从解码队列取出待编码的视频帧 (显存帧或普通内存帧)
        AVFrame *src_frame = decode_video_frame.pop();

        if (is_stopped) {
            av_packet_free(&pkt);
            if (src_frame) av_frame_free(&src_frame);
            qDebug() << "VideoEncoder 线程已取消";
            return;
        }

        if (pass_filter && src_frame && src_frame->format != codec_ctx->pix_fmt) {
            qDebug() << "VideoEncoder: 检测到实际帧像素格式与编码器不一致，自动退出直通并使用 VideoFilter 桥接转换";
            pass_filter = false;
        }

        // ============================================================
        // 分支 A：直通模式 (pass_filter == true，零拷贝直接送入 NVENC)
        // ============================================================
        if (pass_filter)
        {
            bool is_flush_pkt = (src_frame == nullptr);

            if (src_frame) {
                // 1. 时间戳转换：将输入时间基换算至编码器时间基
                if (src_frame->pts != AV_NOPTS_VALUE) {
                    src_frame->pts = av_rescale_q(src_frame->pts, in_time_base, codec_ctx->time_base);
                }
                src_frame->pict_type = AV_PICTURE_TYPE_NONE;
            }

            // 2. 直接将显存帧发送给 NVENC 编码器 (若 src_frame 为 nullptr 则触发 Flush)
            int ret = avcodec_send_frame(codec_ctx, src_frame);
            if (src_frame) {
                av_frame_free(&src_frame);
            }

            if (ret < 0 && ret != AVERROR_EOF) {
                qDebug() << "VideoEncoder(直通): 发送帧到编码器失败，错误码:" << ret;
                break;
            }

            // 3. 提取所有已就绪的 AVPacket
            while (true) {
                ret = avcodec_receive_packet(codec_ctx, pkt);
                if (ret == AVERROR(EAGAIN)) {
                    break; // 需要更多输入帧
                } else if (ret == AVERROR_EOF) {
                    is_eof = true;
                    break;
                } else if (ret < 0) {
                    qDebug() << "VideoEncoder(直通): 接收编码 Packet 失败，错误码:" << ret;
                    is_eof = true;
                    break;
                }

                pkt->stream_index = video_stream_id;
                pack_video_frame.push(av_packet_clone(pkt));
                av_packet_unref(pkt);
            }

            // 4. 处理末尾冲刷：若收到 nullptr 哨兵，全力排空底层余包
            if (is_flush_pkt && !is_eof) {
                while (avcodec_receive_packet(codec_ctx, pkt) == 0) {
                    pkt->stream_index = video_stream_id;
                    pack_video_frame.push(av_packet_clone(pkt));
                    av_packet_unref(pkt);
                }
                is_eof = true;
            }
        }
        // ============================================================
        // 分支 B：滤镜模式 (!pass_filter，经过 VideoFilter 滤镜图处理)
        // ============================================================
        else
        {
            // 首次动态使用真实解码帧初始化滤镜图 (获取解码器真实的 hw_frames_ctx 与格式，杜绝上下文不匹配导致 -22)
            if (!filter_inited_with_real_frame) {
                if (src_frame == nullptr) {
                    is_eof = true;
                    break;
                }

                VideoFilterParams in_params;
                in_params.width = src_frame->width;
                in_params.height = src_frame->height;
                in_params.pix_fmt = (AVPixelFormat)src_frame->format;
                in_params.time_base = in_time_base;
                in_params.frame_rate = in_frame_rate;
                in_params.sample_aspect_ratio = (src_frame->sample_aspect_ratio.num > 0) ? src_frame->sample_aspect_ratio : in_sar;

                VideoFilterParams out_params;
                out_params.width = codec_ctx->width;
                out_params.height = codec_ctx->height;
                out_params.pix_fmt = (is_gpu_enc) ? AV_PIX_FMT_CUDA : codec_ctx->pix_fmt;
                out_params.time_base = codec_ctx->time_base;
                out_params.frame_rate = codec_ctx->framerate;
                out_params.sample_aspect_ratio = in_params.sample_aspect_ratio;

                if (!video_filter->init(in_params, out_params, "", hw_device_ctx, src_frame->hw_frames_ctx)) {
                    qDebug() << "VideoEncoder: 真实硬件帧初始化 VideoFilter 失败";
                    av_frame_free(&src_frame);
                    break;
                }
                filter_inited_with_real_frame = true;
            }

            // 2. 发送给滤镜 (若 src_frame 为 nullptr，则触发滤镜的 Flush 冲刷)
            int send_ret = video_filter->send_frame(src_frame);
            if (src_frame) {
                av_frame_free(&src_frame);
            }

            if (send_ret < 0) {
                qDebug() << "VideoEncoder: 发送帧至滤镜失败，错误码:" << send_ret;
                break;
            }

            // 3. 从滤镜拉取规整好的标准视频帧并送入编码器
            while (true)
            {
                AVFrame *filt_frame = av_frame_alloc();
                int ret = video_filter->receive_frame(filt_frame);

                if (ret == AVERROR(EAGAIN)) {
                    av_frame_free(&filt_frame);
                    break; // 需要更多输入帧给滤镜
                } else if (ret == AVERROR_EOF) {
                    av_frame_free(&filt_frame);
                    is_eof = true;
                    break;
                } else if (ret < 0) {
                    qDebug() << "VideoEncoder: 从滤镜接收视频帧失败，错误码:" << ret;
                    av_frame_free(&filt_frame);
                    is_eof = true;
                    break;
                }

                // 4. 将规整好的帧送入视频编码器
                if (filt_frame->pts != AV_NOPTS_VALUE) {
                    filt_frame->pts = av_rescale_q(filt_frame->pts, video_filter->get_time_base(), codec_ctx->time_base);
                }
                filt_frame->pict_type = AV_PICTURE_TYPE_NONE;
                ret = avcodec_send_frame(codec_ctx, filt_frame);
                av_frame_free(&filt_frame);

                if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                    qDebug() << "VideoEncoder: 发送帧到编码器失败，错误码:" << ret;
                    is_eof = true;
                    break;
                }

                // 5. 循环提取编码好的 AVPacket
                while (true) {
                    ret = avcodec_receive_packet(codec_ctx, pkt);
                    if (ret == AVERROR(EAGAIN)) {
                        break;
                    } else if (ret == AVERROR_EOF) {
                        is_eof = true;
                        break;
                    } else if (ret < 0) {
                        qDebug() << "VideoEncoder: 接收编码 Packet 失败，错误码:" << ret;
                        is_eof = true;
                        break;
                    }

                    pkt->stream_index = video_stream_id;
                    pack_video_frame.push(av_packet_clone(pkt));
                    av_packet_unref(pkt);
                }
            }
        }
    }

    if (!is_stopped)
    {
        // 6. 冲刷编码器自身的余包 (Flush 编码器)
        avcodec_send_frame(codec_ctx, nullptr);
        while (avcodec_receive_packet(codec_ctx, pkt) == 0) {
            pkt->stream_index = video_stream_id;
            pack_video_frame.push(av_packet_clone(pkt));
            av_packet_unref(pkt);
        }

        // 资源清理与向下游（封装 Muxer 线程）传递哨兵
        pack_video_frame.push(nullptr); // 告知 Muxer 线程：视频流全部编码结束
    }

    av_packet_free(&pkt);
    qDebug() << "视频编码线程安全退出。";
    emit finished();
}

void VideoEncoder::stop()
{
    is_stopped = true;
}

void VideoEncoder::pause()
{
    qDebug() << "VideoEncoder::pause";
}
