<div align="center">

<img src="app_icon.png" width="128" height="128" alt="G-Codec Icon" />

# G-Codec Pro
### 高性能硬件加速音视频转码引擎 · High-Performance Hardware-Accelerated Transcoding Engine

[![Language](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=c%2B%2B)](https://isocpp.org/)
[![Framework](https://img.shields.io/badge/Qt-6.8%20%7C%205.15-41CD52?logo=qt)](https://www.qt.io/)
[![Engine](https://img.shields.io/badge/FFmpeg-7.x%20Full%20Build-007808?logo=ffmpeg)](https://ffmpeg.org/)
[![Acceleration](https://img.shields.io/badge/Hardware-NVIDIA%20NVDEC%20%2B%20NVENC-76B900?logo=nvidia)](https://developer.nvidia.com/video-encode-decode-gpu-support-matrix)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-0078D6?logo=windows)](https://microsoft.com)
[![License](https://img.shields.io/badge/License-GPL%20v3.0-blue.svg)](LICENSE)

<p align="center">
  <b>全链路 GPU 零拷贝 · 现代黑曜石深色 UI · 智能批量预设 · 工业级 19 组格式测试全覆盖</b>
</p>

</div>

---

## 💡 项目初衷与致开发者 (A Word from Author)

> 开发 **G-Codec** 的初衷，是为了系统化掌握并巩固 FFmpeg 全链路软硬件编解码与多线程同步机制。若这个项目能对正在音视频领域钻研的开发者有所启发或帮助，便是我最大的欣慰。
>
> 项目的核心音视频解码管线、GPU 零拷贝桥接、多格式编码器适配与线程安全队列均由我主导推导设计。在此期间，通过查阅技术资料并与 AI 结对编程，不仅补齐了现代黑曜石深色 UI 交互，更对早期的代码架构与命名进行了工业级重构，最终打磨出了这个兼具性能与可用性的版本。欢迎交流探讨！
>
> 📁 **原版纯手写原型归档**：
> 我将最初未经过 AI 修改与重构的原生核心源码完整保留在了 [`prototype/`](./prototype/) 目录：  
> *“原版简单Qt默认界面文件，核心齐全，有小bug，但是懒得修---”*  
> 欢迎音视频学习者翻阅对比，直观感受底层管线从最初打通到工程化落地的演进过程。

---

## 🌟 核心特性 (Key Features)

- ⚡ **全链路 GPU 零拷贝硬件加速**：
  基于 `NVIDIA NVDEC` 硬件解码 ➔ `scale_cuda` 显存硬件滤镜 ➔ `NVENC` 硬件编码，音视频流数据全程驻留 GPU GDDR6 显存，无需 CPU 介入 host-to-device 内存来回搬运，压制 4K 60fps 蓝光视频仅需数秒。
- 🔄 **工业级混合降级安全通道 (Hybrid Fallback)**：
  遇到非标准奇数分辨率或老旧编码时，GPU 管线自动通过格式协商安全桥接至 CPU 多线程软编软解，全核心并行榨干多核性能，杜绝转码中途崩溃闪退。
- 🛡️ **双条件变量有界阻塞队列架构**：
  基于 C++ 标准库 `std::mutex` 与 `std::condition_variable` 设计生产者-消费者背压环形缓冲区，严格限制显存帧与包队列深度，彻底消灭 CPU 忙等空转与内存爆仓风险。
- 🚀 **智能批量转码预设中心 (Batch Presets)**：
  支持一键套用热门预设（高速压制、蓝光高清、影视母带 ProRes、网络免专利 WebM、纯音频提取等），自动生成防覆盖输出路径，彻底告别逐个视频弹窗配置的繁琐体验。
- 🩺 **结构化全链路错误诊断**：
  底层将 FFmpeg 晦涩的错误码（如 `-22`, `AVERROR(ENOSPC)`）自动转译为通俗原因与针对性排查建议，支持悬停 Tooltip 预览与一键打开诊断弹窗。
- 🎨 **黑曜石现代极简美学 (Obsidian Dark)**：
  深度适配 Windows 11 / 10 原生 DWM 沉浸式深色标题栏与边框，配合磨砂玻璃质感与专属商业级 Squircle 图标，视觉体验克制、协调、专业。

---

## 🏗️ 底层处理管线架构 (Architecture)

```
[源媒体文件] (MP4/MKV/MOV/WebM/TS/FLV/AVI)
     │
     ▼
┌──────────────┐
│   Demuxer    │ ➔ 异步多流解封装
└──────────────┘
     │
     ├── AVPacket 队列 (有界阻塞缓冲)
     ▼
┌──────────────┐
│ VideoDecoder │ ➔ NVDEC 硬件解码 (直接解码到 CUDA 显存 Frame)
└──────────────┘
     │
     ├── AVFrame 显存帧 (Zero-Copy)
     ▼
┌──────────────┐
│ VideoFilter  │ ➔ scale_cuda 硬件滤镜 (动态按需跨线程桥接)
└──────────────┘
     │
     ├── AVFrame 目标画幅
     ▼
┌──────────────┐
│ VideoEncoder │ ➔ NVENC 硬件编码 (h264_nvenc / hevc_nvenc)
└──────────────┘
     │
     ├── AVPacket 目标流 (有界缓冲)
     ▼
┌──────────────┐
│    Muxer     │ ➔ 容器时间基对齐与原子写入 ➔ [目标高清媒体]
└──────────────┘
```

---

## 📊 全格式/编码器支持矩阵 (19/19 Fully Passing)

经过自动化测试套件严格双流校验（音视频流双通道验证）：

| 序号 | 容器格式 | 视频编码器 | 音频编码器 | 硬件加速通道 | 典型应用场景 |
| :---: | :---: | :---: | :---: | :---: | :--- |
| **01** | **MOV** | `prores_ks` | `pcm_s16le` | CPU 软编 | Apple ProRes 422 影视后期剪辑母带 |
| **02** | **MOV** | `mjpeg` | `pcm_s16le` | CPU 软编 | 逐帧高保真图像采集序列 |
| **03** | **MOV** | `libx264` / `h264_nvenc` | `aac` | ⚡ **NVENC 零拷贝** | Apple QuickTime 通用压制 |
| **04** | **MOV** | `libx265` / `hevc_nvenc` | `alac` | ⚡ **NVENC 零拷贝** | Apple Lossless 高清无损音频封装 |
| **05** | **MP4** | `libx264` / `h264_nvenc` | `aac` | ⚡ **NVENC 零拷贝** | 全球最通用流媒体格式（默认推荐） |
| **06** | **MP4** | `libx265` / `hevc_nvenc` | `ac3` | ⚡ **NVENC 零拷贝** | 杜比环绕声 4K 超清压制 |
| **07** | **MP4** | `libsvtav1` | `aac` | CPU 软编 | 下一代免版税高效压缩 |
| **08** | **MP4** | `mpeg4` | `libmp3lame` | CPU 软编 | 传统播放器与车载设备兼容 |
| **09** | **MKV** | `libvpx-vp9` | `libopus` | CPU 软编 | 高保真网络开源流媒体 |
| **10** | **MKV** | `libx265` / `hevc_nvenc` | `flac` | ⚡ **NVENC 零拷贝** | 蓝光原盘压制 + Hi-Res 无损母带 |
| **11** | **MKV** | `prores_ks` | `pcm_s16le` | CPU 软编 | MKV 影视母带级存档 |
| **12** | **MKV** | `mpeg2video` | `ac3` | CPU 软编 | 广播级多声道备份 |
| **13** | **WebM** | `libvpx-vp9` | `libopus` | CPU 软编 | YouTube / HTML5 网页标准封装 |
| **14** | **TS** | `mpeg2video` | `ac3` | CPU 软编 | 广电电视切片与卫星广播标准 |
| **15** | **TS** | `libx264` / `h264_nvenc` | `aac` | ⚡ **NVENC 零拷贝** | HLS / IPTV 广播直播流 |
| **16** | **FLV** | `libx264` / `h264_nvenc` | `aac` | ⚡ **NVENC 零拷贝** | RTMP 直播推流经典容器 |
| **17** | **AVI** | `mpeg4` | `libmp3lame` | CPU 软编 | 传统多媒体兼容 |
| **18** | **AVI** | `mjpeg` | `pcm_s16le` | CPU 软编 | 工业相机原始采集 |
| **19** | **AVI** | `libx264` / `h264_nvenc` | `ac3` | ⚡ **NVENC 零拷贝** | AVI 现代高清增强封装 |

---

## ⚡ 快速开始 (Quick Start)

### 1. 下载即用便携版 (Portable Release)
从 [Releases](../../releases) 页面下载 `G-Codec-v1.0-Windows-x64.zip`，解压至任意目录，双击 `G_Codec.exe` 即可直接使用，无需安装任何前置运行库。

### 2. 本地源码编译 (Build from Source)

#### 环境要求
- Windows 10 / 11 64-bit
- MSVC 2022 (Visual Studio 2022 / Build Tools 17.x+)
- CMake 3.16+
- Qt 6.5+ (推荐 Qt 6.8 MSVC 64-bit)
- FFmpeg 6.x / 7.x SDK (包含 dev 开发库与 bin 运行库)

#### 构建命令
```powershell
# 1. 克隆代码仓库
git clone https://github.com/YourUsername/G-Codec.git
cd G-Codec

# 2. 生成 MSVC 工程
cmake -B build -S . -DCMAKE_PREFIX_PATH="E:/Qt/6.8.3/msvc2022_64" -DFFMPEG_DIR="G:/SDKs/ffmpeg"

# 3. 编译 Release 生产版本
cmake --build build --config Release
```

---

## 📄 开源许可证 (License)

本项目采用 [GNU General Public License v3.0 (GPLv3)](LICENSE) 许可协议开源。底层音视频处理依赖优秀的开源多媒体框架 [FFmpeg](https://ffmpeg.org/)。
