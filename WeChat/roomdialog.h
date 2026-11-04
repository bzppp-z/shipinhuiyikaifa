#ifndef ROOMDIALOG_H
#define ROOMDIALOG_H

#include <QDialog>
#include<QVBoxLayout>
#include"usershow.h"
namespace Ui {
class RoomDialog;
}

class RoomDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RoomDialog(QWidget *parent = nullptr);
    ~RoomDialog();
public slots:
    void slot_Setinfo(QString roomid);
    void slot_addUserShow(UserShow * user);
    //刷新视频帧————————————————————————————————————
    void slot_refreshUser(int id,QImage & img);
    //要移除的信息
    void slot_removeUserShow(UserShow* user);
    //根据id移除一个人信息
    void slot_removeUserShow(int id);
    //音视频初始化————————————————————————
    void slot_setAudioCheck(bool cheack);
    void slot_setVideoCheck(bool cheack);
    void slot_setDeskCheck(bool cheack);
    //左边大画面显示当前登录的人
    void slot_setSelfInfo(int id,QString name);
    void slot_setBigImgID(int id,QString name);
    //清空信息
    void slot_clearUserShow();
    void on_pb_close_clicked();
    void on_pb_levea_clicked();
    void closeEvent(QCloseEvent *event);
signals:
    void SIG_close();
    void SIG_AudioPause();
    void SIG_AudioStart();
    void SIG_VideoStart();
    void SIG_VideoPause();
    void SIG_DeskStart();
    void SIG_DeskPause();
    void SIG_setMoji(int index);
private slots:
    void on_db_audio_clicked();

    void on_cb_video_clicked();

    void on_cb_desk_clicked();


    void on_sb_lvjing_currentIndexChanged(int index);

private:
    Ui::RoomDialog *ui;
    QVBoxLayout *m_mainLayout;
    //房间用户映射关系
    std::map<int,UserShow*>m_mapIDToUserShow;
};

#endif // ROOMDIALOG_H
