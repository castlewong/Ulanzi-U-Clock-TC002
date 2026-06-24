# Local Lab

This folder contains local testing and real-usage helpers. It is for development and may not need to be included in the final upstream PR unless maintainers want a reproducible lab setup.

## Quick start: MQTT chain test

1. Run Mosquitto on the Mac with Homebrew.
2. Configure TC002 to connect to the Mac LAN IP.
3. Publish a manual payload with `mosquitto_pub`.
4. Add Home Assistant after the broker-to-TC002 path works.

See `docs/README.md`.

## Real Claude Code usage pipeline

The MQTT Blueprint (`blueprint.yaml`) only provides a **static** demo `usage` image — it proves the display chain works but does not reflect real account quota.

The scripts in this directory complete the pipeline:

```text
ccusage codex daily (read Codex usage logs)
  → calculate today / week cost (USD)
  → render a fresh 52x16 Claude Bot usage GIF
  → publish JSON payload to TC002 Custom App MQTT topic
```

### publish_usage.sh — one-shot or loop

```bash
cd apps/mqtt/claude-bot

# One shot:
TC002_MQTT_HOST=10.19.1.58 bash lab/publish_usage.sh

# Loop every 300 seconds:
TC002_MQTT_HOST=10.19.1.58 bash lab/publish_usage.sh --loop 300
```

Environment variables:

| Variable | Required | Default | Description |
|---|---|---|---|
| `TC002_MQTT_HOST` | no | `127.0.0.1` | MQTT broker host |
| `TC002_MQTT_PORT` | no | `1883` | MQTT broker port |
| `TC002_MQTT_TOPIC` | no | `ulanzi_1bf6/custom/claude_bot` | Custom App topic |
| `TC002_DURATION` | no | `3600` | Display duration in seconds |
| `TC002_TIMEZONE` | no | `Asia/Shanghai` | Timezone for ccusage |

### Pipeline steps

| Step | Script | What it does |
|---|---|---|
| 1. Read usage | `claude_usage_snapshot.js` | Runs `ccusage codex daily --json`, outputs JSON with today/week cost (USD) and actual token counts |
| 2. Render GIF | `render_usage.py` | Takes two costs (USD), generates a 52×16 animated usage GIF with mascot eye animation, outputs base64 |
| 3. Publish MQTT | `mosquitto_pub` (in `publish_usage.sh`) | Wraps the base64 GIF in a TC002 Custom App JSON payload and publishes to the MQTT topic |

### Usage bar color coding

| Cost | Color | Meaning |
|---|---|---|
| < $8 (day) / < $50 (week) | Green | OK |
| $8–12 (day) / $50–80 (week) | Yellow | Warning |
| > $12 (day) / > $80 (week) | Red | Danger |

Thresholds are based on Codex Plus ($200/month ≈ $7/day, $50/week).

### render_usage.py — standalone renderer

```bash
# Print base64 to stdout:
python3 lab/render_usage.py 8 74

# Also write GIF to disk:
python3 lab/render_usage.py 8 74 --file /tmp/claude_bot_usage.gif
```

Requires Python 3 with Pillow (`pip install pillow`).

### claude_usage_snapshot.js — standalone usage reader

```bash
cd apps/mqtt/claude-bot
node lab/claude_usage_snapshot.js
```

No environment variables required. The output includes:

| Field | Meaning |
|---|---|
| `today.costUSD` | Today's Codex cost in USD |
| `week.costUSD` | This week's Codex cost in USD |
| `today.actualTokens` | Non-cache tokens used today |
| `week.actualTokens` | Non-cache tokens used this week |

Costs are calculated by ccusage using per-model API pricing. Cache-read tokens are excluded.

## Why three separate scripts instead of one monolith

- `claude_usage_snapshot.js` — JSON output, reusable by other tooling
- `render_usage.py` — pure renderer, reusable by WebUI / other publishers
- `publish_usage.sh` — thin orchestrator, easy to cron or hook into Codex lifecycle
