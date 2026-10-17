#ifndef DARK_THEME_H
#define DARK_THEME_H

#include <QString>

namespace DarkTheme {

inline QString getStyleSheet() {
    return R"(
/* ===================================================================
   G-Codec Studio - Professional Dark Theme (Obsidian Clean Design)
   =================================================================== */

/* 全局字体与重置 */
* {
    font-family: "Segoe UI Variable Text", "Segoe UI", "Microsoft YaHei UI", -apple-system, sans-serif;
    font-size: 13px;
}

/* 核心窗口背景 */
QMainWindow {
    background-color: #0b0c10;
}

QDialog {
    background-color: #0f1117;
}

#centralwidget {
    background-color: #0b0c10;
}

/* 核心修复：所有标签默认透明底色，严禁出现黑色矩形文本块 */
QLabel {
    background-color: transparent;
    color: #e2e8f0;
    border: none;
    padding: 0px;
}

/* 状态栏 */
QStatusBar {
    background-color: #08090c;
    color: #64748b;
    border-top: 1px solid #171a23;
    padding: 4px 16px;
    font-size: 12px;
}

/* 菜单栏 (若保留) */
QMenuBar {
    background-color: #0b0c10;
    color: #94a3b8;
    border-bottom: 1px solid #171a23;
    padding: 2px 6px;
    font-size: 12px;
}

QMenuBar::item {
    background: transparent;
    padding: 4px 10px;
    border-radius: 4px;
}

QMenuBar::item:selected {
    background-color: #171a23;
    color: #38bdf8;
}

QMenu {
    background-color: #131620;
    color: #e2e8f0;
    border: 1px solid #222736;
    border-radius: 8px;
    padding: 6px;
}

QMenu::item {
    padding: 6px 20px 6px 12px;
    border-radius: 4px;
    margin: 1px 0px;
}

QMenu::item:selected {
    background-color: #0284c7;
    color: #ffffff;
}

QMenu::separator {
    height: 1px;
    background-color: #222736;
    margin: 4px 6px;
}

/* ==================== 统一按钮系统 ==================== */
QPushButton {
    background-color: #161922;
    color: #cbd5e1;
    border: 1px solid #252b3b;
    border-radius: 6px;
    padding: 6px 14px;
    font-weight: 500;
    font-size: 12px;
    min-height: 20px;
}

QPushButton:hover {
    background-color: #1f2330;
    border-color: #38bdf8;
    color: #ffffff;
}

QPushButton:pressed {
    background-color: #10121a;
    border-color: #0284c7;
}

QPushButton:disabled {
    background-color: #101217;
    color: #475569;
    border-color: #171a22;
}

/* 主操作按钮 (Primary Electric Blue) */
QPushButton[primary="true"] {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                stop:0 #0284c7, stop:1 #0ea5e9);
    color: #ffffff;
    border: 1px solid #38bdf8;
    font-weight: 600;
    border-radius: 6px;
}

QPushButton[primary="true"]:hover {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                stop:0 #0369a1, stop:1 #0284c7);
    border-color: #7dd3fc;
    color: #ffffff;
}

QPushButton[primary="true"]:pressed {
    background-color: #075985;
}

/* 成功状态按钮 (Success Green) */
QPushButton[success="true"] {
    background-color: #064e3b;
    color: #34d399;
    border: 1px solid #059669;
    font-weight: 600;
    border-radius: 6px;
}

QPushButton[success="true"]:hover {
    background-color: #047857;
    border-color: #10b981;
    color: #ffffff;
}

/* 危险/取消按钮 (Danger Red) */
QPushButton[danger="true"] {
    background-color: #241215;
    color: #f87171;
    border: 1px solid #451a20;
    font-weight: 600;
}

QPushButton[danger="true"]:hover {
    background-color: #3b161c;
    border-color: #ef4444;
    color: #ffffff;
}

/* ==================== 下拉框 (QComboBox) ==================== */
QComboBox {
    background-color: #131620;
    color: #f8fafc;
    border: 1px solid #232838;
    border-radius: 6px;
    padding: 6px 12px;
    min-height: 22px;
    font-size: 12px;
}

QComboBox:hover {
    border-color: #38bdf8;
    background-color: #171b26;
}

QComboBox:focus {
    border-color: #38bdf8;
}

QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 24px;
    border-left: none;
}

QComboBox::down-arrow {
    image: none;
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-top: 5px solid #64748b;
    margin-right: 10px;
}

QComboBox QAbstractItemView {
    background-color: #131620;
    color: #e2e8f0;
    border: 1px solid #262c3e;
    border-radius: 6px;
    selection-background-color: #0284c7;
    selection-color: #ffffff;
    padding: 4px;
}

/* ==================== 输入框 ==================== */
QLineEdit, QSpinBox {
    background-color: #131620;
    color: #f8fafc;
    border: 1px solid #232838;
    border-radius: 6px;
    padding: 6px 10px;
}

QLineEdit:focus, QSpinBox:focus {
    border-color: #38bdf8;
}

/* ==================== 进度条 ==================== */
QProgressBar {
    background-color: #131620;
    border: 1px solid #1f2433;
    border-radius: 3px;
    text-align: center;
    color: transparent;
    min-height: 6px;
    max-height: 6px;
}

QProgressBar::chunk {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                stop:0 #0284c7, stop:1 #38bdf8);
    border-radius: 3px;
}

/* ==================== 任务列表 ==================== */
QListWidget {
    background-color: transparent;
    border: none;
    padding: 4px;
}

QListWidget::item {
    background-color: transparent;
    border: none;
    padding: 0px;
    margin-bottom: 8px;
}

QListWidget::item:selected {
    background-color: transparent;
}

/* ==================== 分组面板 ==================== */
QGroupBox {
    background-color: #11131a;
    border: 1px solid #1e2230;
    border-radius: 8px;
    margin-top: 14px;
    padding: 16px 14px 14px 14px;
    font-weight: 600;
    font-size: 12px;
    color: #38bdf8;
}

QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 14px;
    padding: 0 6px;
    background-color: #0f1117;
}

/* ==================== 细条滚动条 ==================== */
QScrollBar:vertical {
    background-color: transparent;
    width: 6px;
    margin: 0;
    border-radius: 3px;
}

QScrollBar::handle:vertical {
    background-color: #222736;
    border-radius: 3px;
    min-height: 28px;
}

QScrollBar::handle:vertical:hover {
    background-color: #38bdf8;
}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0px;
}

QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    background: none;
}

/* ==================== 悬停提示 Tooltip ==================== */
QToolTip {
    background-color: #181b26;
    color: #f8fafc;
    border: 1px solid #2d3448;
    border-radius: 5px;
    padding: 5px 10px;
    font-size: 12px;
}
)";
}

} // namespace DarkTheme

#endif // DARK_THEME_H
