#include "lineitemwidget.h"
#include "./ui_lineitemwidget.h"

#include <QFileDialog>
LineItemWidget::LineItemWidget(QString filepath, QWidget *parent)
    : filepath(filepath), QWidget(parent),ui(new Ui::LineItemWidget)
{
    ui->setupUi(this);
    ui->label->setText(filepath);
}

LineItemWidget::~LineItemWidget()
{
    delete ui;
}

void LineItemWidget::setItemInfo(SourcecodeInfo info)
{
    this->info = info;
    hasInfo = true;
    ui->pushButton->setEnabled(true);
    ui->pushButton->setText("开始");
    on_pushButton_clicked();
}

void LineItemWidget::status(TaskStatus status)
{
    switch (status) {
    case TaskStatus::Waiting:
        ui->pushButton->setEnabled(false);
        ui->pushButton->setText("排队中...");
        break;
    case TaskStatus::Running:
        ui->pushButton->setEnabled(false);
        ui->pushButton->setText("转码中...");
        break;
    case TaskStatus::Finished:
        ui->pushButton->setEnabled(true);
        ui->pushButton->setText("完成");
        break;
    case TaskStatus::Error:
        ui->pushButton->setEnabled(true);
        ui->pushButton->setText("失败");
        break;
    }
}

void LineItemWidget::on_pushButton_2_clicked()
{
    emit delete_current_item();
}


void LineItemWidget::on_pushButton_clicked()//开始按钮
{
    if(ui->pushButton->text()=="完成")
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(save_path));
    }
    else{
        if(!hasInfo)
        {
            ui->pushButton->setEnabled(false);
            // 禁用按钮，防止用户重复狂点
            ui->pushButton->setText("解析中...");

            emit requestParseMetadata();
            // 发送信号请求主界面启动解析
            return;
        }
        Dialog dialog(info,this);

        connect(&dialog,&Dialog::taskDataReady,this,&LineItemWidget::onDialogDataReady);

        int ret = dialog.exec();
        if(ret == QDialog::Accepted)
        {
            qDebug()<<"格式选择完毕,准备开始......";
        }
        else if(ret == QDialog::Rejected)
        {
            qDebug()<<"用户取消";
        }
    }
}

void LineItemWidget::onDialogDataReady(const TranscodeTask &basicTask)
{
    save_path = basicTask.outputPath;
    emit send_task_info(this,basicTask);
}

void LineItemWidget::progress_updata(int taskId, int val)
{
    if (this->taskId == taskId) {
        this->ui->progressBar->setValue(val);
    }
}

