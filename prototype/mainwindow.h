#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMouseEvent> // 必须包含鼠标事件头文件
#include <QFileDialog>  // 必须包含文件对话框头文件
#include <QDebug>       // 用于打印调试信息
#include <QDragEnterEvent> // 拖拽进入事件
#include <QDropEvent>      // 释放放置事件
#include <QMimeData>       // 用于数据类型解析
#include <QMimeDatabase>
#include <QMimeType>
#include <QStringList>
#include <QFileIconProvider>
#include <QMessageBox>
#include <QListWidgetItem>

#include "lineitemwidget.h"
#include "taskmanager.h"



QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    void dragEnterEvent(QDragEnterEvent *event)override;
    void dropEvent(QDropEvent *event)override;
    void closeEvent(QCloseEvent *event) override;
private slots:
    void on_actionopen_triggered();
    void on_actionquit_triggered();
    void onMetadataReady(int taskId, SourcecodeInfo info);


private:
    Ui::MainWindow *ui;
    QStringList videoMimeTypes;
    QStringList filePath;
    TaskManager *taskManager;

    void addFileToList(const QString &filePath);
    void updatePageBasedOnList();
    QListWidgetItem* findItemByTaskId(int taskId);

signals:

};
#endif // MAINWINDOW_H
