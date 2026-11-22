# QA 回归测试

第一阶段先验证 Windows 测试机能连接 WSL2 中运行的会议服务端。

```powershell
python -m venv .venv-qa
.\.venv-qa\Scripts\Activate.ps1
python -m pip install -r qa\requirements.txt

pytest -c qa\pytest.ini qa\tests\test_server_available.py `
  --html=qa\reports\report.html `
  --self-contained-html `
  --junitxml=qa\reports\report.xml
```

默认连接 `127.0.0.1:8000`，也可以通过环境变量覆盖：

```powershell
$env:MEETING_SERVER_HOST = "127.0.0.1"
$env:MEETING_SERVER_PORT = "8000"
```

统一运行回归并按时间生成 HTML、JUnit XML 和 JSON 清单：

```powershell
powershell -ExecutionPolicy Bypass -File qa\scripts\run_regression.ps1
```

在 WSL2 中运行 C++ 单元测试：

```bash
bash qa/scripts/run_cpp_unit.sh
```

比较两个版本的 JUnit 结果：

```powershell
python qa\scripts\compare_reports.py qa\reports\current.xml --baseline qa\reports\baseline.xml
```

Qt 冒烟默认只检查工程资源；设置 `MEETING_QT_EXE` 后才会启动并立即关闭真实 Qt 可执行文件。

运行压力场景：

```powershell
python qa\scripts\run_load_test.py --scenario login --users 5
python qa\scripts\run_load_test.py --scenario login --users 20
python qa\scripts\run_load_test.py --scenario login --users 50
python qa\scripts\run_load_test.py --scenario login --users 100
python qa\scripts\run_load_test.py --scenario rooms --rooms 5 --users-per-room 10
python qa\scripts\run_load_test.py --scenario media --users 5 --duration 60 --fps 2
```

GitHub Actions 工作流位于 `.github/workflows/qa.yml`，每次 push 或 Pull Request 自动执行：

- GoogleTest C++ 单元测试。
- 本地协议编解码和纯 Python 测试。

依赖真实 MySQL、运行中服务端、房间状态、音视频 socket 和持续压力的测试，不在 GitHub 公共 runner 中执行，继续通过本地或专用 WSL2 环境运行：

```powershell
powershell -ExecutionPolicy Bypass -File qa\scripts\run_regression.ps1
python qa\scripts\run_load_test.py --scenario all
```

## 阶段七和阶段八

`.github/workflows/qa.yml` 在 push/PR 时运行 C++ GoogleTest、纯 Python 协议测试和 Qt 资源检查；真实 MySQL、运行中的服务端、房间/音视频集成和压力测试留在本地或专用 WSL 环境。

故障注入（需要服务端运行）：

```powershell
python qa/scripts/run_fault_injection.py --scenario all
```

资源监控和压力编排（WSL 服务端）：

```powershell
powershell -ExecutionPolicy Bypass -File qa/scripts/run_monitored_load.ps1 `
  -ServerPid <WSL_PID> -Duration 60 -Users 5 -Fps 2 -UseWslMonitor
```

资源报告写入 `qa/reports/resource-monitor*.json` 和 `.csv`。版本对比支持 JUnit、压力 JSON 和资源 JSON：

```powershell
python qa/scripts/compare_reports.py qa/reports/current.xml `
  --baseline qa/reports/baseline.xml `
  --current-load qa/reports/current-load.json `
  --baseline-load qa/reports/baseline-load.json `
  --current-resource qa/reports/current-resource.json `
  --baseline-resource qa/reports/baseline-resource.json
```

房间压力默认使用 10 个 setup worker，仍创建 5 个房间、每房间 10 用户；可通过 `--room-workers` 调整连接建立并发度：

```powershell
python qa/scripts/run_load_test.py --scenario rooms --rooms 5 --users-per-room 10 --room-workers 10
```
