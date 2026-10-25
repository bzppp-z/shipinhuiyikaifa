#include "ckernel.h"
#include"QDebug"
#include<QTime>
#include<QBuffer>
//通过宏定义化简
#define NetMap(x)  m_netMap[ x - _DEF_PACK_BASE ]


//协议映射关系
void Ckernel::setNetMap()
{
    memset( m_netMap , 0 , sizeof(PFUN ));
    //    m_netMap[ _DEF_PACK_LOGIN_RS - _DEF_PROTOCOL_BASE ] = &CKernel::slot_dealLoginRs;
    NetMap(_DEF_PACK_LOGIN_RS) = &Ckernel::slot_dealLoginRs;
    NetMap(_DEF_PACK_REGISTER_RS)=&Ckernel::slot_dealRegisterRs;
    NetMap(_DEF_PACK_CREATEROOM_RS)=&Ckernel::slot_CreateRoomRs;
    NetMap(_DEF_PACK_JOINROOM_RS)=&Ckernel::slot_JoinRoomRs;
    NetMap(_DEF_PACK_ROOM_MEMBER)=&Ckernel::slot_RoomMenrq;
    NetMap(_DEF_ROOM_MEMBER_RQ)=&Ckernel::slot_RoomMenrq;
    NetMap(_DEF_PACK_LEAVEROOM_RQ)=&Ckernel::slot_dealLeaveRq;
    NetMap(_DEF_PACK_AUDIO_FRAME)=&Ckernel::slot_audioFrameRq;
    NetMap(_DEF_PACK_VIDEO_FRAME)=&Ckernel::slot_videoFrameRq;
}


//初始化配置
void Ckernel::initConfig()
{
    m_serverIP = _DEF_SERVER_IP;
    // 路径设置  exe 同级的目录下 -> applicationDirPath config.ini
    QString path = QApplication::applicationDirPath() + "/config.ini";
    //判断是否存在
    QFileInfo info( path );
    QSettings settings( path , QSettings::IniFormat , NULL ); // 有打开 没有创建
    if( info.exists() ){
        //加载配置文件 ip  如配置文件 设置为配置文件中的ip
        //打开配置文件
        //移动到 Net 组
        settings.beginGroup( "Net");
        //读取 ip -> addr --> 赋值
        QVariant ip = settings.value( "ip" );
        QString strIP = ip.toString();
        //结束
        settings.endGroup();
        if( !strIP.isEmpty() ){
            m_serverIP = strIP;
        }
    }else{
        //没有配置文件 写入默认的ip
        settings.beginGroup( "Net");
        settings.setValue( "ip" , m_serverIP );
        settings.endGroup();
    }
    qDebug() << m_serverIP;
}

Ckernel::Ckernel(QObject *parent) : QObject(parent),id(0),roomid(0)
{
    qDebug()<<"Ckernel thread:"<<QThread::currentThreadId();
    //设置协议映射关系
    setNetMap();
    m_pWeChatDlg=new WeChatDialog;
    // m_pWeChatDlg->show();


    initConfig();

    m_loginDialog   = new LoginDialog;
    connect( m_loginDialog , SIGNAL(SIG_loginCommit(QString,QString)),
             this , SLOT(slot_loginCommit(QString,QString)));
    connect(m_loginDialog,SIGNAL(SIG_close()),
            this,SLOT(slot_destroy()));
    connect(m_loginDialog,SIGNAL(SIG_registerCommit(QString,QString,QString)),
            this,SLOT(slot_registerCommit(QString,QString,QString)));
    m_loginDialog->show();
    //创建房间界面
    m_pRoomDialog=new RoomDialog;
    connect( m_pRoomDialog , SIGNAL(SIG_close())
             , this , SLOT(slot_quitRoom()));
    connect( m_pRoomDialog , SIGNAL(SIG_AudioPause())
             , this , SLOT(slot_AudioPause()));
    connect( m_pRoomDialog , SIGNAL(SIG_AudioStart())
             , this , SLOT(slot_AudioStart()));
    connect( m_pRoomDialog , SIGNAL(SIG_VideoPause())
             , this , SLOT(slot_VideoPause()));
    connect( m_pRoomDialog , SIGNAL(SIG_VideoStart())
             , this , SLOT(slot_VideoStart()));
    connect( m_pRoomDialog , SIGNAL(SIG_DeskPause())
             , this , SLOT(slot_DeskPause()));
    connect( m_pRoomDialog , SIGNAL(SIG_DeskStart())
             , this , SLOT(slot_DeskStart()));
    //添加网络
    m_client        = new TcpClientMediator;
    //音频和视频网络连接
    for(int i=0;i<=1;i++)
    {
        m_pAVClient[i]=new TcpClientMediator;
        m_pAVClient[i]->OpenNet( m_serverIP.toStdString().c_str(),8000);
        connect( m_pAVClient[i] , SIGNAL(SIG_ReadyData(uint,char*,int))
                 , this , SLOT( slot_dealData(uint,char*,int)) );
    }
    connect( m_client , SIGNAL(SIG_ReadyData(uint,char*,int))
             , this , SLOT( slot_dealData(uint,char*,int)) );
    connect( m_pWeChatDlg , SIGNAL(SIG_close())
             ,this , SLOT(slot_destroy()) );
    connect(m_pWeChatDlg,SIGNAL(SIG_Create()),
            this,SLOT(slot_Create()));
    connect(m_pWeChatDlg,SIGNAL(SIG_joinroom()),
            this,SLOT(slot_joinroom()));
    m_client->OpenNet( m_serverIP.toStdString().c_str(),8000);
    m_pAudioRead=new Audio_Read;
    connect(m_pAudioRead,SIGNAL(SIG_audioFrame( QByteArray)),
            this,SLOT(slot_audioFrame(QByteArray)));
    m_pVideoRead=new VideoRead;
    bool bVideoConn = connect(m_pVideoRead,SIGNAL(SIG_sendVideoFrame(QImage)),
                              this,SLOT(slot_sendVideoFrame(QImage)));
    qDebug() << "[VID] connect SIG_sendVideoFrame -> slot_sendVideoFrame = " << bVideoConn;
    m_pScreenRead=new ScreenRead;
    connect(m_pScreenRead,SIGNAL(SIG_getScreenFrame(QImage)),
            this,SLOT(slot_sendVideoFrame(QImage)));
    m_pSendVideoWork=QSharedPointer<SendVideoWork>(new SendVideoWork);
    connect(this,SIGNAL(SIG_SendVideo(char*,int)),
            m_pSendVideoWork.data(),SLOT(slot_sendVideo(char*,int)));
    //设置萌拍效果
    connect( m_pRoomDialog , SIGNAL(SIG_setMoji(int))
             , m_pVideoRead , SLOT(slot_setMoji(int)));
}
//回收
void Ckernel::slot_destroy()
{
    qDebug() << __func__;


    if( m_pWeChatDlg ){
        m_pWeChatDlg->hide();
        delete m_pWeChatDlg; m_pWeChatDlg = NULL;
    }
    if( m_loginDialog ){
        m_loginDialog->hide(); //不写close 不然会触发关闭事件会死循环
        delete m_loginDialog; m_loginDialog = NULL;
    }
    if(m_pAudioRead)
    {
        //m_pAudioRead->hide(); //不写close 不然会触发关闭事件会死循环
        delete m_pAudioRead; m_pAudioRead = NULL;
    }
    if(m_pRoomDialog)
    {
        m_pRoomDialog->hide(); //不写close 不然会触发关闭事件会死循环
        delete m_pRoomDialog; m_pRoomDialog = NULL;
    }
    if( m_client ){
        m_client->CloseNet();
        delete m_client; m_client = NULL;
    }
    exit(0);

}
static std::string Getmd5(QString ps)
{
    std::string strpass=ps.toStdString();
    MD5 md5(strpass);
    return md5.toString();
}
//提交登录信息
void Ckernel::slot_loginCommit(QString tel, QString ps)
{
    std::string strtel=tel.toStdString();
    //std::string strpass=ps.toStdString();
    STRU_LOGIN_RQ rq;
    strcpy(rq.tel,strtel.c_str());
    //md5加密
    std::string md5pass=Getmd5(ps);
    strcpy(rq.password,md5pass.c_str());
    //    //md5加密
    //    int len=strlen(rq.password);
    //    MD5 md5(rq.password,len);
    //    std::string passmd5=md5.toString();
    qDebug()<<md5pass.c_str();
    m_client->SendData(0,(char*)&rq,sizeof(rq));
}

void Ckernel::slot_registerCommit(QString tel, QString ps, QString name)
{
    std::string strtel=tel.toStdString();
    std::string strname=name.toStdString();//utf_8兼容中文:QString->std::string->cahr*
    STRU_REGISTER_RQ rq;
    strcpy(rq.name,strname.c_str());
    strcpy(rq.tel,strtel.c_str());
    //md5加密
    std::string md5pass=Getmd5(ps);
    strcpy(rq.password,md5pass.c_str());
    m_client->SendData(0,(char*)&rq,sizeof(rq));
}
#include<QInputDialog>
#include"QRegExp"
void Ckernel::slot_joinroom()
{
    //检查是否在房间里
    if(roomid!=0)
    {
        QMessageBox::about(m_pWeChatDlg,"提示","已经在房间内");
        return;
    }
    //弹出窗口 输入房间号
    QString strRoom=QInputDialog::getText(m_pWeChatDlg,"加入房间","请输入房间号");
    //检查是否合法
    QRegExp exp("^[0-9]{1,10}$");
    if(!exp.exactMatch(strRoom))
    {
        QMessageBox::about(m_pWeChatDlg,"提示","请输入正确房间号");
        return;
    }
    qDebug()<<strRoom;
    //发命令加入房间
    STRU_JOINROOM_RQ rq;
    rq.m_UserID=id;
    rq.m_RoomID=strRoom.toInt();
    m_client->SendData(0,(char*)&rq,sizeof(rq));
}

void Ckernel::slot_Create()
{
    //检查是否在房间里
    if(roomid!=0)
    {
        QMessageBox::about(m_pWeChatDlg,"提示","已经在房间内");
        return;
    }
    //创建房间
    STRU_CREATEROOM_RQ rq;
    rq.m_UserID=id;
    m_client->SendData(0,(char*)&rq,sizeof(rq));
}
//音频开启和关闭
void Ckernel::slot_AudioPause()
{
    m_pAudioRead->pause();
}

void Ckernel::slot_AudioStart()
{

    m_pAudioRead->start();
}

void Ckernel::slot_audioFrame(QByteArray ba)
{
    int size=24+ba.size();
    char *buf=new char[size];
    char *tmp=buf;

    int type=_DEF_PACK_AUDIO_FRAME;
    int userId=id;
    //注意: 不能写 int roomid=roomid; 那是用未初始化的自己初始化, 会盖掉成员变量,
    //发出去的房间号就是垃圾值, 服务端按房间号找不到人 -> 一帧都不转发
    int nRoomId=this->roomid;
    QTime tm=QTime::currentTime();
    int min=tm.minute();
    int sec=tm.second();
    int msec=tm.msec();

    *(int *)tmp=type;
    tmp+=sizeof(int);

    *(int *)tmp=userId;
    tmp+=sizeof(int);

    *(int *)tmp=nRoomId;
    tmp+=sizeof(int);

    *(int *)tmp=min;
    tmp+=sizeof(int);

    *(int *)tmp=sec;
    tmp+=sizeof(int);

    *(int *)tmp=msec;
    tmp+=sizeof(int);

    memcpy(tmp,ba.data(),ba.size());

    //只打印前3帧, 确认发出去的房间号是对的
    static int s_sndDbg = 0;
    if( s_sndDbg < 3 )
    {
        ++s_sndDbg;
        qDebug() << "send audio: uid" << userId << "rid" << nRoomId << "size" << size;
    }

    m_pAVClient[audio_client]->SendData(0,buf,size);
    delete[] buf;
}

void Ckernel::slot_audioFrameRq(uint sock, char *buf, int nlen)
{


    char *tmp=buf;

    int type=_DEF_PACK_AUDIO_FRAME;
    int m_userid;
    int m_roomid;


    tmp+=sizeof(int);

    m_userid=*(int *)tmp;
    tmp+=sizeof(int);

    m_roomid=*(int *)tmp;
    tmp+=sizeof(int);

    //跳过时间
    tmp+=sizeof(int);

    tmp+=sizeof(int);

    tmp+=sizeof(int);


    QByteArray ba(tmp,nlen-6*sizeof(int));
    //只打印前5帧, 避免刷屏
    static int s_audioDbg = 0;
    if( s_audioDbg < 5 )
    {
        ++s_audioDbg;
        qDebug() << "audio rq: uid" << m_userid << "rid" << m_roomid
                 << "selfrid" << roomid << "players" << (int)m_mapIDToAudioWrute.size()
                 << "hit" << (int)m_mapIDToAudioWrute.count(m_userid)
                 << "bytes" << ba.size();
    }
    if(m_roomid==roomid)
    {
        if(m_mapIDToAudioWrute.count(m_userid)>0)
        {
            Audio_Write *aw=m_mapIDToAudioWrute[m_userid];
            aw->slot_net_rx(ba);
        }
    }

}
//发送视频帧
void Ckernel::slot_sendVideoFrame(QImage img)
{
    static int s_sndCnt = 0;
    ++s_sndCnt;
    if( s_sndCnt <= 3 )
        qDebug() << "[VID] slot_sendVideoFrame cnt =" << s_sndCnt
                 << "img =" << img.size() << "null =" << img.isNull()
                 << "uid =" << id << "rid =" << roomid;
    //视频帧自定义协议
    //成员描述
    //int type;
    //int useridd;
    //int roomid;
    //int min;
    //int sec;
    //int msec;
    //QByteArray videoFrame
    //显示图片
    slot_refreshVideo(id,img);
    //压缩
    //压缩图片从RGB24格式压缩到JPEG格式, 发送出去
    QByteArray ba;
    QBuffer qbuf(&ba); // QBuffer 与 QByteArray 字节数组联立联系
    img.save( &qbuf , "JPEG" , 50 );  //将图片的数据写入 ba
    //使用ba对象, 可以获取图片对应的缓冲区
    //可以使用ba.data() , ba.size()将缓冲区发送出去
    //写视频帧 发送
    int nlen=6*sizeof(int)+ba.size();
    if( s_sndCnt <= 3 )
        qDebug() << "[VID] jpeg bytes =" << ba.size() << "packet =" << nlen;
    char *buf=new char[nlen];
    char *tmp=buf;
    int type=_DEF_PACK_VIDEO_FRAME;
    int userId=id;
    //注意: 不能写 int roomid=roomid; 那是用未初始化的自己初始化, 会盖掉成员变量,
    //发出去的房间号就是垃圾值, 服务端按房间号找不到人 -> 一帧都不转发
    int nRoomId=this->roomid;
    QTime tm=QTime::currentTime();
    int min=tm.minute();
    int sec=tm.second();
    int msec=tm.msec();

    *(int *)tmp=type;
    tmp+=sizeof(int);

    *(int *)tmp=userId;
    tmp+=sizeof(int);

    *(int *)tmp=nRoomId;
    tmp+=sizeof(int);

    *(int *)tmp=min;
    tmp+=sizeof(int);

    *(int *)tmp=sec;
    tmp+=sizeof(int);

    *(int *)tmp=msec;
    tmp+=sizeof(int);

    memcpy(tmp,ba.data(),ba.size());
    //发送是一个阻塞函数，如果服务器缓冲区由于数量很大，没及时取走缓冲区数据
    //滑动窗口变小  send函数阻塞，影响用户界面响应，出现卡顿
    //将视频发送变为一个信号，放到另一个线程执行  todo
//    m_client->SendData(0,buf,nlen);
//    delete[] buf;
    Q_EMIT SIG_SendVideo(buf,nlen);
}
//多线程发送视频
void Ckernel::slot_SendVideo(char *buf, int nlen)
{
    char *tmp=buf;
    tmp+=sizeof(int);
    tmp+=sizeof(int);
    tmp+=sizeof(int);

    int min=*(int *)tmp;
    tmp+=sizeof(int);
    int sec=*(int *)tmp;
    tmp+=sizeof(int);
    int msec=*(int *)tmp;
    tmp+=sizeof(int);
    //当前时间
    QTime ctm=QTime::currentTime();
    //数据包时间
    QTime tm(ctm.hour(),min,sec,msec);
    //发送数据包时间超过300，舍弃
    if(tm.msecsTo(ctm)>300)
    {
        qDebug()<<"send fail";
        delete[] buf;
        return ;
    }

    qDebug()<<"slot_SendVideo thread:"<<QThread::currentThreadId();
    m_pAVClient[video_client]->SendData(0,buf,nlen);
     delete[] buf;
}

void Ckernel::slot_VideoPause()
{
    qDebug() << "[VID] slot_VideoPause";
    m_pVideoRead->slot_closeVideo();
}

void Ckernel::slot_VideoStart()
{
    qDebug() << "[VID] slot_VideoStart, rid =" << roomid;
    m_pVideoRead->slot_openVideo();
}

void Ckernel::slot_refreshVideo(int id,QImage &img)
{
    m_pRoomDialog->slot_refreshUser(id,img);
}

void Ckernel::slot_videoFrameRq(uint sock, char *buf, int nlen)
{
    qDebug() << "slot_videoFrameRq";
    char *tmp=buf;
    int m_userid;
    int m_roomid;
    tmp+=sizeof(int);
    m_userid=*(int *)tmp;
    tmp+=sizeof(int);
    m_roomid=*(int *)tmp;
    tmp+=sizeof(int);
    //跳过时间
    tmp+=sizeof(int);
    tmp+=sizeof(int);
    tmp+=sizeof(int);
    static int s_rcvCnt = 0;
    ++s_rcvCnt;
    QImage img;
    int datalen=nlen-6*sizeof(int);
    bool bLoad = img.loadFromData(reinterpret_cast<const uchar*>(tmp), datalen);
    if( s_rcvCnt <= 3 || !bLoad )
        qDebug() << "[VID] video rq cnt =" << s_rcvCnt << "uid =" << m_userid
                 << "rid =" << m_roomid << "selfrid =" << roomid
                 << "datalen =" << datalen << "load =" << bLoad << "img =" << img.size();

    if(m_roomid==roomid)
    {
        m_pRoomDialog->slot_refreshUser(m_userid,img);
    }
}
//桌面开启和关闭
void Ckernel::slot_DeskPause()
{
    m_pScreenRead->slot_PauseVideo();
}

void Ckernel::slot_DeskStart()
{
    m_pScreenRead->slot_Openvideo();
}
//桌面采集发送, 和视频采集一样，但是不是单独线程
void Ckernel::slot_getScreenFrame(QImage img)
{
    qDebug()<<"slot_getScreenFrame thread:"<<QThread::currentThreadId();
    static int s_sndCnt = 0;
    ++s_sndCnt;
    if( s_sndCnt <= 3 )
        qDebug() << "[VID] slot_sendVideoFrame cnt =" << s_sndCnt
                 << "img =" << img.size() << "null =" << img.isNull()
                 << "uid =" << id << "rid =" << roomid;
    //视频帧自定义协议
    //成员描述
    //int type;
    //int useridd;
    //int roomid;
    //int min;
    //int sec;
    //int msec;
    //QByteArray videoFrame
    //显示图片
    slot_refreshVideo(id,img);
    //压缩
    //压缩图片从RGB24格式压缩到JPEG格式, 发送出去
    QByteArray ba;
    QBuffer qbuf(&ba); // QBuffer 与 QByteArray 字节数组联立联系
    img.save( &qbuf , "JPEG" , 50 );  //将图片的数据写入 ba
    //使用ba对象, 可以获取图片对应的缓冲区
    //可以使用ba.data() , ba.size()将缓冲区发送出去
    //写视频帧 发送
    int nlen=6*sizeof(int)+ba.size();
    if( s_sndCnt <= 3 )
        qDebug() << "[VID] jpeg bytes =" << ba.size() << "packet =" << nlen;
    char *buf=new char[nlen];
    char *tmp=buf;
    int type=_DEF_PACK_VIDEO_FRAME;
    int userId=id;
    //注意: 不能写 int roomid=roomid; 那是用未初始化的自己初始化, 会盖掉成员变量,
    //发出去的房间号就是垃圾值, 服务端按房间号找不到人 -> 一帧都不转发
    int nRoomId=this->roomid;
    QTime tm=QTime::currentTime();
    int min=tm.minute();
    int sec=tm.second();
    int msec=tm.msec();

    *(int *)tmp=type;
    tmp+=sizeof(int);

    *(int *)tmp=userId;
    tmp+=sizeof(int);

    *(int *)tmp=nRoomId;
    tmp+=sizeof(int);

    *(int *)tmp=min;
    tmp+=sizeof(int);

    *(int *)tmp=sec;
    tmp+=sizeof(int);

    *(int *)tmp=msec;
    tmp+=sizeof(int);

    memcpy(tmp,ba.data(),ba.size());
    m_client->SendData(0,buf,nlen);
    delete[] buf;



}
//网络信息处理
void Ckernel::slot_dealData(uint sock,char* buf,int nlen)
{
    //协议映射表
    int type = *(int*)buf;
    if( type == _DEF_PACK_VIDEO_FRAME )
    {
        static int s_vfCnt = 0;
        if( ++s_vfCnt <= 3 || (s_vfCnt % 50 == 0) )
            qDebug() << "[NET] dealData VIDEO" << s_vfCnt << "len =" << nlen;
    }
    if( type != _DEF_PACK_AUDIO_FRAME && type != _DEF_PACK_VIDEO_FRAME )      //音频帧太频繁, 不打印
        qDebug() << "recv type =" << type << "len =" << nlen;
    else if( type == _DEF_PACK_AUDIO_FRAME )
    {
        //音频帧只打印前3个: 用来区分"包没到"和"包到了但没分发"
        static int s_afDbg = 0;
        if( s_afDbg < 3 )
        {
            ++s_afDbg;
            qDebug() << "recv AUDIO len =" << nlen;
        }
    }
    if( type >= _DEF_PACK_BASE && type < _DEF_PACK_BASE + _DEF_PACK_COUNT )
    {
        //取得协议头，根据协议映射关系，找到函数指针
        PFUN pf = NetMap(type);
        if( pf )
            (this->*pf)( sock , buf , nlen );
    }

    delete[] buf;
}


//登录回复处理
void Ckernel::slot_dealLoginRs(uint sock,char* buf,int nlen){

    qDebug() << __func__;
    //拆包
    STRU_LOGIN_RS* rs=(STRU_LOGIN_RS*)buf;
    //根据返回结果，得到不同信息
    if(rs->result==login_success)
    {
        QString strName=QString("用户[%1]登录成功").arg(rs->name);
        QMessageBox::about(m_loginDialog,"提示",strName);
        //id记录
        id=rs->userid;
        name=QString::fromStdString(rs->name);
        m_pWeChatDlg->SetInfo(name);
        //ui跳转
        m_loginDialog->hide();
        m_pWeChatDlg->showNormal();
        //注册视频和音频fd
        STRU_AUDIO_REGISTER rq_audio;
        rq_audio.m_id=id;
        STRU_VIDEO_REGISTER rq_video;
        rq_video.m_id=id;
        m_pAVClient[audio_client]->SendData(0,(char *)&rq_audio,sizeof(rq_audio));
        m_pAVClient[video_client]->SendData(0,(char *)&rq_video,sizeof(rq_video));

    }
    else if(rs->result==password_error)
    {
        QMessageBox::about(m_loginDialog,"提示","密码错误存在");
    }
    else{
        QMessageBox::about(m_loginDialog,"提示","用户不存在");
    }
}
//注册回复处理
void Ckernel::slot_dealRegisterRs(uint sock,char* buf,int nlen){
    qDebug() << __func__;
    //拆包
    STRU_REGISTER_RS* rs=(STRU_REGISTER_RS *)buf;
    //根据不同的结果弹出不同的提示窗
    if(rs->result==register_success)
    {
        QMessageBox::about(m_loginDialog,"提示","注册成功");
    }
    else if(rs->result==tel_is_exist)
    {
        QMessageBox::about(m_loginDialog,"提示","电话已经存在");
    }
    else{
        QMessageBox::about(m_loginDialog,"提示","姓名已经存在");
    }
}
//创建房间回复处理
void Ckernel::slot_CreateRoomRs(uint sock, char *buf, int nlen)
{
    //拆包
    STRU_CREATEROOM_RS* rs=(STRU_CREATEROOM_RS*)buf;
    //房间号  显示到界面跳转

    //先改变一下最上面标题
    m_pRoomDialog->slot_Setinfo(QString::number(rs->m_RoomId));
    //左边大画面显示自己
    m_pRoomDialog->slot_setSelfInfo(id,name);
    //服务器没有i吧个人信息发过来，作为第一个进入房间的

    //把自己的信息放到房间里  做显示  todo
    UserShow *user =new UserShow;
    connect(user,SIGNAL(SIG_itemClicked(int,QString))
            ,m_pRoomDialog,SLOT(slot_setBigImgID(int,QString)));
    user->slot_setInfo(id,name);
    m_pRoomDialog->slot_addUserShow(user);

    roomid=rs->m_RoomId;
    m_pRoomDialog->showNormal();
    //音频初始化
    m_pRoomDialog->slot_setAudioCheck(false);
    //视频初始化todo
    m_pRoomDialog->slot_setVideoCheck(false);
}
//加入房间回复处理
void Ckernel::slot_JoinRoomRs(uint sock, char *buf, int nlen)
{
    //拆包
    STRU_JOINROOM_RS*rs=(STRU_JOINROOM_RS*)buf;
    //根据结果  失败  提示
    if(rs->m_lResult==0)
    {
        QMessageBox::about(m_pWeChatDlg,"提示","房间id不存在，加入失败");
        return;
    }
    //成功
    //房间号  显示到界面 跳转
    m_pRoomDialog->slot_Setinfo(QString::number(rs->m_RoomID));
    //左边大画面显示自己
    m_pRoomDialog->slot_setSelfInfo(id,name);
    //跳转  roomid设置
    roomid=rs->m_RoomID;
    //自己也是房间成员, 先把自己显示出来(和创建房间保持一致, 同id会去重)
    UserShow *user =new UserShow;
    connect(user,SIGNAL(SIG_itemClicked(int,QString))
            ,m_pRoomDialog,SLOT(slot_setBigImgID(int,QString)));
    user->slot_setInfo(id,name);
    m_pRoomDialog->slot_addUserShow(user);
    //显示画面
    m_pRoomDialog->showNormal();
    //音频初始化
    m_pRoomDialog->slot_setAudioCheck(false);
    //视频初始化todo
    m_pRoomDialog->slot_setVideoCheck(false);
}
//自己退出房间操作
void Ckernel::slot_quitRoom()
{

    //发退出包
    STRU_LEAVEROOM_RQ rq;
    rq.m_nUserId=id;
    rq.m_RoomId=roomid;
    // std ::string m_name=name.toStdString();
    // strcpy(m_name,rq.szUserName);
    m_client->SendData(0,(char*)&rq,sizeof(rq));
    //关闭音频视频
    m_pAudioRead->pause();
    m_pVideoRead->slot_closeVideo();
    m_pScreenRead->slot_PauseVideo();
    m_pRoomDialog->slot_setAudioCheck(false);
    m_pRoomDialog->slot_setVideoCheck(false);
    //回收所有人的audiowrite
    for(auto ite=m_mapIDToAudioWrute.begin();ite!=m_mapIDToAudioWrute.end();)
    {
        Audio_Write* p=ite->second;
        ite=m_mapIDToAudioWrute.erase(ite);
        delete p;
    }
    //回收资源
    m_pRoomDialog->slot_clearUserShow();
    roomid=0;
}
//房间成员请求处理
void Ckernel::slot_RoomMenrq(uint sock, char *buf, int nlen)
{
    //拆包
    STRU_ROOM_MEMBER_RQ* rq=(STRU_ROOM_MEMBER_RQ*)buf;
    qDebug() << "ROOM_MEMBER id =" << rq->m_id << rq->m_szUserName;
    //创建对应的控件
    UserShow* user=new UserShow;
    connect(user,SIGNAL(SIG_itemClicked(int,QString))
            ,m_pRoomDialog,SLOT(slot_setBigImgID(int,QString)));
    user->slot_setInfo(rq->m_id,QString::fromStdString(rq->m_szUserName));
    m_pRoomDialog->slot_addUserShow(user);

    //音频初始化
    Audio_Write * aw=NULL;
    //为每个人创建一个播放对象
    if(m_mapIDToAudioWrute.count(rq->m_id)==0)
    {
        aw=new Audio_Write;
        m_mapIDToAudioWrute[rq->m_id]=aw;
    }

    //视频初始化todo

}
//接受房间成员离开请求处理
void Ckernel::slot_dealLeaveRq(uint sock, char *buf, int nlen)
{
    //拆包
    STRU_LEAVEROOM_RQ* rq=(STRU_LEAVEROOM_RQ*)buf;
    //根据id把人移除  调用slot_removeUserShow(int id)这个重载函数
    if(rq->m_RoomId==roomid)
    {
        m_pRoomDialog->slot_removeUserShow(rq->m_nUserId);
    }
    if(m_mapIDToAudioWrute.count(rq->m_nUserId)>0)
    {
        Audio_Write *Paw=m_mapIDToAudioWrute[rq->m_nUserId];
        m_mapIDToAudioWrute.erase(rq->m_nUserId);
        delete Paw;
    }

}
