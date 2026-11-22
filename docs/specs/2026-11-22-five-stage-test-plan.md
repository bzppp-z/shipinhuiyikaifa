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


## 阶段七：CI 自动运行的快速质量门禁

阶段七把不依赖外部服务的检查放进 GitHub Actions，每次 push 和 Pull Request 自动执行：

- C++/GoogleTest 单元测试，验证协议编解码、房间状态和媒体路由纯逻辑；
- 纯 Python 协议编解码测试，验证定长字段、边界长度、半包重组和响应解析；
- Qt 工程资源检查，确认 `.pro`、头文件和源文件仍然完整。

CI 不启动真实 MySQL、WSL 服务端、房间集成、音视频 socket 或压力场景。这些场景依赖本机服务和可控数据，继续由阶段三至六的本地回归脚本执行，避免公共 runner 把环境问题误报成代码回归。

### 阶段七文件

```text
.github/workflows/qa.yml       # push/PR 触发的 CI
qa/cpp_unit/                   # GoogleTest 测试和 CMake 配置
qa/tests/test_protocol.py     # 纯 Python 协议测试
qa/tests/test_compare_reports.py
qa/reports/                   # 本地报告（被 .gitignore 忽略）
```

### 阶段七验收结果

本地按 CI 相同筛选条件运行：4 passed，1 skipped，11 deselected；C++ GoogleTest 7/7 passed。GitHub Actions 使用 Ubuntu runner、GCC、CMake、GoogleTest 和 Python 3.12，报告以 artifact 形式保留。

## 阶段八：故障注入、服务端资源监控和版本对比

阶段八在真实服务端环境运行，用于回答“异常输入后服务是否仍然可用”和“新版本是否退化”两个问题。

### 故障注入

`qa/scripts/run_fault_injection.py` 通过公开 TCP 协议发送以下输入：负数包长、超过 1 MiB 的包长、未知消息类型、已知消息类型但字段截断、半包后断开、控制连接突然关闭、媒体连接突然关闭、重复媒体注册和非法房间号。每个场景结束后都会建立新的 TCP 连接；新连接失败就判定服务端监听器被故障拖垮。报告写入 `qa/reports/fault-injection.json`。

```powershell
python qa/scripts/run_fault_injection.py --scenario all
```

对应的 pytest 用例在 `qa/tests/test_fault_injection.py`，标记为 `integration`，不会进入阶段七的纯 Python CI 子集。

### 服务端资源监控

`qa/scripts/monitor_server.py` 读取 Linux/WSL `/proc`，记录进程存活、CPU、RSS、虚拟内存、线程数和 TCP 连接数；Windows 原生进程使用 PowerShell 查询同一组指标。结果同时输出 JSON 和 CSV：

```text
qa/reports/resource-monitor.json
qa/reports/resource-monitor.csv
```

对 WSL 服务端，先在 WSL 中取得服务端 PID，再让监控脚本在 WSL 中运行；`run_monitored_load.ps1 -UseWslMonitor` 可以编排监控和阶段六压力脚本。

```powershell
powershell -ExecutionPolicy Bypass -File qa/scripts/run_monitored_load.ps1 `
  -ServerPid <WSL_PID> -Duration 60 -Users 5 -Fps 2 -UseWslMonitor
```

### 版本对比

`qa/scripts/compare_reports.py` 保留 JUnit 失败数比较，并增加压力和资源阈值：成功率下降、P95 上升、媒体投递比例下降、新增错误类型、CPU/RSS/线程/TCP 峰值上升都会报告为 regression。默认阈值分别为 P95 25%、成功率 2 个百分点、投递比例 2 个百分点和资源峰值 50%，可通过命令行覆盖。

```powershell
python qa/scripts/compare_reports.py qa/reports/current.xml `
  --baseline qa/reports/baseline.xml `
  --current-load qa/reports/load-monitored-current.json `
  --baseline-load qa/reports/load-monitored-baseline.json `
  --current-resource qa/reports/resource-monitor-current.json `
  --baseline-resource qa/reports/resource-monitor-baseline.json
```

### 阶段八文件

```text
qa/fault_injection.py                    # 可复用故障探针
qa/scripts/run_fault_injection.py        # 故障注入入口和 JSON 报告
qa/tests/test_fault_injection.py         # 集成回归用例
qa/scripts/monitor_server.py             # /proc/PowerShell 资源采样
qa/scripts/run_monitored_load.ps1        # 监控和压力测试编排
qa/scripts/compare_reports.py            # JUnit/压力/资源版本门禁
qa/tests/test_compare_reports.py         # 比较算法纯单测
```

### 阶段八验收标准

- 任一故障注入场景后，新的登录连接仍可建立，服务端进程不退出；
- 压力期间资源报告包含至少两个采样点，并记录峰值和进程退出状态；
- 当前版本相对基线没有超过阈值的成功率、P95、投递比例或资源退化；
- 所有 JSON、CSV、HTML、JUnit 报告均写入 `qa/reports/`，可按 Git commit 保存到外部制品库。

### 阶段八实际运行记录（2026-11-22）

最终服务端二进制在 WSL2 中完成以下验证：

| 检查 | 结果 |
| --- | --- |
| 故障注入 9 场景 | 9/9 通过；每个场景后新连接均可建立 |
| C++ GoogleTest | 7/7 通过 |
| 纯 Python CI 子集 | 6 passed，1 skipped（Qt 可选冒烟） |
| 房间 5×10（10 个 setup worker） | 50/50 成功，0.488s |
| 媒体 5 用户×60 秒 | 发送 600 帧，接收 2400 帧，投递比例 1.0，P95 7.57ms |
| 资源监控 | 16 个采样点，进程未退出，峰值 RSS 11940 kB、线程 12 |
| JUnit/压力/资源版本比较 | no regressions detected |

故障注入期间发现并修复了 `SIGPIPE`、截断包越界、异步 fd 复用污染响应和突发连接接收问题；这些修复已经包含在服务端源代码中。房间压力脚本增加 `--room-workers`，默认 10，避免测试机一次性建立 50 个连接造成 WSL 网络层瞬时拒绝，同时仍保持 50 个用户和 5 个房间的并发创建/加入验证。
