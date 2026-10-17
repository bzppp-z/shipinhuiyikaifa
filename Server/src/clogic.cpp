#include "clogic.h"

void CLogic::setNetPackMap()
{
    NetPackMap(_DEF_PACK_REGISTER_RQ)    = &CLogic::RegisterRq;
    NetPackMap(_DEF_PACK_LOGIN_RQ)       = &CLogic::LoginRq;
    NetPackMap(_DEF_PACK_CREATEROOM_RQ)  = &CLogic::CreateRoomRq;
    NetPackMap(_DEF_PACK_JOINROOM_RQ)  = &CLogic::JoinRoomRq;
    NetPackMap(_DEF_PACK_LEAVEROOM_RQ)  = &CLogic::LeaveRoomRq;
    NetPackMap(_DEF_PACK_AUDIO_FRAME)   =&CLogic::AudioFrameRq;
    NetPackMap(_DEF_PACK_VIDEO_FRAME)   =&CLogic::VideoFrameRq;
    NetPackMap(_DEF_PACK_VIDEO_REGISTER)   =&CLogic::VideoRegister;
    NetPackMap(_DEF_PACK_AUDIO_REGISTER)   =&CLogic::AudioRegister;

}

#define _DEF_COUT_FUNC_    cout << "clientfd:"<< clientfd << __func__ << endl;

//注册
void CLogic::RegisterRq(sock_fd clientfd,char* szbuf,int nlen)
{
    printf("clientfd:%d RegisterRq\n", clientfd);
    STRU_REGISTER_RQ* rq=(STRU_REGISTER_RQ*) szbuf;
    STRU_REGISTER_RS rs; //栈上结构体，不再野指针
    //1.拆包
    //获取tel  password  name
    //查表 t_user 根据tel 查tel
    char sqlstr[1024]={0};
    sprintf(sqlstr,"select tel from t_user where tel= '%s'",rq->tel);
    list<string>reList;
    if(!m_sql->SelectMysql(sqlstr,1,reList))
    {
        printf("SelectMysql error:%s",sqlstr);
        return;
    }
    //有 user存在
    if(reList.size()>0)
    {
        rs.result=tel_is_exist;
    }
    else{
        //没有  name有没有
        sprintf(sqlstr,"select tel from t_user where tel= '%s'",rq->name);
        reList.clear();
        if(!m_sql->SelectMysql(sqlstr,1,reList))
        {
            printf("SelectMysql error:%s",sqlstr);
            return;
        }
        if(reList.size()>0)
        {
            //有 name存在
            rs.result=name_is_exist;
        }
        else
        {
            //没有 写表 tel pass  name 头像和签名的默认值  返回注册成功
            rs.result=register_success;
            sprintf(sqlstr,"insert into t_user(tel,password,name,icon,feeling) values('%s','%s','%s',%d,'%s')",
                    rq->tel, rq->password, rq->name, 1,"啥也没写");
            if(!m_sql->UpdataMysql(sqlstr))
            {
                printf("SelectMysql error:%s",sqlstr);
                return;
            }
        }
    }
    m_tcp->SendData(clientfd, (char*)&rs, sizeof(rs));


}

//登录
void CLogic::LoginRq(sock_fd clientfd ,char* szbuf,int nlen)
{
    printf("clientfd:%d LoginRq\n", clientfd);
    //拆包  // 手机号 密码
    STRU_LOGIN_RQ* rq=(STRU_LOGIN_RQ*) szbuf;
    STRU_LOGIN_RS rs; //栈上结构体，不再野指针
    // 根据tel 查pass 和id
    char sqlstr[1024]={0};
    sprintf(sqlstr,"select password,id,name from t_user where tel= '%s'",rq->tel);
    list<string>reList;
    if(!m_sql->SelectMysql(sqlstr,3,reList))
    {
        printf("SelectMysql error:%s",sqlstr);
        return;
    }
    //查不到 返回 没此用户
    if(reList.size()==0)
    {
        rs.result=user_not_exist;
    }
    else{
        // 查到了 pass是否一致

        if(strcmp(rq->password,reList.front().c_str())!=0)
        {
            // 不一致 返回密码错误
            rs.result=password_error;

        }else{
            reList.pop_front();
            //一致  sock 要保存起来  id -> sock 映射 (为了通信)
            int id=atoi(reList.front().c_str());
            reList.pop_front();

            //sock保存起来 id->sock映射
            UserInfo *pInfo=new UserInfo;
            pInfo->id=id;
            pInfo->roomid=0;
            pInfo->sockfd=clientfd;
            strcpy(pInfo->userName,reList.front().c_str());
            reList.pop_front();
            if(m_pKernel->m_mapIDToUserInfo.IsExist(pInfo->id))
            {
                //已经有账号登录，先强制下线
            }
            m_pKernel->m_mapIDToUserInfo.insert(pInfo->id,pInfo);
            rs.userid=id;
            rs.result=login_success;
            strcpy(rs.name,pInfo->userName);
        }
    }
    //写返回包  返回  带id 和结果
    m_tcp->SendData(clientfd, (char*)&rs, sizeof(rs));

}
void CLogic::CreateRoomRq(sock_fd clientfd, char *szbuf, int nlen)
{
    printf("clientfd:%d CreateRoomRq\n", clientfd);
    //拆包
    STRU_CREATEROOM_RQ* rq=(STRU_CREATEROOM_RQ*)szbuf;
    //随机数，得到房间号
    int roomid=0;
    do{
        roomid=rand()%99999999+1;
        //看有咩有房间号，可能循环随机，  map  roomid->list
    }while(m_mapIDToRoomid.IsExist(roomid));
    list<int>lit;
    lit.push_back(rq->m_UserID);
    m_mapIDToRoomid.insert(roomid,lit);
    //回复
    STRU_CREATEROOM_RS rs;
    rs.m_RoomId=roomid;
    rs.m_lResult=1;
    m_pKernel->SendData(clientfd,(char *)&rs,sizeof(rs));
}

void CLogic::JoinRoomRq(sock_fd clientfd, char *szbuf, int nlen)
{
    //拆包
    STRU_JOINROOM_RQ* rq=(STRU_JOINROOM_RQ*)szbuf;
    STRU_JOINROOM_RS rs;
    //查看房间是否存在
    //不存在 返回失败
    if(!m_mapIDToRoomid.IsExist(rq->m_RoomID))
    {
        rs.m_lResult=0;
        m_pKernel->SendData(clientfd,(char*)&rs,sizeof(rs));
        return;
    }
    //存在返回成功
    rs.m_lResult=1;
    rs.m_RoomID=rq->m_RoomID;
    m_pKernel->SendData(clientfd,(char *)&rs,sizeof(rs));

    if(!m_pKernel->m_mapIDToUserInfo.IsExist(rq->m_UserID))return;
    UserInfo* joiner=m_pKernel->m_mapIDToUserInfo.find(rq->m_UserID);


    //新加入的信息
    STRU_ROOM_MEMBER_RQ joinrq;
    joinrq.m_id=rq->m_UserID;
    strcpy(joinrq.m_szUserName,joiner->userName);
    m_pKernel->SendData(clientfd,(char *)&joinrq,sizeof(joinrq));

    //根据房间号拿到房间成员列表
    list<int>lst=m_mapIDToRoomid.find(rq->m_RoomID);
    //遍历列表  互换信息
    for(auto ite=lst.begin();ite!=lst.end();ite++)
    {
        int Memid=*ite;
        if(!m_pKernel->m_mapIDToUserInfo.IsExist(Memid))
        {
            continue;
        }
        UserInfo* Meminfo=m_pKernel->m_mapIDToUserInfo.find(Memid);
        //房间内成员个人信息
        STRU_ROOM_MEMBER_RQ memrq;
        memrq.m_id=Meminfo->id;
        strcpy(memrq.m_szUserName,Meminfo->userName);

        //把加入人的信息发给每一个房间内成员
        m_pKernel->SendData(Meminfo->sockfd,(char *)&joinrq,sizeof(joinrq));
        //房间内成员个人信息发送给加入ren
        m_pKernel->SendData(clientfd,(char *)&memrq,sizeof(memrq));
    }
    //加入人 添加到房间列表
    lst.push_back(rq->m_UserID);
    m_mapIDToRoomid.insert(rq->m_RoomID,lst);
}

void CLogic::LeaveRoomRq(int clientfd, char *szbuf, int nlen)
{
    //拆包
    STRU_LEAVEROOM_RQ* rq=(STRU_LEAVEROOM_RQ*)szbuf;
    //看房间是否存在
    if(!m_mapIDToRoomid.IsExist(rq->m_RoomId))return;
    //如果房间存在，可以获得用户列表
    list<int>lst=m_mapIDToRoomid.find(rq->m_RoomId);
    //遍历每个用户，用户是否在线，在线转发
    for(auto it=lst.begin();it!=lst.end();)
    {
        //是不是自己，如果是自己 从链表里面移除——房间移除
        if(*it==rq->m_nUserId)
        {
            it=lst.erase(it);
        }
        else{
            //不是自己，看是否在线，在线转发
            if(m_pKernel->m_mapIDToUserInfo.IsExist(*it))
            {
                UserInfo* user=m_pKernel->m_mapIDToUserInfo.find(*it);
                m_pKernel->SendData(user->sockfd,szbuf,nlen);
            }
            ++it;
        }
    }
    //链表是否节点数为0  （全部退出）-》map去掉
    if(lst.size()==0){
        m_mapIDToRoomid.erase(rq->m_RoomId);
    }
    //更新成员列表
    m_mapIDToRoomid.insert(rq->m_RoomId,lst);
}
 //处理音频帧
void CLogic::AudioFrameRq(int clientfd, char *szbuf, int nlen)
{
    printf("clientfd:%d AudioFrameRq\n", clientfd);
    //拆包
    //拆包
    char *tmp=szbuf;
    //jump type
    tmp+=sizeof(int);
    int myuserid=*(int*)tmp;
    tmp+=sizeof(int);
    int roomid=*(int*)tmp;
    //看房间是否存在
    if(!m_mapIDToRoomid.IsExist(roomid))return;
    list<int> lst=m_mapIDToRoomid.find(roomid);
    //获取成员列表
    for(auto v=lst.begin();v!=lst.end();v++)
    {
        int userid=*v;
        if(userid==myuserid)continue;
    //看是否在线，转发
        if(!m_pKernel->m_mapIDToUserInfo.IsExist(userid))continue;
        UserInfo* userinfo=m_pKernel->m_mapIDToUserInfo.find(userid);
        m_pKernel->SendData(userinfo->m_audiofd,szbuf,nlen);
    }
}

void CLogic::VideoFrameRq(int clientfd, char *szbuf, int nlen)
{
    printf("clientfd:%d VideoFrameRq\n", clientfd);
    //拆包
    //拆包
    char *tmp=szbuf;
    //jump type
    tmp+=sizeof(int);
    int myuserid=*(int*)tmp;
    tmp+=sizeof(int);
    int roomid=*(int*)tmp;
    //看房间是否存在
    if(!m_mapIDToRoomid.IsExist(roomid))return;
    list<int> lst=m_mapIDToRoomid.find(roomid);
    //获取成员列表
    for(auto v=lst.begin();v!=lst.end();v++)
    {
        int userid=*v;
        if(userid==myuserid)continue;
    //看是否在线，转发
        if(!m_pKernel->m_mapIDToUserInfo.IsExist(userid))continue;
        UserInfo* userinfo=m_pKernel->m_mapIDToUserInfo.find(userid);
        m_pKernel->SendData(userinfo->m_videofd,szbuf,nlen);
    }
}

void CLogic::VideoRegister(int clientfd, char *szbuf, int nlen)
{
    printf("clientfd:%d VideoRegister\n", clientfd);
    //拆包
    STRU_VIDEO_REGISTER* rq=(STRU_VIDEO_REGISTER*)szbuf;
    //根据id 找到节点更新 fd
    int userid=rq->m_id;
    if(m_pKernel->m_mapIDToUserInfo.IsExist(userid))
    {
        UserInfo* info=m_pKernel->m_mapIDToUserInfo.find(userid);
        info->m_videofd=clientfd;
    }

}

void CLogic::AudioRegister(int clientfd, char *szbuf, int nlen)
{
    printf("clientfd:%d AudioRegister\n", clientfd);
    //拆包
    STRU_AUDIO_REGISTER* rq=(STRU_AUDIO_REGISTER*)szbuf;
    //根据id 找到节点更新 fd
    int userid=rq->m_id;
    if(m_pKernel->m_mapIDToUserInfo.IsExist(userid))
    {
        UserInfo* info=m_pKernel->m_mapIDToUserInfo.find(userid);
        info->m_audiofd=clientfd;
    }
}
