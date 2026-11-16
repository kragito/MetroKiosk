# Runs the MetroKiosk automation tests (MetroKiosk.Search.*, MetroKiosk.Loc.*, MetroKiosk.Session.*) headlessly and prints PASS/FAIL per test.
# Usage: powershell -File Tools\run_tests.ps1 [-Filter MetroKiosk.Loc]
# Needs up-to-date binaries (run Tools\build.ps1 first) and no running editor for this project.
param([string]$Filter = "MetroKiosk")

. "$PSScriptRoot\engine.ps1"
$Engine = Join-Path (Get-UnrealRoot) "Engine"

$Project = (Resolve-Path "$PSScriptRoot\..\MetroKiosk.uproject").Path
$Root = Split-Path $Project
$Log = Join-Path $Root "Saved\Logs\AutomationRun.log"
New-Item -ItemType Directory -Force (Split-Path $Log) | Out-Null
Remove-Item $Log -ErrorAction SilentlyContinue

& "$Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $Project `
    "-ExecCmds=Automation RunTests $Filter; Quit" -unattended -nullrhi -nosplash -nosound "-abslog=$Log" | Out-Null

if (-not (Test-Path $Log)) { Write-Output "No log was written."; exit 2 }
$lines = Get-Content $Log -Encoding UTF8
$results = $lines | Where-Object { $_ -match "Test Completed\. Result=\{(Success|Passed|Fail|Failed)\}" }
$failed = 0
foreach ($line in $results) {
    if ($line -match "Result=\{(Success|Passed)\}\s+Name=\{.*?\}\s+Path=\{(.*?)\}") { Write-Output "PASS  $($Matches[2])" }
    elseif ($line -match "Result=\{(Fail|Failed)\}\s+Name=\{.*?\}\s+Path=\{(.*?)\}") { Write-Output "FAIL  $($Matches[2])"; $failed++ }
}
if (-not $results) { Write-Output "No test results found in $Log"; exit 2 }
$lines | Where-Object { $_ -match "Error:|LogAutomationController: Error" } | Select-Object -First 40 | ForEach-Object { Write-Output $_ }
exit $failed
