"""Minimal CLI bridge to the Unreal Editor MCP server (ModelContextProtocol plugin).

Usage:
  python ue_mcp.py toolsets
  python ue_mcp.py describe <ToolsetName>
  python ue_mcp.py call <ToolsetName> <tool_name> '<json args>' [--out image.png]

Any base64 image in the result (e.g. SlateInspector Screenshot, CaptureEditorImage)
is written to --out (default: mcp_capture.png) instead of being printed.

Requires the editor to be running with the MetroKiosk project (server on :8000/mcp).
"""
import base64
import json
import os
import sys
import urllib.request

URL = "http://localhost:8000/mcp"
HEADERS = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream"}


def _post(payload, session=None):
    headers = dict(HEADERS)
    if session:
        headers["Mcp-Session-Id"] = session
    req = urllib.request.Request(URL, json.dumps(payload).encode(), headers)
    with urllib.request.urlopen(req, timeout=600) as resp:
        body = resp.read().decode("utf-8", "replace")
        return resp.headers.get("Mcp-Session-Id"), (json.loads(body) if body.strip() else None)


def _session():
    sid, _ = _post({"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {
        "protocolVersion": "2025-06-18", "capabilities": {}, "clientInfo": {"name": "ue_mcp_cli", "version": "1"}}})
    _post({"jsonrpc": "2.0", "method": "notifications/initialized"}, sid)
    return sid


def call(name, arguments):
    sid = _session()
    _, res = _post({"jsonrpc": "2.0", "id": 2, "method": "tools/call",
                    "params": {"name": name, "arguments": arguments}}, sid)
    return res


def _extract_image(obj):
    """Find the first {"mimeType": ..., "data": <base64>} dict anywhere in obj."""
    if isinstance(obj, dict):
        if isinstance(obj.get("data"), str) and "mimeType" in obj:
            return obj["data"]
        for v in obj.values():
            found = _extract_image(v)
            if found:
                return found
    elif isinstance(obj, list):
        for v in obj:
            found = _extract_image(v)
            if found:
                return found
    return None


def _print_item(item, out_path):
    if item.get("type") == "image":
        data = item.get("data")
    else:
        text = item.get("text", json.dumps(item))
        try:
            data = _extract_image(json.loads(text))
        except (ValueError, TypeError):
            data = None
        if not data:
            print(text)
            return
    with open(out_path, "wb") as f:
        f.write(base64.b64decode(data))
    print(f"image saved: {os.path.abspath(out_path)}")


def main(argv):
    sys.stdout.reconfigure(encoding="utf-8")
    out_path = "mcp_capture.png"
    if "--out" in argv:
        i = argv.index("--out")
        out_path = argv[i + 1]
        argv = argv[:i] + argv[i + 2:]
    if len(argv) < 2:
        print(__doc__)
        return 1
    cmd = argv[1]
    if cmd == "toolsets":
        res = call("list_toolsets", {})
    elif cmd == "describe":
        res = call("describe_toolset", {"toolset_name": argv[2]})
    elif cmd == "call":
        args = json.loads(argv[4]) if len(argv) > 4 else {}
        res = call("call_tool", {"toolset_name": argv[2], "tool_name": argv[3], "arguments": args})
    else:
        print(__doc__)
        return 1
    if res and "result" in res:
        for item in res["result"].get("content", []):
            _print_item(item, out_path)
        if res["result"].get("isError"):
            return 2
    else:
        print(json.dumps(res, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
