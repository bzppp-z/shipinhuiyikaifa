# 架构说明

## 组件

- `WeChat/` 是 Qt Widgets 桌面客户端。`Ckernel` 协调界面、账户和网络事件；`AudioApi/` 负责音频采集和播放，`VideoApi/` 负责摄像头、屏幕采集和视频帧处理。
- `Server/` 是 Linux C++ 服务端。`Block_Epoll_Net` 接收 TCP 数据并将处理任务放入线程池；`TcpKernel` 按消息类型分发到 `CLogic` 中的账户、房间和媒体处理函数。
- MySQL 保存账户。房间和在线连接关系存储在服务端进程内存中，重启后会丢失。

## 连接和媒体流

每个客户端向同一服务器端口建立三条 TCP 连接：控制连接处理账户和房间请求，另两条分别处理音频和视频。用户登录后，客户端在两条媒体连接上发送用户 ID 注册包；服务端把 socket 记录到用户信息中。媒体帧携带用户 ID 和房间号，服务端查找房间成员并将帧转发给其他成员对应的音频或视频 socket。

摄像头帧经过格式转换、缩放和 JPEG 编码后进入视频通道。屏幕截图复用同一视频消息路径。音频使用 Qt Multimedia 获取输入、通过 Speex 编解码后传输。客户端收到帧后按用户 ID 更新成员窗口或播放对象。

## 并发边界

服务端使用 `epoll`、线程池和 `EPOLLONESHOT` 处理连接；映射表内部使用互斥锁。客户端 TCP 接收使用独立线程，视频发送由 `SendVideoWork` 承接。OpenCV 采集对象在主线程和 worker 间访问，其打开、读取、关闭及线程退出仍需一致的生命周期管理。

## 主要入口

| 功能 | 文件 |
| --- | --- |
| 客户端启动与协调 | `WeChat/main.cpp`、`WeChat/ckernel.cpp` |
| 服务端启动 | `Server/src/main.cpp` |
| epoll 网络收发 | `Server/src/block_epoll_net.cpp` |
| 协议路由与业务处理 | `Server/src/TCPKernel.cpp`、`Server/src/clogic.cpp` |
| Windows TCP 客户端 | `WeChat/netapi/net/TcpClient.cpp` |
| 视频和屏幕采集 | `WeChat/VideoApi/` |
| 音频采集和播放 | `WeChat/AudioApi/` |
