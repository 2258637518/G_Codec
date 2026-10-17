#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileDialog>
#include <QDirIterator>
#include <QMessageBox>
#include <QListWidgetItem>
#include <QDebug>

#include "task_item_widget.h"
#include "task_manager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_actionopen_triggered();
    void on_actionOpenFolder_triggered();
    void on_actionquit_triggered();
    void on_actionStartAll_triggered();
    void on_actionStopAll_triggered();
    void on_actionClearFinished_triggered();
    void on_actionBatchPreset_triggered();
    void on_actionAbout_triggered();

    void on_btnOpen_clicked();
    void on_btnOpenFolder_clicked();
    void on_btnBatchPreset_clicked();
    void on_btnStartAll_clicked();
    void on_btnStopAll_clicked();
    void on_btnClearFinished_clicked();
    void on_btnCenterSelect_clicked();

    void onMetadataReady(int taskId, SourcecodeInfo info);
    void onTaskStatusChanged(int taskId, TaskStatus status);
    void onTaskErrorOccurred(int taskId, const QString &title, const QString &detail);

private:
    Ui::MainWindow *ui;
    QStringList filePath;
    TaskManager *taskManager;

    int m_finishedCount = 0;
    int m_runningCount = 0;
    int m_errorCount = 0;

    void addFileToList(const QString &filePath);
    void addFolderToList(const QString &dirPath);
    void applyBatchPreset(const BatchPreset &preset);
    void updatePageBasedOnList();
    void updateStatusBar();
    QListWidgetItem* findItemByTaskId(int taskId);
};

#endif // MAINWINDOW_H
