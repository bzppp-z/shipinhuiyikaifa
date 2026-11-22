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
