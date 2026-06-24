#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────
# TC002 Claude Bot — Codex cost-based usage publisher
# ─────────────────────────────────────────────────────────────
# Reads Codex daily usage (via ccusage), renders a 52x16
# cost-based usage GIF, and publishes it to the TC002 Custom App
# MQTT topic.
#
# No budget configuration is needed — costs come from ccusage,
# which applies per-model pricing to non-cache tokens.
#
# Prerequisites:
#   - Python 3 with Pillow (pip install pillow)
#   - Node.js with npx (for ccusage)
#   - mosquitto_pub (brew install mosquitto)
#
# Environment variables:
#   TC002_MQTT_HOST                MQTT broker host (default: 127.0.0.1)
#   TC002_MQTT_PORT                MQTT broker port (default: 1883)
#   TC002_MQTT_TOPIC               Custom App topic (default: ulanzi_1bf6/custom/claude_bot)
#   TC002_DURATION                 payload duration in seconds (default: 3600)
#   TC002_TIMEZONE                 timezone for ccusage (default: Asia/Shanghai)
#
# Usage:
#   TC002_MQTT_HOST=10.19.1.58 bash lab/publish_usage.sh
#
#   # Loop mode: publish every 300 seconds
#   bash lab/publish_usage.sh --loop 300
# ─────────────────────────────────────────────────────────────

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# ── Config ──────────────────────────────────────────────────
MQTT_HOST="${TC002_MQTT_HOST:-127.0.0.1}"
MQTT_PORT="${TC002_MQTT_PORT:-1883}"
MQTT_TOPIC="${TC002_MQTT_TOPIC:-ulanzi_1bf6/custom/claude_bot}"
DURATION="${TC002_DURATION:-3600}"
TIMEZONE="${TC002_TIMEZONE:-Asia/Shanghai}"

# ── Help ────────────────────────────────────────────────────
if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
  cat <<EOF
Usage: bash lab/publish_usage.sh [--loop SECONDS]

Reads Codex daily cost from ccusage and displays it on TC002.
No token budget configuration is needed — costs are calculated
by ccusage using per-model pricing on non-cache tokens.

Optional env:
  TC002_MQTT_HOST    (default: 127.0.0.1)
  TC002_MQTT_PORT    (default: 1883)
  TC002_MQTT_TOPIC   (default: ulanzi_1bf6/custom/claude_bot)
  TC002_DURATION     (default: 3600)
  TC002_TIMEZONE     (default: Asia/Shanghai)

Options:
  --loop SECONDS     Run continuously, publishing every SECONDS
EOF
  exit 0
fi

# ── Check dependencies ───────────────────────────────────────
for cmd in python3 node npx mosquitto_pub; do
  if ! command -v "$cmd" &>/dev/null; then
    echo "❌ Missing dependency: $cmd" >&2
    exit 1
  fi
done

# ── Step 1: Read Codex usage ─────────────────────────────────
read_usage() {
  echo "📊 Reading Codex usage via ccusage..." >&2
  TC002_TIMEZONE="$TIMEZONE" node "$SCRIPT_DIR/claude_usage_snapshot.js"
}

# ── Step 2: Render usage GIF ─────────────────────────────────
render_gif() {
  local day_cost="$1"
  local week_cost="$2"
  echo "🎨 Rendering usage GIF (today=\$${day_cost} week=\$${week_cost})..." >&2
  python3 "$SCRIPT_DIR/render_usage.py" "$day_cost" "$week_cost"
}

# ── Step 3: Publish to MQTT ──────────────────────────────────
publish_mqtt() {
  local b64="$1"
  local payload
  payload=$(cat <<EOP
{"duration":$DURATION,"text":[],"image":[{"data":"data:image/gif;base64,$b64","position":[0,0]}],"draw":[]}
EOP
  )
  echo "📤 Publishing to $MQTT_TOPIC..." >&2
  mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" -t "$MQTT_TOPIC" -m "$payload"
  echo "✅ Published to $MQTT_TOPIC" >&2
}

# ── Main pipeline ────────────────────────────────────────────
run_once() {
  local snapshot day_cost week_cost

  snapshot=$(read_usage)

  day_cost=$(echo "$snapshot" | python3 -c "
import sys, json
d = json.load(sys.stdin)
t = d.get('today')
if t and t.get('costUSD') is not None:
    print(t['costUSD'])
else:
    print(0)
")

  week_cost=$(echo "$snapshot" | python3 -c "
import sys, json
d = json.load(sys.stdin)
w = d.get('week')
if w and w.get('costUSD') is not None:
    print(w['costUSD'])
else:
    print(0)
")

  echo "  today: \$${day_cost}  |  week: \$${week_cost}" >&2

  local b64
  b64=$(render_gif "$day_cost" "$week_cost")
  publish_mqtt "$b64"
}

# ── Entry ────────────────────────────────────────────────────
run_once

# Optional loop mode
if [[ "${1:-}" == "--loop" ]]; then
  LOOP_SECONDS="${2:-300}"
  echo "🔁 Looping every ${LOOP_SECONDS}s (Ctrl+C to stop)..." >&2
  while true; do
    sleep "$LOOP_SECONDS"
    run_once || echo "⚠️  One cycle failed, will retry next loop." >&2
  done
fi
