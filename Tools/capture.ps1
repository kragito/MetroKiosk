# Runs the kiosk in -game mode, captures every screen to Saved/Captures, and exits on its own.
$Project = (Resolve-Path "$PSScriptRoot\..\MetroKiosk.uproject").Path
$Dir = Join-Path (Split-Path $Project) "Saved\Captures"
New-Item -ItemType Directory -Force $Dir | Out-Null
Remove-Item "$Dir\*.png" -ErrorAction SilentlyContinue
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" $Project -game -windowed -ResX=1917 -ResY=1105 "-KioskCaptureAll=$Dir" -log -nosplash | Out-Null
Get-ChildItem $Dir -Filter *.png | Select-Object -ExpandProperty Name
