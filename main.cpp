#include "mainwindow.h"
#include "dark_theme.h"
#include "transcode_task.h"
#include <QApplication>
#include <QIcon>
#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
}

#include "test_matrix.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 注册跨线程信号槽所需的元类型（杜绝 QObject::connect 跨线程投递失败警告）
    qRegisterMetaType<TaskStatus>("TaskStatus");
    qRegisterMetaType<SourcecodeInfo>("SourcecodeInfo");
    qRegisterMetaType<TranscodeTask>("TranscodeTask");

    // 检查是否进入命令行测试模式
    for (int i = 1; i < argc; ++i) {
        if (QString(argv[i]) == "--test-matrix") {
            QString samplePath = (i + 1 < argc) ? QString::fromLocal8Bit(argv[i + 1]) : "G:/my_project/G_Codec/test_sample.mp4";
            return runAutomatedTestMatrix(samplePath);
        }
        if (QString(argv[i]) == "--test-single" && i + 4 < argc) {
            return runSingleTranscodeTest(
                QString::fromLocal8Bit(argv[i + 1]),
                QString::fromLocal8Bit(argv[i + 2]),
                QString::fromLocal8Bit(argv[i + 3]),
                QString::fromLocal8Bit(argv[i + 4])
            );
        }
    }

    // 应用高性能现代化深色系 QSS 主题
    a.setStyleSheet(DarkTheme::getStyleSheet());

    // 设置全局应用程序与窗口图标
    a.setWindowIcon(QIcon(":/app_icon.ico"));

    MainWindow w;
    w.show();
    return a.exec();
}
