#include<TCPKernel.h>
#include "packdef.h"
#include<stdio.h>
#include<sys/time.h>
#include"clogic.h"

using namespace std;

namespace {
bool hasMinimumPacketSize(PackType type, int nlen)
{
    switch (type)
    {
    case _DEF_PACK_REGISTER_RQ:
        return nlen >= static_cast<int>(sizeof(STRU_REGISTER_RQ));
    case _DEF_PACK_LOGIN_RQ:
        return nlen >= static_cast<int>(sizeof(STRU_LOGIN_RQ));
    case _DEF_PACK_CREATEROOM_RQ:
        return nlen >= static_cast<int>(sizeof(STRU_CREATEROOM_RQ));
    case _DEF_PACK_JOINROOM_RQ:
        return nlen >= static_cast<int>(sizeof(STRU_JOINROOM_RQ));
    case _DEF_PACK_LEAVEROOM_RQ:
        return nlen >= static_cast<int>(sizeof(STRU_LEAVEROOM_RQ));
    case _DEF_PACK_AUDIO_REGISTER:
    case _DEF_PACK_VIDEO_REGISTER:
        return nlen >= static_cast<int>(sizeof(STRU_AUDIO_REGISTER));
    case _DEF_PACK_AUDIO_FRAME:
    case _DEF_PACK_VIDEO_FRAME:
        // The router needs type, sender id and room id.  The media body may
        // be empty in a malformed packet, but those three fields are required.
        return nlen >= static_cast<int>(sizeof(PackType) + sizeof(int) + sizeof(int));
    default:
        return true;
    }
}
}


//设置网络协议映射
void TcpKernel::setNetPackMap()
{
    //清空映射
    bzero( m_NetPackMap , sizeof(m_NetPackMap) );

    //协议映射赋值
    m_logic->setNetPackMap();

}


TcpKernel::TcpKernel()
{

}

TcpKernel::~TcpKernel()
{
    if( m_logic ) delete m_logic;
}

TcpKernel *TcpKernel::GetInstance()
{
    static TcpKernel kernel;
    return &kernel;
}

int TcpKernel::Open( int port)
{
    // A peer is allowed to close while another worker is sending a response.
    // Do not let that ordinary network race terminate the whole process.
    signal(SIGPIPE, SIG_IGN);
    initRand();

    m_sql = new CMysql;
    // 数据库 使用127.0.0.1 地址  用户名root 密码colin123 数据库 WeChat 没有的话需要创建 不然报错
    if(  !m_sql->ConnectMysql( _DEF_DB_IP , _DEF_DB_USER, _DEF_DB_PWD, _DEF_DB_NAME )  )
    {
        printf("Conncet Mysql Failed...\n");
        return FALSE;
    }
    else
    {
        printf("MySql Connect Success...\n");
    }
    //初始网络
    m_tcp = new Block_Epoll_Net;
    bool res = m_tcp->InitNet( port , &TcpKernel::DealData ) ;
    if( !res )
        err_str( "net init fail:" ,-1);

    m_logic = new CLogic(this);

    setNetPackMap();

    return TRUE;
}

void TcpKernel::Close()
{
    m_sql->DisConnect();

}

//随机数初始化
void TcpKernel::initRand()
{
    struct timeval time;
    gettimeofday( &time , NULL);
    srand( time.tv_sec + time.tv_usec );
}

void TcpKernel::DealData(sock_fd clientfd,char *szbuf,int nlen)
{
    if (szbuf == NULL || nlen < static_cast<int>(sizeof(PackType)))
        return;
    PackType type = *(PackType*)szbuf;
    if( (type >= _DEF_PACK_BASE) && ( type < _DEF_PACK_BASE + _DEF_PACK_COUNT) )
    {
        if (!hasMinimumPacketSize(type, nlen))
            return;
        PFUN pf = NetPackMap( type );
        if( pf )
        {
            (TcpKernel::GetInstance()->m_logic->*pf)( clientfd , szbuf , nlen);
        }
    }

    return;
}

void TcpKernel::EventLoop()
{
    printf("event loop\n");
    m_tcp->EventLoop();
}

void TcpKernel::SendData(sock_fd clientfd, char *szbuf, int nlen)
{
    m_tcp->SendData(clientfd , szbuf ,nlen );
}



