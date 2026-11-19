# ONE-OFF: generates Content/Maps/Concourse.umap from the layout in Tools/make_concourse_level.py (editor commandlet, Python, no window).
# The level is now the source of truth for the 3D scene. Do NOT rerun this unless you want to discard every change made in the editor;
# it refuses to overwrite an existing level unless you pass -Force.
# Usage: powershell -File Tools\make_concourse_level.ps1 [-Force]
param([switch]$Force)

. "$PSScriptRoot\engine.ps1"
$Engine = Join-Path (Get-UnrealRoot) "Engine"
$Project = (Resolve-Path "$PSScriptRoot\..\MetroKiosk.uproject").Path
$Root = Split-Path $Project
$Map = Join-Path $Root "Content\Maps\Concourse.umap"
$Script = Join-Path $PSScriptRoot "make_concourse_level.py"
$Log = Join-Path $Root "Saved\Logs\MakeConcourse.log"

if ((Test-Path $Map) -and -not $Force) {
    Write-Output "Content\Maps\Concourse.umap already exists and is the source of truth. Pass -Force to regenerate it (discards editor changes)."
    exit 1
}
if ($Force) { Remove-Item $Map -ErrorAction SilentlyContinue }
New-Item -ItemType Directory -Force (Split-Path $Log) | Out-Null
Remove-Item $Log -ErrorAction SilentlyContinue
Get-Process UnrealEditor -ErrorAction SilentlyContinue | Stop-Process -Force

& "$Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $Project "-run=pythonscript" "-script=$Script" -unattended -nullrhi -nosplash -nosound "-abslog=$Log" | Out-Null
$ok = (Test-Path $Log) -and (Select-String -Path $Log -Pattern "make_concourse_level: saved" -Quiet)
if ($ok -and (Test-Path $Map)) { Write-Output "Wrote $Map ($((Get-Item $Map).Length) bytes)"; exit 0 }
Write-Output "Level was not written. See $Log"
if (Test-Path $Log) { Select-String -Path $Log -Pattern "Error|Traceback" | Select-Object -First 20 | ForEach-Object { Write-Output $_.Line } }
exit 1
