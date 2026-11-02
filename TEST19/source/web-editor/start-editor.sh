#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"
python3 -m http.server 8765 --bind 127.0.0.1 &
pid=$!
trap 'kill $pid 2>/dev/null || true' EXIT
python3 - <<'PY'
import webbrowser
webbrowser.open('http://127.0.0.1:8765/')
PY
wait $pid
