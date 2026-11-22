param(
    [Parameter(Mandatory = $true)]
    [int]$ServerPid,
    [int]$Duration = 60,
    [int]$Interval = 1,
    [int]$Users = 5,
    [int]$Fps = 2,
    [string]$Python = "python",
    [switch]$UseWslMonitor,
    [string]$WslDistribution = "",
    [string]$OutputDirectory = "qa/reports"
)

$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$output = Join-Path $repo $OutputDirectory
New-Item -ItemType Directory -Force -Path $output | Out-Null
$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$loadOutput = Join-Path $output "load-monitored-$stamp.json"
$monitorOutput = Join-Path $output "resource-monitor-$stamp.json"
$monitorCsv = Join-Path $output "resource-monitor-$stamp.csv"
$monitorLog = Join-Path $output "resource-monitor-$stamp.log"
$monitorErrorLog = Join-Path $output "resource-monitor-$stamp.err.log"

function Quote-Arg([string]$value) {
    return '"' + ($value -replace '"', '\"') + '"'
}

$monitorScript = Join-Path $repo "qa\scripts\monitor_server.py"
if ($UseWslMonitor) {
    $wslRepo = (& wsl.exe -d $WslDistribution -- wslpath -a $repo).Trim()
    if (-not $wslRepo) { throw "Unable to translate repository path for WSL" }
    $wslMonitorOutput = (& wsl.exe -d $WslDistribution -- wslpath -a $monitorOutput).Trim()
    $wslMonitorCsv = (& wsl.exe -d $WslDistribution -- wslpath -a $monitorCsv).Trim()
    $monitorScript = "$wslRepo/qa/scripts/monitor_server.py"
    $distributionArg = if ($WslDistribution) { "-d $WslDistribution " } else { "" }
    $monitorArguments = "$distributionArg-- python3 $(Quote-Arg $monitorScript) --pid $ServerPid --duration $Duration --interval $Interval --output-json $(Quote-Arg $wslMonitorOutput) --output-csv $(Quote-Arg $wslMonitorCsv)"
    $monitorFile = "wsl.exe"
} else {
    $monitorArguments = "$(Quote-Arg $monitorScript) --pid $ServerPid --duration $Duration --interval $Interval --output-json $(Quote-Arg $monitorOutput) --output-csv $(Quote-Arg $monitorCsv)"
    $monitorFile = $Python
}

$monitorProcess = Start-Process -FilePath $monitorFile -ArgumentList $monitorArguments -WorkingDirectory $repo -RedirectStandardOutput $monitorLog -RedirectStandardError $monitorErrorLog -PassThru -NoNewWindow
try {
    $loadArgs = @(
        (Join-Path $repo "qa\scripts\run_load_test.py"),
        "--scenario", "all",
        "--users", $Users,
        "--duration", $Duration,
        "--fps", $Fps,
        "--output", $loadOutput
    )
    & $Python @loadArgs
    $loadExit = $LASTEXITCODE
} finally {
    if (-not $monitorProcess.HasExited) {
        $monitorProcess.WaitForExit(($Duration + 30) * 1000)
    }
}

if (-not $monitorProcess.HasExited) {
    Stop-Process -Id $monitorProcess.Id -Force
}
if ($loadExit -ne 0 -or $monitorProcess.ExitCode -ne 0) {
    exit 1
}
Write-Output "load report: $loadOutput"
Write-Output "resource report: $monitorOutput"
