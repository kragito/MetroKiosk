# Opens the kiosk inside the 3D concourse (the default game mode) in a standalone -game window. No editor, no PIE.
# Usage: powershell -File Tools\play_3d.ps1 [-NoBuild] [-StartAtKiosk] [-Shot C:\path\scene.png]
#   -StartAtKiosk  start stepped up to the kiosk, camera locked on it (-Kiosk3DStart=kiosk)
#   -Shot <png>    save a screenshot of the scene after a few seconds, then quit (-Kiosk3DShot=)
#   -Wide          with -Shot: take it from an overview position (-Kiosk3DShotView=wide)
#   -Hardware      with -Shot: close-up of the kiosk hardware (-Kiosk3DShotView=kiosk)
#   -ShotDir <dir> with -SelfTest: also save a screenshot at each stage (-Kiosk3DShotDir=)
#   -SelfTest      run the automatic walk / lock-on / cash purchase / card tap check (-Kiosk3DSelfTest); exits 1 unless it logs KIOSK3D SELFTEST PASS
param(
    [switch]$NoBuild,
    [switch]$StartAtKiosk,
    [string]$Shot = "",
    [switch]$Wide,
    [switch]$Hardware,
    [switch]$SelfTest,
    [string]$ShotDir = ""
)

. "$PSScriptRoot\engine.ps1"
$Engine = Join-Path (Get-UnrealRoot) "Engine"
$Project = (Resolve-Path "$PSScriptRoot\..\MetroKiosk.uproject").Path
$Root = Split-Path $Project
$Dll = Join-Path $Root "Binaries\Win64\UnrealEditor-MetroKiosk.dll"

$newestSource = Get-ChildItem (Join-Path $Root "Source") -Recurse -File | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($NoBuild -and (Test-Path $Dll)) {
    Write-Output "NoBuild: using the existing binaries."
} elseif (-not (Test-Path $Dll) -or $newestSource.LastWriteTime -gt (Get-Item $Dll).LastWriteTime) {
    Write-Output "Binaries are stale; building first..."
    & "$PSScriptRoot\build.ps1"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$args = @("`"$Project`"", "-game", "-windowed", "-ResX=1917", "-ResY=1105", "-nosplash", "-nosound")
if ($StartAtKiosk) { $args += "-Kiosk3DStart=kiosk" }
if ($Shot) {
    New-Item -ItemType Directory -Force (Split-Path -Parent $Shot) | Out-Null
    $args += "-Kiosk3DShot=`"$Shot`""
}
if ($Wide) { $args += "-Kiosk3DShotView=wide" }
if ($Hardware) { $args += "-Kiosk3DShotView=kiosk" }
$selfLog = Join-Path $Root "Saved\Logs\Kiosk3DSelfTest.log"
if ($SelfTest) {
    New-Item -ItemType Directory -Force (Split-Path -Parent $selfLog) | Out-Null
    Remove-Item $selfLog -ErrorAction SilentlyContinue
    $args += "-Kiosk3DSelfTest"
    $args += "-KioskCard=Adult"
    if ($ShotDir) {
        New-Item -ItemType Directory -Force $ShotDir | Out-Null
        $args += "-Kiosk3DShotDir=`"$ShotDir`""
    }
    $args += "-abslog=`"$selfLog`""
}
$proc = Start-Process -FilePath "$Engine\Binaries\Win64\UnrealEditor.exe" -ArgumentList $args -PassThru
if ($Shot) {
    if (-not $proc.WaitForExit(120 * 1000)) { Stop-Process -Id $proc.Id -Force; Write-Output "TIMEOUT" }
    if (Test-Path $Shot) { Write-Output "Screenshot: $Shot" } else { Write-Output "No screenshot was written." }
}
if ($SelfTest) {
    if (-not $proc.WaitForExit(240 * 1000)) { Stop-Process -Id $proc.Id -Force; Write-Output "TIMEOUT" }
    $lines = if (Test-Path $selfLog) { Get-Content $selfLog -Encoding UTF8 | Select-String "KIOSK3D SELFTEST" } else { @() }
    $lines | ForEach-Object { Write-Output $_.Line }
    if ($lines | Where-Object { $_.Line -match "KIOSK3D SELFTEST PASS" }) { exit 0 }
    Write-Output "Self-test did not pass. Log: $selfLog"
    exit 1
}
