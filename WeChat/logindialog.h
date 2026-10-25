#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include<QCloseEvent>
namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT
signals:
    void SIG_loginCommit(QString tel,QString passord);
    void SIG_registerCommit(QString tel,QString passord,QString name);
    void SIG_close();
public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();
    void closeEvent(QCloseEvent *);
private slots:
    void on_pd_commit_clicked();

    void on_pb_clear_clicked();

    void on_pb_clear_rigister_clicked();

    void on_pd_commit_rigister_clicked();

private:
    Ui::LoginDialog *ui;
};

#endif // LOGINDIALOG_H
