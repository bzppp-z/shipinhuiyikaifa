#include "wechatdialog.h"
#include "ui_wechatdialog.h"
#include "ckernel.h"
#include<QMessageBox>
WeChatDialog::WeChatDialog(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::WeChatDialog)
{
    ui->setupUi(this);
    setWindowTitle("朋朋会议");

}

WeChatDialog::~WeChatDialog()
{
    delete ui;
}

void WeChatDialog::closeEvent(QCloseEvent *event)
{
    if( QMessageBox::question( this , "提示" , "是否关闭?" )
            == QMessageBox::Yes  )
    {
        //关闭
        //event->accept();
        Q_EMIT SIG_close();
    }else{
        //忽略
        event->ignore();
    }
}
//设置名字和头像
void WeChatDialog::SetInfo(QString name, int icon)
{
    ui->le_name->setText(name);
}

void WeChatDialog::on_pd_Joinroom_clicked()
{
    Q_EMIT SIG_joinroom();

}
void WeChatDialog::on_pd_Create_clicked()
{
    Q_EMIT SIG_Create();
}

