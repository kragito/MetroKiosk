# Launches the editor for this project and waits until the MCP server answers on :8000.
$Engine = "C:\Program Files\Epic Games\UE_5.8\Engine"
$Project = (Resolve-Path "$PSScriptRoot\..\MetroKiosk.uproject").Path

Start-Process "$Engine\Binaries\Win64\UnrealEditor.exe" -ArgumentList "`"$Project`"", "-NoSplash"
for ($i = 0; $i -lt 150; $i++) {
    try {
        $body = '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-06-18","capabilities":{},"clientInfo":{"name":"probe","version":"1"}}}'
        Invoke-WebRequest -Uri "http://localhost:8000/mcp" -Method Post -Body $body -ContentType "application/json" -Headers @{ Accept = "application/json, text/event-stream" } -TimeoutSec 3 -UseBasicParsing | Out-Null
        Write-Output "EDITOR + MCP READY"
        exit 0
    } catch { Start-Sleep -Seconds 2 }
    if (-not (Get-Process UnrealEditor -ErrorAction SilentlyContinue)) { Write-Output "EDITOR EXITED"; exit 1 }
}
Write-Output "TIMEOUT waiting for MCP"
exit 1
