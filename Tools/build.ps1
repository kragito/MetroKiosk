# Closes any running editor for this project, then builds the editor target.
# Usage: powershell -File Tools\build.ps1 [-Relaunch]
param([switch]$Relaunch)

$Engine = "C:\Program Files\Epic Games\UE_5.8\Engine"
$Project = (Resolve-Path "$PSScriptRoot\..\MetroKiosk.uproject").Path

Get-Process UnrealEditor -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 2

& "$Engine\Build\BatchFiles\Build.bat" MetroKioskEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReloadFromIDE
$code = $LASTEXITCODE
if ($code -ne 0) { Write-Output "BUILD FAILED ($code)"; exit $code }
Write-Output "BUILD SUCCEEDED"

if ($Relaunch) { & "$PSScriptRoot\launch_editor.ps1" }
