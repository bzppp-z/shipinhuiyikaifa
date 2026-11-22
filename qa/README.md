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
