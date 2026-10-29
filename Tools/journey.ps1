# Runs a scripted UX journey in a standalone -game process (no editor, no PIE) and prints its log.
# Usage: powershell -File Tools\journey.ps1 -Script "click:Buy a ticket|wait:1|shot:tickets" -Out C:\path\to\out
# Step reference: see README.md ("Scripted journeys").
param(
    [Parameter(Mandatory = $true)][string]$Script,
    [Parameter(Mandatory = $true)][string]$Out,
    [int]$TimeoutSeconds = 300
)

$Engine = "C:\Program Files\Epic Games\UE_5.8\Engine"
$Project = (Resolve-Path "$PSScriptRoot\..\MetroKiosk.uproject").Path
$Root = Split-Path $Project
$Dll = Join-Path $Root "Binaries\Win64\UnrealEditor-MetroKiosk.dll"

# A running editor is never touched. If the code changed and needs a rebuild, stop and say so.
$editor = Get-Process UnrealEditor -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowTitle -notmatch "Preview" }
$newestSource = Get-ChildItem (Join-Path $Root "Source") -Recurse -File | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not (Test-Path $Dll) -or $newestSource.LastWriteTime -gt (Get-Item $Dll).LastWriteTime) {
    if ($editor) {
        Write-Output "STOP: the binaries are older than the sources and the editor is running. Close the editor (or run Tools\build.ps1, which closes it) and try again."
        exit 2
    }
    Write-Output "Binaries are stale; building first..."
    & "$PSScriptRoot\build.ps1"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

New-Item -ItemType Directory -Force $Out | Out-Null
Remove-Item (Join-Path $Out "*") -Include *.png, *.txt -ErrorAction SilentlyContinue

$args = @("`"$Project`"", "-game", "-windowed", "-ResX=1917", "-ResY=1105", "-nosplash", "-nosound", "-log",
          "-KioskScript=`"$Script`"", "-KioskScriptOut=`"$Out`"")
# Let any previous -game process finish closing so its window cannot interfere.
While (Get-Process UnrealEditor -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowTitle -match 'Preview' -or $_.MainWindowTitle -eq 'MetroKiosk' }) { Start-Sleep -Seconds 1 }
Start-Sleep -Seconds 2
$proc = Start-Process -FilePath "$Engine\Binaries\Win64\UnrealEditor.exe" -ArgumentList $args -PassThru
if (-not $proc.WaitForExit($TimeoutSeconds * 1000)) {
    Write-Output "TIMEOUT after $TimeoutSeconds s; stopping the game process (the editor is not touched)."
    Stop-Process -Id $proc.Id -Force
}

$log = Join-Path $Out "log.txt"
if (Test-Path $log) { Get-Content $log -Encoding UTF8 } else { Write-Output "No log.txt was written." }
Write-Output "Output: $Out"
Get-ChildItem $Out | Select-Object -ExpandProperty Name
