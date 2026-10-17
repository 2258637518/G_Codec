# G-Codec 原始手写核心原型 (Raw Prototype)

> ### 📌 原版说明
> **“原版简单Qt默认界面文件，核心齐全，有小bug，但是懒得修---”**

---

## 📖 归档背景

本目录完整封存了作者在系统化自学与攻关 FFmpeg 音视频全链路编解码时，**纯手工编写的第一版原始实现代码**。

本目录的代码未经过后续 AI 对类名规范、接口重构与深色界面的现代工程化改造，完完整整地保留了作者当初最初的设计思路、原始命名、函数结构以及真实的调试轨迹。

如果你也在自学音视频开发或 FFmpeg，欢迎翻阅此处的源码。你可以对照根目录重构后的工程化版本（`src/`、`include/`），直观观察一个音视频项目从**“从零摸索打通核心流程的原型”**到**“工业级规范应用”**的演进全过程。

---

## 🗂️ 核心模块与文件对照

| 原始文件 | 模块职责 | 现版本（发布版）对应 |
| :--- | :--- | :--- |
| `control_center.cpp / .h` | 全链路总控调度中枢，跨线程信号串联 | `src/core/transcode_engine.cpp` |
| `demuxer.cpp / .h` | 异步解复用器，读取多流数据包 | `src/core/demuxer.cpp` |
| `decoder.cpp / .h` | 音视频解码器，支持软解与 GPU 硬件加速解码 | `src/core/decoder.cpp` |
| `video_filter.cpp / .h` | 基于 AVFilterGraph 的视频滤镜（Scale / FPS / CUDA零拷贝） | `src/core/video_filter.cpp` |
| `audio_filter.cpp / .h` | 基于 AVFilterGraph 的音频滤镜（重采样 / 声道重映射） | `src/core/audio_filter.cpp` |
| `videoencoder.cpp / .h` | 视频编码器（H.264 / HEVC / AV1 / NVENC / ProRes） | `src/core/video_encoder.cpp` |
| `audioencoder.cpp / .h` | 音频编码器（AAC / MP3 / Opus / FLAC / PCM） | `src/core/audio_encoder.cpp` |
| `muxer.cpp / .h` | 混流复用器，交错写入 AVPacket 生成目标容器 | `src/core/muxer.cpp` |
| `savequeue.h` | 原始有界条件变量阻塞队列，控制显存背压与平滑缓冲 | `include/concurrent_queue.h` |
| `taskmanager.cpp / .h` | 任务管理器，负责多任务并发调度与生命周期管理 | `src/core/task_manager.cpp` |
| `dialog.cpp / .h / .ui` | 原版原生 Qt 参数配置弹窗 | `src/ui/transcode_settings_dialog.*` |
| `lineitemwidget.*` | 原版转码任务项列表控件 | `src/ui/task_item_widget.*` |
| `mainwindow.*` | 原版 Qt 默认主窗口与基础布局 | `src/ui/mainwindow.*` (现代黑曜石深色 UI) |
| `简要日志` | **作者最初的真实研发便签与踩坑日志**（见下方） | - |

---

## 📝 原始研发足迹（节选自《简要日志》）

以下为最初从 7 月至 10 月自学与调试时，每一阶段攻克技术节点时的真实记录：

```text
7.12 实现：实现了任务添加功能和 dialog 获取元数据界面以及 taskid 绑定
7.24 实现：优化了并发结构和线程控制，解决了取消任务时的 bug
7.28 实现：编写可复用模板队列，采用 worker-thread 结构来进行编解码的架构设计，初步完成 demuxer 解封装
8.03 实现：修复信号连接问题，可以正常启动 FFmpeg 的解封装流程
8.05 实现：完成了 decoder 类的编写，实现了解码，并且正确连接信号
8.06 规划：编码环节有关视频分辨率、音频采样率、声道数改变需要用到 sws/swr，开始编写 videoencoder 与 audioencoder
8.10 实现：完成了 videoencoder 的主要循环函数与初始化代码编写
8.13 实现：使用 FilterGraph 封装 AudioFilter 与 VideoFilter 独立模块，重构 Encoder，统一处理分辨率缩放、帧率转换与时间戳重构
8.16 实现：完成了 Muxer 类的设计与编写，将音视频编码队列重构为单一共享队列，实现时间基换算与交错写入
8.18 实现：完成 Muxer 信号连接，全链路打通！能够正常输出可以播放的目标视频文件
8.19 实现：添加了进度条功能，按照 1% 的精度实时反馈转码进度
8.27 实现：初步解决取消卡死问题，修复 abort 导致队列 nullptr 冲突
9.01 实现：重构 SaveQueue 为双条件变量有界阻塞队列，消除 CPU 无效空转
9.03 实现：全面打通全链路 GPU 硬件加速流水线（NVDEC 硬解 -> scale_cuda 显存滤镜 -> NVENC 硬件编码），实现 GDDR6 显存零拷贝
10.17 整理：使用 AI 重构规范命名与接口，制作现代化深色界面，打造完整可用的正式发布版本
```
