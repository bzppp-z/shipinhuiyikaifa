param(
    [string]$ServerHost = $env:MEETING_SERVER_HOST,
    [int]$ServerPort = $(if ($env:MEETING_SERVER_PORT) { [int]$env:MEETING_SERVER_PORT } else { 8000 }),
    [string]$ReportName = "regression"
)

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$python = Join-Path $repo ".venv-qa\Scripts\python.exe"
if (-not (Test-Path $python)) {
    throw "QA virtual environment not found: $python"
}
if (-not $ServerHost) { $ServerHost = "127.0.0.1" }

$env:MEETING_SERVER_HOST = $ServerHost
$env:MEETING_SERVER_PORT = "$ServerPort"
$reports = Join-Path $PSScriptRoot "..\reports"
New-Item -ItemType Directory -Force -Path $reports | Out-Null
$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$base = "{0}-{1}" -f $ReportName, $stamp
$html = Join-Path $reports "$base.html"
$xml = Join-Path $reports "$base.xml"

Push-Location $repo
try {
    & $python -m pytest -c qa\pytest.ini qa\tests `
        "--html=$html" --self-contained-html "--junitxml=$xml"
    $exitCode = $LASTEXITCODE
} finally {
    Pop-Location
}

$manifest = [ordered]@{
    generated_at = (Get-Date).ToString("o")
    git_commit = (git rev-parse HEAD).Trim()
    server_host = $ServerHost
    server_port = $ServerPort
    html_report = $html
    junit_report = $xml
    exit_code = $exitCode
}
$manifest | ConvertTo-Json | Set-Content -Encoding utf8 (Join-Path $reports "$base.json")
exit $exitCode
