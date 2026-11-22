# 五阶段测试实施计划

## 总体目标

为 Qt Windows 客户端与 Linux C++ 服务端建立可重复的自动化评测体系。测试平台独立放在 `qa/`，使用 Python/pytest 负责协议和集成测试，使用 GoogleTest 负责 C++ 纯逻辑单元测试，最后补充 Qt 客户端冒烟测试。

```text
环境配置
  -> 协议与账号
  -> 房间状态与 C++ 单测
  -> 音视频、断线与并发
  -> Qt 冒烟、版本回归与报告
```

## 阶段一：测试环境配置（已完成）

### 测试内容

- WSL2 Ubuntu、GCC、qmake、MySQL 环境。
- `WeChat` 数据库和 `t_user` 表。
- 本地测试数据库账号。
- 服务端编译、启动和 MySQL 连接。
- Windows 到 WSL2 `8000` 端口的连通性。
- Python 3.12 虚拟环境。
- pytest、pytest-html、pytest-timeout。
- HTML 和 JUnit XML 报告生成。

### 已创建文件

```text
qa/
├── README.md
├── requirements.txt
├── pytest.ini
└── tests/
    └── test_server_available.py
```

报告输出到 `qa/reports/`，虚拟环境 `.venv-qa/` 和报告文件不提交到 Git。

### 通过标准

- 服务端能够连接 MySQL。
- 服务端监听 `8000`。
- Windows 能连接 `127.0.0.1:8000`。
- 连通性测试通过并生成 HTML/JUnit 报告。

## 阶段二：协议和账号回归测试

### 测试内容

- 4 字节长度前缀编码和解码。
- 完整包、半包、空包、负长度和超大包。
- 未知消息类型和错误响应包校验。
- 注册成功。
- 重复手机号注册失败。
- 正确密码登录成功。
- 错误密码和不存在用户登录失败。
- 手机号、密码、昵称长度边界。

### 创建文件

```text
qa/
├── protocol_client/
│   ├── __init__.py
│   ├── framing.py
│   ├── packets.py
│   └── client.py
├── fixtures/
│   ├── accounts.py
│   └── cleanup.py
└── tests/
    ├── test_protocol.py
    └── test_account.py
```

### 通过标准

- 测试客户端能够按当前 C++ 结构体协议与服务端通信。
- 每次测试使用唯一账号，不依赖执行顺序。
- 所有网络操作都有超时和清理逻辑。
- 注册和登录场景能够重复执行。

## 阶段三：房间状态和 C++ 单元测试

### 测试内容

#### 房间集成测试

- 创建房间。
- 加入存在和不存在的房间。
- 多个用户加入同一房间。
- 重复加入。
- 退出房间。
- 最后一个成员退出。
- 成员加入和退出通知。

#### C++ 单元测试

从当前强耦合的 `CLogic` 中抽取少量纯逻辑：

- `FrameCodec`：协议编码、解码和边界检查。
- `RoomManager`：创建、加入、退出和成员查询。
- `MediaRouter`：计算房间内媒体转发目标。

### 创建文件

```text
Server/
├── include/
│   ├── FrameCodec.h
│   ├── RoomManager.h
│   └── MediaRouter.h
└── src/
    ├── FrameCodec.cpp
    ├── RoomManager.cpp
    └── MediaRouter.cpp

qa/
├── cpp_unit/
│   ├── CMakeLists.txt
│   ├── FrameCodecTest.cpp
│   ├── RoomManagerTest.cpp
│   └── MediaRouterTest.cpp
└── tests/
    ├── test_room.py
    └── test_boundary.py
```

### 通过标准

- 房间纯逻辑可以脱离 MySQL 和 epoll 单独测试。
- 三个虚拟用户能够重复完成创建和加入房间。
- 房间失败场景有明确结果。
- C++ 单元测试可以独立编译和运行。

## 阶段四：音视频转发、断线和并发测试

### 测试内容

#### 音视频转发

- 音频连接注册。
- 视频连接注册。
- A 发送视频帧，B/C 收到。
- A 发送音频帧，B/C 收到。
- 发送者不会收到自己的媒体帧。
- 不同房间之间不能收到媒体帧。
- 成员退出后不再收到媒体帧。

#### 异常场景

- TCP 半包。
- 非法包长度。
- 超大媒体包。
- 客户端突然断开。
- 单独断开音频或视频连接。
- 重复注册媒体连接。

#### 并发和性能

- 多用户同时登录和加入房间。
- 多用户持续发送合成音视频帧。
- 记录平均延迟、P95 延迟、转发延迟、丢帧和连接数。

测试使用固定内容的合成音频/视频数据，不依赖真实摄像头和麦克风。

### 创建文件

```text
qa/
├── protocol_client/
│   ├── media.py
│   └── metrics.py
├── tests/
│   ├── test_media_forward.py
│   ├── test_disconnect.py
│   └── test_concurrency.py
└── scripts/
    ├── run_media_test.py
    └── run_load_test.py
```

### 通过标准

- 真实服务端能够完成多人媒体转发。
- 测试不依赖硬件设备。
- 失败时保存用户 ID、房间号、消息序号和服务端日志。
- 性能指标来自实际运行记录。

## 阶段五：Qt 冒烟、版本回归和最终报告

### 测试内容

- Qt 客户端启动。
- 读取 `config.ini`。
- 登录窗口显示。
- 登录成功进入主窗口。
- 创建、加入和退出房间。
- 服务端地址错误时正确提示。

真实摄像头、麦克风和桌面采集保留为手工设备测试；音视频传输由阶段四的合成帧测试自动验证。

每次测试保存：

- Git commit。
- 测试时间。
- 服务端版本。
- 测试环境。
- 通过率。
- 失败用例。
- P95 延迟和媒体转发延迟。
- HTML/JUnit 报告路径。

### 创建文件

```text
qa/
├── scripts/
│   ├── run_smoke.ps1
│   ├── run_regression.ps1
│   ├── start_server.ps1
│   └── compare_reports.py
├── tests/
│   └── test_qt_smoke.py
├── reports/
│   └── history/
└── README.md
```

### 最终目录

```text
shipinhuiyikaifa/
├── Server/
│   ├── include/
│   │   ├── FrameCodec.h
│   │   ├── RoomManager.h
│   │   └── MediaRouter.h
│   └── src/
│       ├── FrameCodec.cpp
│       ├── RoomManager.cpp
│       └── MediaRouter.cpp
├── WeChat/
├── docs/
│   └── specs/
│       ├── 2026-11-22-meeting-evaluation-platform-design.md
│       └── 2026-11-22-five-stage-test-plan.md
├── qa/
│   ├── README.md
│   ├── requirements.txt
│   ├── pytest.ini
│   ├── protocol_client/
│   ├── fixtures/
│   ├── cpp_unit/
│   │   └── CMakeLists.txt
│   ├── tests/
│   ├── scripts/
│   └── reports/
└── .gitignore
```

## 简历表达

完成后可写成：

> 设计并实现跨环境自动化评测中台：使用 Python/pytest 编写 TCP 协议虚拟客户端，覆盖账号、房间、音视频转发、半包、断线和并发回归；使用 GoogleTest 覆盖 C++ 协议编解码与房间状态单元逻辑，输出 HTML/JUnit 测试报告，并按 Git 版本对比通过率、P95 延迟和媒体转发指标。

具体并发数、通过率和延迟只填写实际运行得到的数据。

## 当前验证基线

2026-11-22 在 WSL2 服务端和 Windows Python 3.12 测试机上完成：

- Python/pytest：15 passed，1 skipped。
- C++/GoogleTest：7/7 passed。
- 覆盖注册、登录、房间创建/加入、媒体转发、断线、并发、协议半包和 Qt 工程资源检查。
- Qt 可执行文件冒烟测试因未设置 `MEETING_QT_EXE` 自动跳过；设置该变量后可启用。
- HTML/JUnit/JSON 报告由 `qa/scripts/run_regression.ps1` 生成到本地 `qa/reports/`，报告文件不提交到 Git。

## 阶段六：压力与负载评测

在五阶段回归稳定后，增加独立的负载脚本 `qa/scripts/run_load_test.py`，不把压力场景混入每次快速回归。

### 场景

| 场景 | 规模 | 目标 |
| --- | ---: | --- |
| 登录压力 | 5 基线、20、50、100 用户 | 连接、线程池和数据库并发处理 |
| 房间压力 | 5 个房间、每房间 10 用户 | 房间隔离、成员状态和通知 |
| 媒体压力 | 每房间多个用户、合成视频帧、持续 60 秒 | 转发延迟、投递比例和阻塞发送 |

### 指标

- 成功/失败数量和成功率。
- 登录吞吐量。
- P95 延迟。
- 媒体发送帧数、接收帧数和投递比例。
- 媒体接收 P95 延迟。
- 测试持续时间和错误类型。

运行示例：

```powershell
python qa/scripts/run_load_test.py --scenario login --users 5
python qa/scripts/run_load_test.py --scenario login --users 20
python qa/scripts/run_load_test.py --scenario login --users 50
python qa/scripts/run_load_test.py --scenario rooms --rooms 5 --users-per-room 10
python qa/scripts/run_load_test.py --scenario media --users 5 --duration 60 --fps 2
```

压力结果写入 `qa/reports/load-result.json`，不进入 Git。100 用户场景作为可选扩展，先确认 50 用户结果稳定后再运行。

### 实际运行基线（2026-11-22）

| 场景 | 结果 |
| --- | --- |
| 登录 5 用户 | 5/5 成功，吞吐量 85.87/s，P95 56.11ms |
| 登录 20 用户 | 20/20 成功，吞吐量 99.48/s，P95 181.09ms |
| 登录 50 用户 | 50/50 成功，吞吐量 107.94/s，P95 299.41ms |
| 登录 100 用户 | 100/100 成功，吞吐量 112.49/s，P95 254.16ms |
| 房间 5×10 用户 | 50/50 成功，耗时 1.092s；登录、创建和加入均采用并发 worker |
| 媒体 5 用户×60 秒 | 发送 600 帧，接收 2400 帧，投递比例 1.0，P95 15.0ms |

登录压力使用 20 个 Python worker；媒体压力使用合成视频帧和 2 FPS，指标只代表当前本地 WSL2 环境，不代表互联网生产容量。

