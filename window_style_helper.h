#ifndef WINDOW_STYLE_HELPER_H
#define WINDOW_STYLE_HELPER_H

#include <QWidget>
#include <QColor>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif
#ifndef DWMWA_TEXT_COLOR
#define DWMWA_TEXT_COLOR 36
#endif

namespace WindowStyleHelper {

/**
 * @brief 应用 Windows 原生 DWM 沉浸式深色窗口框架
 * 保留 Aero Snap、多屏缩放、系统动画等全部原生特性，且零 CPU 开销地将原生边框与标题栏与 UI 深度融合
 */
inline void applyDarkWindowFrame(QWidget *window,
                                 QColor captionColor = QColor(11, 12, 16),
                                 QColor textColor = QColor(226, 232, 240),
                                 QColor borderColor = QColor(23, 26, 35))
{
    if (!window) return;
    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    if (!hwnd) return;

    // 1. 启用沉浸式深色模式标题栏 (兼容 Win10 1809+ / 20H1+ 及 Win11)
    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
    DwmSetWindowAttribute(hwnd, 19 /* 早期 Windows 10 属性常量 */, &darkMode, sizeof(darkMode));

    // 2. 自定义标题栏底色 (Win11 22000+) - 与主题 #0b0c10 严丝合缝
    COLORREF dwmCaption = RGB(captionColor.red(), captionColor.green(), captionColor.blue());
    DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &dwmCaption, sizeof(dwmCaption));

    // 3. 自定义标题文字颜色
    COLORREF dwmText = RGB(textColor.red(), textColor.green(), textColor.blue());
    DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &dwmText, sizeof(dwmText));

    // 4. 自定义窗口细边框颜色 - 统一为 #171a23 碳素灰
    COLORREF dwmBorder = RGB(borderColor.red(), borderColor.green(), borderColor.blue());
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &dwmBorder, sizeof(dwmBorder));
}

} // namespace WindowStyleHelper

#else
namespace WindowStyleHelper {
inline void applyDarkWindowFrame(QWidget *window,
                                 QColor captionColor = QColor(11, 12, 16),
                                 QColor textColor = QColor(226, 232, 240),
                                 QColor borderColor = QColor(23, 26, 35)) {}
}
#endif

#endif // WINDOW_STYLE_HELPER_H
