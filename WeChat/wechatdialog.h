#ifndef WECHATDIALOG_H
#define WECHATDIALOG_H

#include <QMainWindow>
#include <QDialog>
#include <QCloseEvent>
QT_BEGIN_NAMESPACE
namespace Ui { class WeChatDialog; }
QT_END_NAMESPACE

class WeChatDialog : public QMainWindow
{
    Q_OBJECT

public:
    WeChatDialog(QWidget *parent = nullptr);
    ~WeChatDialog();

    void closeEvent(QCloseEvent *event);
    void SetInfo(QString name,int icon=1);
signals:
    void SIG_close();
    void SIG_joinroom();
    void SIG_Create();
private slots:
    void on_pd_Create_clicked();

    void on_pd_Joinroom_clicked();

private:
    Ui::WeChatDialog *ui;
};
#endif // WECHATDIALOG_H
