#include "logindialog.h"
#include "ui_logindialog.h"

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    setWindowTitle("朋朋会议");      // ← 加这句
    ui->tw_page->setCurrentIndex(0);//第一时间显示注册
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::closeEvent(QCloseEvent *)
{
    Q_EMIT SIG_close();
}
#include<QRegExp>
#include<QMessageBox>
//提交登录信息
void LoginDialog::on_pd_commit_clicked()
{
QString strTel=ui->le_tel->text();
QString strPassword=ui->le_password->text();
//校验
//检查去掉空格是否为空
QString tel=strTel,pass=strPassword;
tel.remove(" ");
pass.remove(" ");
if(tel.isEmpty()||pass.isEmpty())
{
    QMessageBox::about(this, "提示","账号或者密码不能为空");
    return;
}
//1.校验手机号 正则表达式 8-11
QRegExp reg("^1[3-8][0-9]\{6,9\}$");
bool res=reg.exactMatch(strTel);
if(!res)
{
 //不匹配
    QMessageBox::about(this, "提示","账号应该位8-11位，并且1开头3-8次开头");
    return;
}

//2.校验密码不能位20以上
if(strPassword.length()>20)
{
    QMessageBox::about(this, "提示","密码过长");
    return;
}
Q_EMIT SIG_loginCommit(strTel,strPassword);

}

//清空登录信息
void LoginDialog::on_pb_clear_clicked()
{

ui->le_password->setText("");
ui->le_tel->setText("");
}

//清空注册信息
void LoginDialog::on_pb_clear_rigister_clicked()
{
    ui->le_name_rigister->setText("");
    ui->le_password_rigister->setText("");
    ui->le_password_rigister_2->setText("");
    ui->le_tel_rigister->setText("");
}

//提交注册信息
void LoginDialog::on_pd_commit_rigister_clicked()
{

    QString strTel=ui->le_tel_rigister->text();
    QString strPassword1=ui->le_password_rigister->text();
    QString strPassword2=ui->le_password_rigister_2->text();
    QString name=ui->le_name_rigister->text();
    //校验
    //检查去掉空格是否为空
    QString tel=strTel,pass1=strPassword1,pass2=strPassword2,name1=name;
    tel.remove(" ");
    pass1.remove(" ");
    pass2.remove(" ");
    name1.remove("");
    if(tel.isEmpty()||pass1.isEmpty()||pass2.isEmpty()||name1.isEmpty())
    {
        QMessageBox::about(this, "提示","账号或者密码或者名字不能为空");
        return;
    }
    //1.校验手机号 正则表达式 8-11
    QRegExp reg("^1[3-8][0-9]\{6,9\}$");
    bool res=reg.exactMatch(strTel);
    if(!res)
    {
     //不匹配
        QMessageBox::about(this, "提示","账号应该位8-11位，并且1开头3-8次开头");
        return;
    }
    //2.校验密码不能位20以上
    if(pass1.length()>20||pass2.length()>20)
    {
        QMessageBox::about(this, "提示","密码过长");
        return;
    }
    //检验密码相同
    if(pass1!=pass2)
    {
        QMessageBox::about(this, "提示","两次密码不同");
        return;
    }
    //name不大于10
    if(name.length()>10)
    {
        QMessageBox::about(this, "提示","名字过长");
        return;
    }
    Q_EMIT SIG_registerCommit(strTel,pass1,name);


}

