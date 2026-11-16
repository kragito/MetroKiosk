# Shared helper, dot-sourced by the other Tools scripts: . "$PSScriptRoot\engine.ps1"
# Get-UnrealRoot returns the folder that contains Engine\ (UE 5.8). Lookup order:
#   1. $env:UE_ROOT (ignored with a warning if it has no engine)
#   2. the launcher's install record: registry HKLM:\SOFTWARE\EpicGames\Unreal Engine\5.8, then LauncherInstalled.dat
#   3. C:\Program Files\Epic Games\UE_5.8
# A candidate is accepted only if Engine\Build\BatchFiles\Build.bat exists under it.
# If none works it prints one line naming UE_ROOT (to the host, not the pipeline) and exits 3.
# The optional parameters exist so the lookup can be tested against non-existent locations.
function Get-UnrealRoot {
    param(
        [string]$RegistryPath = "HKLM:\SOFTWARE\EpicGames\Unreal Engine\5.8",
        [string]$LauncherDat = "$env:ProgramData\Epic\UnrealEngineLauncher\LauncherInstalled.dat",
        [string]$Fallback = "C:\Program Files\Epic Games\UE_5.8"
    )
    $marker = "Engine\Build\BatchFiles\Build.bat"

    if ($env:UE_ROOT) {
        if (Test-Path -LiteralPath (Join-Path $env:UE_ROOT $marker)) { return $env:UE_ROOT.TrimEnd('\') }
        Write-Host "UE_ROOT is set to $env:UE_ROOT but no engine was found there; ignoring it."
    }

    $candidates = New-Object System.Collections.Generic.List[string]
    try {
        $reg = Get-ItemProperty -Path $RegistryPath -ErrorAction Stop
        if ($reg.InstalledDirectory) { $candidates.Add([string]$reg.InstalledDirectory) }
    } catch { }
    try {
        if (Test-Path -LiteralPath $LauncherDat) {
            $entry = (Get-Content -LiteralPath $LauncherDat -Raw | ConvertFrom-Json).InstallationList | Where-Object { $_.AppName -eq "UE_5.8" } | Select-Object -First 1
            if ($entry -and $entry.InstallLocation) { $candidates.Add([string]$entry.InstallLocation) }
        }
    } catch { }
    $candidates.Add($Fallback)

    foreach ($c in $candidates) {
        if (Test-Path -LiteralPath (Join-Path $c $marker)) { return $c.TrimEnd('\') }
    }
    Write-Host "Unreal Engine 5.8 not found: set UE_ROOT to the folder that contains Engine (for example C:\Program Files\Epic Games\UE_5.8)."
    exit 3
}
