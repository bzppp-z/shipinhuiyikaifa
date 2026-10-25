#include "roomdialog.h"
#include "ui_roomdialog.h"
#include<QMessageBox>
#include<QDebug>
RoomDialog::RoomDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RoomDialog)
{
    ui->setupUi(this);
    m_mainLayout =new QVBoxLayout;
    m_mainLayout->setContentsMargins(0,0,0,0);
    m_mainLayout->setSpacing((5));

    //设置一个数值布局的画布，可以向里面加东西
    ui->wbg_list->setLayout(m_mainLayout);
    m_mainLayout->addStretch(1);            // 尾部弹簧
//    //测试
//    for(int i=0;i<1;i++)
//    {
//        UserShow* user1=new UserShow;
//        user1->slot_setInfo(i+1,QString("测试用户%1").arg(i+1));
//        slot_addUserShow(user1);
//    }
}

RoomDialog::~RoomDialog()
{
    delete ui;
}

void RoomDialog::slot_Setinfo(QString roomid)
{
    QString title=QString("房间号:%1").arg(roomid);
    setWindowTitle(title);
    ui->lb_title->setText(title);
}

void RoomDialog::slot_addUserShow(UserShow *user)
{
    // 同一个 id 已经在列表里，先移除旧的，防止新旧控件叠加
          if(m_mapIDToUserShow.count(user->m_id) > 0)
          {
              slot_removeUserShow(user->m_id);   // 走 (int) 重载，内部会 erase
          }
          user->setFixedHeight(128);
          m_mainLayout->insertWidget(m_mainLayout->count() - 1,user);

          m_mapIDToUserShow[user->m_id] = user;
}

void RoomDialog::slot_refreshUser(int id, QImage &img)
{
    //预览图的id和要刷新的图片id一样，刷新预览图
    if(ui->wbf_usershow->m_id==id)
    {
        ui->wbf_usershow->slot_setImage(img);
    }
    if(m_mapIDToUserShow.count(id)>0)
    {
        UserShow* user=m_mapIDToUserShow[id];
        user->slot_setImage(img);
    }
    else
    {
        static int s_missCnt = 0;
        if( ++s_missCnt <= 3 )
            qDebug() << "[VID] refresh miss, id =" << id
                     << "mapSize =" << (int)m_mapIDToUserShow.size();
    }
}
//移除一个
void RoomDialog::slot_removeUserShow(UserShow *user)
{
    if(!user) return;
         m_mainLayout->removeWidget(user);
         user->hide();
         user->deleteLater();
         // 兜底：把 map 里指向它的项也删掉
         for(auto it = m_mapIDToUserShow.begin(); it != m_mapIDToUserShow.end(); )
         {
             if(it->second == user) it = m_mapIDToUserShow.erase(it);
             else ++it;
         }
}

void RoomDialog::slot_removeUserShow(int id)
{
    if(m_mapIDToUserShow.count(id)>0)
    {
        UserShow * removeuser=m_mapIDToUserShow[id];
          m_mapIDToUserShow.erase(id);
        slot_removeUserShow(removeuser);

    }

}

void RoomDialog::slot_setAudioCheck(bool cheack)
{
    ui->db_audio->setChecked(cheack);
}
void RoomDialog::slot_setVideoCheck(bool cheack)
{
    ui->cb_video->setChecked(cheack);
}
void RoomDialog::slot_setDeskCheck(bool cheack)
{
    ui->cb_desk->setChecked(cheack);
}
//左边大画面显示当前登录的人
void RoomDialog::slot_setSelfInfo(int id, QString name)
{
    qDebug() << "setSelfInfo:" << id << name;
    ui->wbf_usershow->slot_setInfo(id,name);
}

void RoomDialog::slot_setBigImgID(int id, QString name)
{
    ui->wbf_usershow->slot_setInfo(id,name);
}
//清空
void RoomDialog::slot_clearUserShow()
{
    for(auto it = m_mapIDToUserShow.begin(); it != m_mapIDToUserShow.end(); ++it)
          {
              UserShow *user = it->second;
              m_mainLayout->removeWidget(user);
              user->hide();
              user->deleteLater();
          }
          m_mapIDToUserShow.clear();      // ← 关键，必须加
}

//退出房间
void RoomDialog::on_pb_close_clicked()
{
    this->close();
}
void RoomDialog::on_pb_levea_clicked()
{
    this->close();
}
void RoomDialog::closeEvent(QCloseEvent *event)
{
    if(QMessageBox::question(this,"退出提示","是否退出会议")
            ==QMessageBox::Yes){
        Q_EMIT  SIG_close();
        event->accept();
        return;
    }
    event->ignore();
}

//开启或者关闭音频
void RoomDialog::on_db_audio_clicked()
{
    if(ui->db_audio->isChecked())
    {
        //ui->db_audio->setChecked(false);
         Q_EMIT SIG_AudioStart();

    }
    else
    {
        //ui->db_audio->setChecked(true);
        Q_EMIT SIG_AudioPause();
    }
}

//开启或者关闭视频
void RoomDialog::on_cb_video_clicked()
{
    if(ui->cb_video->isChecked())
    {
        ui->cb_desk->setChecked(false);
        Q_EMIT SIG_DeskPause();
         Q_EMIT SIG_VideoStart();

    }
    else
    {
        Q_EMIT SIG_VideoPause();
    }
}


void RoomDialog::on_cb_desk_clicked()
{
    if(ui->cb_desk->isChecked())
    {
         ui->cb_video->setChecked(false);
          Q_EMIT SIG_VideoPause();
         Q_EMIT SIG_DeskStart();

    }
    else
    {
        Q_EMIT SIG_DeskPause();
    }
}



void RoomDialog::on_sb_lvjing_currentIndexChanged(int index)
{
    Q_EMIT SIG_setMoji(index);
}

