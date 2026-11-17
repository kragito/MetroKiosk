# Runs every script in Tools\journeys (or the named ones) through Tools\journey.ps1 and prints PASS/FAIL per journey.
# Each journey writes its screenshots and log to Saved\Journeys\<name>. Script files hold one `|`-separated line.
# Usage: powershell -File Tools\run_journeys.ps1 [-Name a_tourist_gardens,b_typo_harbourfront] [-NoBuild]
param([string[]]$Name, [switch]$NoBuild)

$Root = Split-Path $PSScriptRoot
$Files = Get-ChildItem (Join-Path $PSScriptRoot "journeys") -Filter *.txt | Where-Object { -not $Name -or $Name -contains $_.BaseName }
$Failed = 0
foreach ($File in $Files) {
    $Out = Join-Path $Root ("Saved\Journeys\" + $File.BaseName)
    $Script = (Get-Content $File.FullName -Raw -Encoding UTF8).Trim()
    $Log = & "$PSScriptRoot\journey.ps1" -Script $Script -Out $Out -NoBuild:$NoBuild | Out-String
    $Done = ($Log -split "`n" | Where-Object { $_ -match "^DONE:" } | Select-Object -First 1)
    if ($Done -match "DONE: \d+ steps, 0 failed") { Write-Output "PASS  $($File.BaseName)  ($Out)" }
    else { Write-Output "FAIL  $($File.BaseName)  ($Out)"; $Failed++; $Log -split "`n" | Where-Object { $_ -match "FAIL|ABORT" } | Select-Object -First 5 }
}
exit $Failed
