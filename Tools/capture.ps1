# Runs the kiosk in -game mode, captures every screen to Saved/Captures, and exits on its own.
# Usage: powershell -File Tools\capture.ps1 [-ResX 1280 -ResY 720 -Out Saved\Captures_1280x720]
# Without arguments it captures at the design size, 1917x1105, into Saved\Captures.
param(
    [int]$ResX = 1917,
    [int]$ResY = 1105,
    [string]$Out
)
. "$PSScriptRoot\engine.ps1"
$Engine = Join-Path (Get-UnrealRoot) "Engine"
$Project = (Resolve-Path "$PSScriptRoot\..\MetroKiosk.uproject").Path
$Root = Split-Path $Project
if ($Out) { $Dir = if ([System.IO.Path]::IsPathRooted($Out)) { $Out } else { Join-Path $Root $Out } }
else { $Dir = Join-Path $Root "Saved\Captures" }
New-Item -ItemType Directory -Force $Dir | Out-Null
Remove-Item "$Dir\*.png" -ErrorAction SilentlyContinue
$GameArgs = @($Project, "-game", "-windowed", "-ResX=$ResX", "-ResY=$ResY", "-KioskCaptureAll=$Dir", "-KioskFakeTime=10:30", "-log", "-nosplash")
if ($ResX -ne 1917 -or $ResY -ne 1105) { $GameArgs += "-KioskCaptureRes=${ResX}x${ResY}" }
& "$Engine\Binaries\Win64\UnrealEditor.exe" @GameArgs | Out-Null
Get-ChildItem $Dir -Filter *.png | Select-Object -ExpandProperty Name
