#ifndef TEST_MATRIX_H
#define TEST_MATRIX_H

#include <QString>
#include <QVector>
#include <QDebug>
#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <iostream>
#include <windows.h>

#include "control_center.h"
#include "transcode_task.h"

struct TestCombo {
    QString label;
    QString container;   // "mov", "mp4", "mkv", "webm", "ts", "flv", "avi"
    QString videoCodec;  // "prores_ks", "libx264", "libx265", "mjpeg", "libvpx-vp9", "mpeg4", "mpeg2video", "libsvtav1"
    QString audioCodec;  // "pcm_s16le", "pcm_s24le", "alac", "aac", "libmp3lame", "ac3", "libopus", "flac"
    int width = -1;
    int height = -1;
    double fps = -1.0;
};

inline int runAutomatedTestMatrix(const QString &inputSamplePath)
{
    // 绑定至父终端 (PowerShell / CMD) 以便实时输出测试信息
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE *fDummy;
        freopen_s(&fDummy, "CONOUT$", "w", stdout);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);
    }

    QString logFilePath = "G:/my_project/G_Codec/build/test_matrix_result.log";
    QFile logFile(logFilePath);
    logFile.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream logOut(&logFile);

    auto printMsg = [&](const QString &msg) {
        std::cout << msg.toLocal8Bit().constData() << std::flush;
        logOut << msg;
        logOut.flush();
    };

    printMsg("\n=======================================================\n");
    printMsg(" [G_Codec] 启动全自动化全格式转码矩阵测试 (Matrix Test)\n");
    printMsg(QString(" 测试输入源文件: %1\n").arg(inputSamplePath));
    printMsg("=======================================================\n\n");

    QVector<TestCombo> tests = {
        {"1. MOV + Apple ProRes (prores_ks) + PCM 16-bit (pcm_s16le)", "mov", "prores_ks", "pcm_s16le", -1, -1, -1.0},
        {"2. MOV + MJPEG (mjpeg) + PCM 16-bit (pcm_s16le)", "mov", "mjpeg", "pcm_s16le", -1, -1, -1.0},
        {"3. MOV + H.264 (libx264) + AAC (aac)", "mov", "libx264", "aac", -1, -1, -1.0},
        {"4. MOV + H.265 (libx265) + ALAC (alac)", "mov", "libx265", "alac", -1, -1, -1.0},
        {"5. MP4 + H.264 (libx264) + AAC (aac)", "mp4", "libx264", "aac", -1, -1, -1.0},
        {"6. MP4 + H.265 (libx265) + AC-3 (ac3)", "mp4", "libx265", "ac3", -1, -1, -1.0},
        {"7. MP4 + AV1 (libsvtav1) + AAC (aac)", "mp4", "libsvtav1", "aac", -1, -1, -1.0},
        {"8. MP4 + MPEG-4 (mpeg4) + MP3 (libmp3lame)", "mp4", "mpeg4", "libmp3lame", -1, -1, -1.0},
        {"9. MKV + VP9 (libvpx-vp9) + Opus (libopus)", "mkv", "libvpx-vp9", "libopus", -1, -1, -1.0},
        {"10. MKV + H.265 (libx265) + FLAC (flac)", "mkv", "libx265", "flac", -1, -1, -1.0},
        {"11. MKV + Apple ProRes (prores_ks) + PCM (pcm_s16le)", "mkv", "prores_ks", "pcm_s16le", -1, -1, -1.0},
        {"12. MKV + MPEG-2 (mpeg2video) + AC-3 (ac3)", "mkv", "mpeg2video", "ac3", -1, -1, -1.0},
        {"13. WebM + VP9 (libvpx-vp9) + Opus (libopus)", "webm", "libvpx-vp9", "libopus", -1, -1, -1.0},
        {"14. TS + MPEG-2 (mpeg2video) + AC-3 (ac3)", "ts", "mpeg2video", "ac3", -1, -1, -1.0},
        {"15. TS + H.264 (libx264) + AAC (aac)", "ts", "libx264", "aac", -1, -1, -1.0},
        {"16. FLV + H.264 (libx264) + AAC (aac)", "flv", "libx264", "aac", -1, -1, -1.0},
        {"17. AVI + MPEG-4 (mpeg4) + MP3 (libmp3lame)", "avi", "mpeg4", "libmp3lame", -1, -1, -1.0},
        {"18. AVI + MJPEG (mjpeg) + PCM (pcm_s16le)", "avi", "mjpeg", "pcm_s16le", -1, -1, -1.0},
        {"19. AVI + H.264 (libx264) + AC-3 (ac3)", "avi", "libx264", "ac3", -1, -1, -1.0}
    };

    QString outDir = "G:/my_project/G_Codec/build/test_matrix_outputs";
    QDir().mkpath(outDir);

    int passedCount = 0;
    int failedCount = 0;

    for (int i = 0; i < tests.size(); ++i) {
        const auto &tc = tests[i];
        QString outPath = QString("%1/out_%2_%3_%4.%5")
                              .arg(outDir)
                              .arg(i + 1)
                              .arg(tc.videoCodec)
                              .arg(tc.audioCodec)
                              .arg(tc.container);
        QFile::remove(outPath);

        printMsg(QString("[%1/%2] 正在测试: %3 ...\n")
                     .arg(i + 1)
                     .arg(tests.size())
                     .arg(tc.label));

        TranscodeTask task;
        task.inputPath = inputSamplePath;
        task.outputPath = outPath;
        task.videoCodec = tc.videoCodec;
        task.audioCodec = tc.audioCodec;
        task.videoBitrate = 4000000;
        task.audioBitrate = 192000;
        task.audioSampleRate = 48000;
        task.audioChannels = 2;
        task.width = tc.width;
        task.height = tc.height;
        task.frameRate = tc.fps;
        task.state = TaskStatus::Waiting;

        auto worker = std::make_unique<ControlCenter>(inputSamplePath, i + 1, task);

        bool metaReady = false;
        QObject::connect(worker.get(), &ControlCenter::metadataReady, [&](int, SourcecodeInfo) {
            metaReady = true;
        });
        QObject::connect(worker.get(), &ControlCenter::metadataError, [&](int) {
            metaReady = false;
        });

        // open() 是同步执行的底层解析
        worker->open();

        if (!metaReady) {
            printMsg("  -> [FAILED] 元数据解析失败！\n");
            failedCount++;
            continue;
        }

        // 重新同步配置参数并触发转码
        worker->set_task_info(task);

        QEventLoop loop;
        bool transcodeFinished = false;
        bool transcodeFailed = false;

        QObject::connect(worker.get(), &ControlCenter::finished, [&]() {
            transcodeFinished = true;
            loop.quit();
        });
        QObject::connect(worker.get(), &ControlCenter::metadataError, [&]() {
            transcodeFailed = true;
            loop.quit();
        });

        QTimer watchdog;
        watchdog.setSingleShot(true);
        QObject::connect(&watchdog, &QTimer::timeout, [&]() {
            printMsg("  -> [TIMEOUT] 转码超时！\n");
            loop.quit();
        });
        watchdog.start(10000); // 10秒看门狗超时

        worker->start();
        loop.exec();

        worker->stop();
        worker.reset(); // 确保安全析构释放所有线程

        QFileInfo outFi(outPath);
        bool hasVideo = false;
        bool hasAudio = false;
        if (outFi.exists() && outFi.size() > 512) {
            AVFormatContext *probeFmt = nullptr;
            if (avformat_open_input(&probeFmt, outPath.toUtf8().constData(), nullptr, nullptr) >= 0) {
                avformat_find_stream_info(probeFmt, nullptr);
                for (unsigned int s = 0; s < probeFmt->nb_streams; ++s) {
                    if (probeFmt->streams[s]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) hasVideo = true;
                    if (probeFmt->streams[s]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) hasAudio = true;
                }
                avformat_close_input(&probeFmt);
            }
        }

        bool inputHasAudio = false;
        AVFormatContext *inFmt = nullptr;
        if (avformat_open_input(&inFmt, inputSamplePath.toUtf8().constData(), nullptr, nullptr) >= 0) {
            avformat_find_stream_info(inFmt, nullptr);
            if (av_find_best_stream(inFmt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0) >= 0) {
                inputHasAudio = true;
            }
            avformat_close_input(&inFmt);
        }

        if (transcodeFinished && !transcodeFailed && hasVideo && (!inputHasAudio || hasAudio)) {
            printMsg(QString("  -> [PASS] 成功转码生成文件: 大小 %1 KB (流结构完备)\n")
                         .arg(outFi.size() / 1024));
            passedCount++;
        } else {
            printMsg(QString("  -> [FAILED] 转码失败或文件缺少有效流! (Finished=%1, Failed=%2, Size=%3, hasV=%4, hasA=%5, needA=%6)\n")
                         .arg(transcodeFinished)
                         .arg(transcodeFailed)
                         .arg(outFi.size())
                         .arg(hasVideo)
                         .arg(hasAudio)
                         .arg(inputHasAudio));
            failedCount++;
        }
    }

    printMsg("\n=======================================================\n");
    printMsg(QString(" 测试完成: 总计 %1 项, 通过: %2 项, 失败: %3 项\n")
                 .arg(tests.size())
                 .arg(passedCount)
                 .arg(failedCount));
    printMsg("=======================================================\n\n");

    return (failedCount == 0) ? 0 : 1;
}

inline int runSingleTranscodeTest(const QString &inputPath, const QString &outputPath, const QString &vCodec, const QString &aCodec)
{
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE *fDummy;
        freopen_s(&fDummy, "CONOUT$", "w", stdout);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);
    }

    std::cout << "\n[G_Codec Single Test] 开始单文件转码: " << inputPath.toLocal8Bit().constData() << " -> " << outputPath.toLocal8Bit().constData() << std::endl;

    QFile::remove(outputPath);

    TranscodeTask task;
    task.inputPath = inputPath;
    task.outputPath = outputPath;
    task.videoCodec = vCodec;
    task.audioCodec = aCodec;
    task.videoBitrate = 4000000;
    task.state = TaskStatus::Waiting;

    auto worker = std::make_unique<ControlCenter>(inputPath, 1, task);
    bool metaReady = false;
    QObject::connect(worker.get(), &ControlCenter::metadataReady, [&](int, SourcecodeInfo) {
        metaReady = true;
    });
    worker->open();
    if (!metaReady) {
        std::cout << "元数据解析失败!" << std::endl;
        return 1;
    }

    worker->set_task_info(task);
    QEventLoop loop;
    bool finished = false;
    int lastPct = -1;
    QObject::connect(worker.get(), &ControlCenter::progress_updata, [&](int pct) {
        if (pct != lastPct) {
            lastPct = pct;
            std::cout << "\r[转码进度] " << pct << "%" << std::flush;
        }
    });
    QObject::connect(worker.get(), &ControlCenter::finished, [&]() {
        finished = true;
        std::cout << "\n[成功] 转码流程全部顺利完成 (100%)!" << std::endl;
        loop.quit();
    });
    QObject::connect(worker.get(), &ControlCenter::transcodeError, [&](int, const QString &title, const QString &detail) {
        std::cout << "\n[收到详细错误诊断] 标题: " << title.toLocal8Bit().constData() << "\n详情:\n" << detail.toLocal8Bit().constData() << std::endl;
    });
    QObject::connect(worker.get(), &ControlCenter::metadataError, [&]() {
        std::cout << "\n[失败] 转码过程中止!" << std::endl;
        loop.quit();
    });

    worker->start();
    loop.exec();
    worker->stop();
    worker.reset();
    return finished ? 0 : 1;
}

#endif // TEST_MATRIX_H
