#ifndef CKERNEL_H
#define CKERNEL_H

#include <QObject>
#include "wechatdialog.h"
#include "TcpClientMediator.h"
#include "packdef.h"
#include"logindialog.h"
#include <QApplication>
#include <QSettings>
#include <QFileInfo>
#include"md5.h"
#include<QMessageBox>
#include"roomdialog.h"
#include"audio_read.h"
#include"audio_write.h"
#include"videoread.h"
#include"screenread.h"
#include"threadworker.h"
#include<QSharedPointer>
#include<QTime>
class Ckernel;
typedef void (Ckernel::*PFUN)(uint socket,char* buf,int nlen);

class SendVideoWork;
class Ckernel : public QObject
{
    Q_OBJECT
public:
    explicit Ckernel(QObject *parent = nullptr);
    //单例
    static Ckernel* getInstance()
    {
        static Ckernel kernel;
        return &kernel;
    }
signals:
void SIG_SendVideo(char *buf,int nlen);
public slots:
    //设置协议映射关系
    void setNetMap();
    //初始化配置
    void initConfig();
    //回收
    void slot_destroy();
    //发送登录
    void slot_loginCommit(QString tel,QString ps);
    //发送注册信息
    void slot_registerCommit(QString tel,QString ps,QString name);
    void slot_joinroom();
    //创建会议
    void slot_Create();
    //音频开启和暂停
    void slot_AudioPause();
    void slot_AudioStart();
    //发送音频帧
    void slot_audioFrame(QByteArray ba);
     //服务端音频帧发过来处理----------------
    void slot_audioFrameRq(uint sock,char* buf,int nlen);
    //视频帧发送
    void slot_sendVideoFrame(QImage img);
    //多线程视频信息发送
    void slot_SendVideo(char *buf,int nlen);
    //视频开启和暂停
    void slot_VideoPause();
    void slot_VideoStart();
    //刷新图片显示
    void  slot_refreshVideo(int id,QImage &img);
    //服务端视频帧发过来处理
   void slot_videoFrameRq(uint sock,char* buf,int nlen);
   //桌面开启和暂停-------------------
   void slot_DeskPause();
   void slot_DeskStart();
   //桌面帧发送
   void slot_getScreenFrame(QImage img);
    //网络信息处理--------------处理---------------
    void slot_dealData(uint sock,char* buf,int nlen);

    ////登录回复处理
    void slot_dealLoginRs(uint sock,char* buf,int nlen);
    //注册回复处理
    void slot_dealRegisterRs(uint sock,char* buf,int nlen);
    //创建房间处理
    void slot_CreateRoomRs(uint sock,char* buf,int nlen);
    //加入房间处理
    void slot_JoinRoomRs(uint sock,char* buf,int nlen);
    //退出房间
    void slot_quitRoom();
    //房间成员请求处理
    void slot_RoomMenrq(uint sock,char* buf,int nlen);
    //离开房间请求处理
    void slot_dealLeaveRq(uint sock,char* buf,int nlen);
private:
    //协议映射表
      PFUN            m_netMap[_DEF_PACK_COUNT];
      LoginDialog*    m_loginDialog;
      WeChatDialog*   m_pWeChatDlg;
      RoomDialog*     m_pRoomDialog;
      INetMediator*   m_client;
      QString         m_serverIP;
      int id;
      int roomid=0;
      QString name;
      //音频  1个采集  多个播放  每一个房间成员，map映射
      Audio_Read * m_pAudioRead;
      std::map<int,Audio_Write*>m_mapIDToAudioWrute;

      //视频  1个采集  多个播放  每一个房间成员，map映射
        VideoRead *m_pVideoRead;
        ScreenRead* m_pScreenRead;
        enum client_type{audio_client=0,video_client};
        INetMediator* m_pAVClient[2];
        QSharedPointer<SendVideoWork>m_pSendVideoWork;
};

class SendVideoWork:public ThreadWorker{
    Q_OBJECT
public slots:
    void slot_sendVideo(char *buf,int nlen)
    {
        Ckernel::getInstance()->slot_SendVideo(buf,nlen);
    }
};

#endif // CKERNEL_H
