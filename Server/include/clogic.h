#ifndef CLOGIC_H
#define CLOGIC_H

#include"TCPKernel.h"

class CLogic
{
public:
    CLogic( TcpKernel* pkernel )
    {
        m_pKernel = pkernel;
        m_sql = pkernel->m_sql;
        m_tcp = pkernel->m_tcp;
    }
public:
    //设置协议映射
    void setNetPackMap();
    /************** 发送数据*********************/
    void SendData( sock_fd clientfd, char*szbuf, int nlen )
    {
        m_pKernel->SendData( clientfd ,szbuf , nlen );
    }
    /************** 网络处理 *********************/
    //注册
    void RegisterRq(sock_fd clientfd, char*szbuf, int nlen);
    //登录
    void LoginRq(sock_fd clientfd, char*szbuf, int nlen);

    /*******************************************/
    //处理创建房间请求
    void CreateRoomRq(int clientfd,char* szbuf,int nlen);
    //处理加入房间请求
     void JoinRoomRq(int clientfd,char* szbuf,int nlen);
    //离开房间请求
     void LeaveRoomRq(int clientfd,char* szbuf,int nlen);
     //处理音频帧
     void AudioFrameRq(int clientfd,char* szbuf,int nlen);
     void VideoFrameRq(int clientfd,char* szbuf,int nlen);
     void VideoRegister(int clientfd,char* szbuf,int nlen);
     void AudioRegister(int clientfd,char* szbuf,int nlen);
private:
    TcpKernel* m_pKernel;
    CMysql * m_sql;
    Block_Epoll_Net * m_tcp;
    MyMap<int,list<int>>m_mapIDToRoomid;
};

#endif // CLOGIC_H
