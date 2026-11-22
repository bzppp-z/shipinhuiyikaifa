# 多人音视频会议项目评测中台需求与设计

## Context

当前项目由 Windows Qt 客户端和 Linux C++ 服务端组成，服务端可以运行在 WSL2 或 Linux 虚拟机中。仓库目前没有自动化测试，协议、房间状态、媒体转发和断线清理主要依赖手工验证。客户端还依赖摄像头、声卡和本机 OpenCV 路径，直接对整个 Qt 界面做自动化测试成本高且不稳定。

本方案增加一个独立的 `qa/` 评测中台。它通过 TCP 模拟多个客户端连接真实服务端，验证协议和业务流程；同时为可抽取的 C++ 纯逻辑增加单元测试；最后补少量 Qt 客户端冒烟测试。测试结果按 Git 版本保存，支持发现回归、记录指标和在面试中展示完整测试闭环。

本文是第一版实现规格，默认先覆盖服务端和协议，不要求一次性把摄像头、麦克风和完整 GUI 自动化全部解决。

## Goals

- 建立可重复执行的协议、账号、房间和媒体转发回归测试。
- 模拟多个用户，验证真实 Linux 服务端的并发连接和房间转发行为。
- 用 C++ 单元测试覆盖协议编解码、包边界和房间状态等纯逻辑。
- 输出 JUnit/HTML 测试报告、服务端日志和版本信息。
- 记录通过率、响应延迟、媒体转发延迟、丢帧和断线清理结果。
- 为每个 Git 版本保留可比较的测试结果。
- 形成可写入简历、且能在面试中解释架构和取舍的工程成果。

## Non-Goals

- 第一版不重写现有 TCP 协议，不把媒体传输改成 UDP/WebRTC。
- 第一版不追求完整的 Qt GUI 自动化，也不依赖真实摄像头和声卡完成所有回归。
- 不把测试逻辑写进生产客户端窗口或服务端事件循环。
- 不虚构性能数字；简历中的指标必须来自实际测试记录。

## Recommended Approach

采用三层组合方案：

1. **C++ 单元测试**：用 GoogleTest 测试提取出来的 `FrameCodec`、`RoomManager` 和边界校验等纯逻辑。
2. **Python/pytest 集成回归**：实现一个轻量 TCP 协议客户端，模拟 A、B、C 多个用户，连接 WSL2 或 Linux 虚拟机中的服务端，验证真实登录、房间、音频和视频包路由。
3. **Qt 客户端冒烟测试**：只保留启动、配置读取、登录窗口和基础房间操作等少量稳定场景；媒体硬件流程先使用人工验证或合成帧验证。

这种拆分能同时证明“会写 C++ 测试”和“会做跨进程、跨机器的系统测试”。pytest 不负责替代 C++ 单测，而是作为服务端黑盒和协议集成测试的编排器。

## Alternatives Considered

| 方案 | 优点 | 缺点 | 决策 |
| --- | --- | --- | --- |
| 只做 C++ 单测 | 执行快，适合纯逻辑 | 覆盖不到真实 TCP、线程池、MySQL 和跨进程行为 | 不采用，作为其中一层 |
| 只用 pytest 测完整 Qt GUI | 能模拟用户操作 | 依赖 Windows、Qt、摄像头、声卡和窗口状态，容易 flaky | 不采用作为主方案 |
| pytest 协议回归 + C++ 单测 + 少量 Qt 冒烟 | 覆盖面和稳定性平衡，容易输出报告 | 需要维护两类测试 | **采用** |

## Runtime Environment

### Production-like test environment

- Windows 10/11：运行 Qt 客户端和 pytest。
- Qt 5.12.x、MinGW 32 位：与当前 `WeChat/WeChat.pro` 保持一致。
- WSL2 或 Linux 虚拟机：GCC C++11、qmake、make、pthread、MySQL client，运行 `Server`。本地开发优先使用 WSL2，跨机器或需要更强隔离时使用虚拟机。
- 服务端使用专用测试端口，例如 `18000`，避免占用日常开发的 `8000`；第一版也可以在独立 WSL2 环境中继续使用 `8000`。
- MySQL 使用独立的 `WeChat_test` 数据库和低权限测试账号，不使用源码中的 root 账号。
- Windows 主机能够访问 WSL2/虚拟机的服务端端口；测试开始前进行 TCP 连通性检查。

### QA dependencies

- Python 3.11 或更高版本，运行在 Windows 主机或 WSL2 中均可。
- `pytest`：测试发现、fixture、断言和参数化。
- `pytest-timeout`：避免网络场景永久阻塞。
- `pytest-html`：生成可读 HTML 报告；同时使用 pytest 内置 `--junitxml` 输出 CI 兼容报告。
- 可选 `pytest-cov`：统计 Python 测试工具本身的覆盖率，不把它误认为 C++ 服务端覆盖率。
- C++ 单测使用 GoogleTest 和 CMake；若目标机器安装成本过高，可使用 Catch2 或 doctest，但项目中只选择一种。

### Configuration

测试配置从环境变量或本地未提交文件读取：

```text
MEETING_SERVER_HOST=WSL2或虚拟机地址
MEETING_SERVER_PORT=18000
MEETING_TEST_DB=WeChat_test
MEETING_TEST_DB_USER=meeting_test
MEETING_TEST_DB_PASSWORD=本地密码
```

禁止把真实数据库口令写入测试代码、报告或 Git。测试开始前清理测试账号和房间数据，测试结束后再次清理。

## Repository Layout

```text
qa/
  README.md
  requirements.txt
  pytest.ini
  protocol_client/
    framing.py          # 4 字节长度前缀和当前协议结构
    packets.py          # 注册、登录、房间、媒体包构造
    client.py           # TCP 连接、收发和超时
  fixtures/
    users.py            # 测试账号和清理逻辑
    server.py           # 服务端连通性/启动检查
  tests/
    test_protocol.py
    test_account.py
    test_room.py
    test_media_forward.py
    test_disconnect.py
    test_concurrency.py
  reports/
  scripts/
    run_regression.ps1
    run_regression.sh
  cpp_unit/
    FrameCodecTest.cpp
    RoomManagerTest.cpp
    CMakeLists.txt
```

`qa/` 只依赖公开的协议行为，不直接引用 Qt 窗口对象。C++ 单测作为独立 CMake target 编译，不需要启动完整服务端。

## Architecture

```text
pytest runner
  ├── protocol client A ─┐
  ├── protocol client B ─┼── TCP ── WSL2/Linux VM: meeting server ── MySQL test DB
  └── protocol client C ─┘

C++ unit runner ── FrameCodec / RoomManager / boundary checks

optional Qt smoke ── Windows Qt client ── TCP ── WSL2/Linux VM: meeting server
```

### Protocol client

当前协议使用 4 字节包长度前缀，payload 首部是 `int` 类型消息号，结构体字段与客户端/服务端的 `packdef.h` 对齐。Python 工具在 `framing.py` 中集中处理长度前缀、完整读取、超时和最大包长检查，在 `packets.py` 中构造具体消息。

测试工具必须明确记录当前协议是本机 C++ 结构体格式，默认按当前 x86 环境的小端整数处理；它不能把该格式包装成通用跨平台协议。协议升级时先更新协议 fixture，再运行兼容性测试。

### C++ unit seams

现有 `CLogic` 同时依赖 `TcpKernel`、网络发送和 MySQL，直接单测成本较高。第一版只做必要的轻量抽取：

- `FrameCodec`：编码、解码、最小长度、最大长度和非法类型检查。
- `RoomManager`：创建房间、加入、退出、成员查询和重复操作处理。
- `MediaRouter` 或等价纯函数：根据房间成员和发送者 ID 计算转发目标。

网络、线程池、MySQL 仍由 pytest 集成测试验证，不在单测中伪造完整 epoll 环境。

## Test Matrix

### Unit tests

- 包类型和长度编码后再解码，字段保持一致。
- 半包、空包、负长度、超过最大长度和未知消息类型被拒绝。
- 房间创建成功；房间 ID 不重复。
- 加入存在/不存在房间的结果正确。
- 重复加入、重复退出和最后一个成员退出的状态正确。
- 媒体转发目标不包含发送者，包含同房间在线成员。

### Component and integration tests

- 注册成功、重复手机号、重复昵称。
- 正确密码登录、错误密码登录、用户不存在。
- 创建房间后加入和退出，成员列表与服务端返回一致。
- 音频注册和视频注册分别绑定正确 socket。
- A 发送合成视频帧，B/C 收到同一帧；A 不收到自己的帧。
- A 发送合成音频帧，B/C 收到；离开房间后不再收到。
- 连接关闭后房间成员和媒体 socket 映射被清理，或明确记录当前缺陷。

### Reliability and performance tests

- 多个虚拟用户同时登录、创建/加入房间。
- 多个用户持续发送固定大小的合成音视频帧。
- 发送拆分到多个 TCP write 的半包，确认服务端能够完整收包。
- 非法包、超大包和服务端主动断开不会导致测试进程挂死。
- 记录成功率、平均延迟、P95 延迟、转发延迟、丢帧数和连接数。

### Qt smoke tests

第一版只验证可稳定控制的流程：启动、读取 `config.ini`、登录窗口显示、登录成功后的主窗口、创建/加入/退出房间。摄像头和麦克风使用真实硬件的场景保留为手工 smoke；音视频转发由协议客户端使用合成帧自动验证。

## Test Data and Isolation

每个 pytest session 生成带版本和随机后缀的测试账号，fixture 负责注册和清理。每个测试尽量创建独立房间，避免依赖执行顺序。测试失败时保存服务端日志、用户 ID、房间 ID、请求类型和最后一次响应。

测试服务端应支持“清空测试状态后启动”。在当前房间状态保存在内存的前提下，回归脚本默认每轮重启服务端或使用唯一测试房间，避免上一次运行残留影响结果。

## Data Flow

1. runner 读取版本、服务端地址和测试数据库配置。
2. fixture 检查 WSL2/Linux 服务端的 TCP 端口和健康状态。
3. protocol client 建立控制、音频和视频连接，按当前协议发送请求。
4. 服务端执行账号、房间和媒体转发逻辑。
5. client 收集响应、媒体帧和时间戳，断言结果和超时。
6. pytest 输出 JUnit/HTML 报告，脚本保存服务端日志和 Git commit。
7. 版本比较脚本读取上一版本结果，标记新增失败和指标退化。

## Error Handling

- 连接超时：记录 WSL2/Linux 目标地址、端口和阶段，测试失败并关闭所有 socket。
- 收包超时：保存已收到的字节数和最近消息类型，禁止无限等待。
- 包长非法：客户端拒绝分配异常大小缓冲区，并记录为协议边界结果。
- 服务端提前断开：报告断开阶段和已建立的连接，继续执行 teardown。
- 数据库不可用：在 fixture 阶段标记环境失败，不把它误报成业务失败。
- 并发测试失败：保留用户、房间、消息序号和时间戳，支持单场景重放。

## Security and Boundaries

- 测试环境使用独立数据库、低权限账号和测试数据。
- 测试报告脱敏，不保存真实密码和生产账号。
- 所有输入都限制最大长度，验证消息类型和最小包长。
- 评测中台不绕过服务端业务规则；它只通过公开 TCP 协议进行黑盒验证。
- 生产代码仍存在原始结构体协议、SQL 字符串拼接和 MD5 等限制，评测平台记录这些风险，但第一版不借机扩大重构范围。

## Acceptance Criteria

- 单条命令能执行 C++ 单测和 pytest 回归，并在服务端不可用时快速给出环境错误。
- 至少覆盖登录、创建房间、加入房间、退出房间、媒体注册和媒体转发。
- 至少一个三用户场景能稳定重复执行 10 次。
- 报告包含测试名称、版本、耗时、失败原因和服务端日志路径。
- 任意失败测试都有明确超时和清理逻辑，不留下无法退出的进程或 socket。
- 实际运行后形成一组可引用的基线数据；简历只填写真实数据。

## Implementation Stages

### Stage 1: protocol regression MVP

实现 Python framing、packet builder、TCP client 和登录/房间场景。先不改 Qt 客户端，不做 GUI 自动化。

### Stage 2: media and failure regression

加入三条连接、音视频合成帧、房间转发、半包、非法包、断线和重连测试，输出日志与 JUnit/HTML 报告。

### Stage 3: C++ pure unit tests

从现有服务端抽取 `FrameCodec`、`RoomManager` 和媒体路由纯逻辑，加入 GoogleTest 和独立 CMake target。

### Stage 4: metrics and version comparison

记录 P95 延迟、转发延迟、丢帧和并发数；按 Git commit 保存结果，增加版本回归对比。

### Stage 5: Qt smoke

补充稳定的启动、配置、登录和房间操作冒烟测试。真实摄像头/麦克风测试作为手工设备矩阵，不作为每次自动回归的硬门槛。

## Resume and Interview Mapping

### Resume wording after implementation

> 设计并实现跨环境自动化评测中台：使用 Python/pytest 编写 TCP 协议虚拟客户端，模拟多用户完成账号、房间、音视频转发、半包、断线和并发回归；使用 GoogleTest 覆盖 C++ 协议编解码与房间状态单元逻辑，生成 JUnit/HTML 报告并按 Git 版本对比通过率、P95 延迟和媒体转发指标。

只有在对应测试确实完成并产生数据后，才填写具体并发数、通过率或延迟数字。

### Interview answer structure

**为什么做：** 原项目主要依靠手工测试，协议和房间逻辑修改后容易回归，Qt 硬件依赖又使完整 GUI 自动化不稳定。

**为什么分层：** C++ 单测适合快速验证纯逻辑；pytest 通过真实 TCP 验证服务端、线程池和协议行为；Qt 冒烟只覆盖少量用户入口，避免把硬件不确定性带进每次回归。

**如何模拟多人：** 每个虚拟用户维护控制、音频和视频三个连接，登录后发送媒体注册包，加入同一房间，再发送带用户 ID 和房间号的合成帧，验证其他成员能收到且发送者不会收到自己的帧。

**如何处理半包：** 测试客户端将长度前缀和 payload 拆成多次发送；接收端按长度循环读取，直到完整包到达，并设置最大包长和超时。

**如何定位失败：** 报告绑定 Git commit、测试用户、房间号、消息序号、耗时和服务端日志；先区分环境失败、协议失败、业务失败和性能退化，再复现单个场景。

**测试发现的工程问题：** 评测可暴露房间断线清理、媒体慢客户端背压、包长度限制、硬编码配置和结构体协议可移植性等问题；修复后用同一回归集验证改动没有引入新回归。

## Deferred Questions

- CI 使用 GitHub Actions、Jenkins 还是仅保留本地/WSL2 脚本，可在第一版稳定后决定；默认先提供 PowerShell 和 Bash 两个入口。
- Qt GUI 自动化工具可在冒烟场景稳定后选择；默认不引入重量级商业工具。
- C++ 单测框架默认 GoogleTest；如果目标 Linux 环境安装成本过高，再切换到 Catch2/doctest，不同时维护多套框架。

