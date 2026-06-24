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
ccusage (read local Claude Code logs)
  → calculate 5h / week percentages
  → render a fresh 52x16 Claude Bot usage GIF
  → publish JSON payload to TC002 Custom App MQTT topic
```

### publish_usage.sh — one-shot or loop

```bash
cd apps/mqtt/claude-bot

# One shot:
TC002_CLAUDE_5H_TOKEN_BUDGET=10000000 \
TC002_CLAUDE_WEEK_TOKEN_BUDGET=50000000 \
bash lab/publish_usage.sh

# Loop every 300 seconds:
TC002_CLAUDE_5H_TOKEN_BUDGET=10000000 \
TC002_CLAUDE_WEEK_TOKEN_BUDGET=50000000 \
bash lab/publish_usage.sh --loop 300
```

Environment variables:

| Variable | Required | Default | Description |
|---|---|---|---|
| `TC002_CLAUDE_5H_TOKEN_BUDGET` | **yes** | — | Token budget for 5-hour window |
| `TC002_CLAUDE_WEEK_TOKEN_BUDGET` | **yes** | — | Token budget for weekly window |
| `TC002_MQTT_HOST` | no | `127.0.0.1` | MQTT broker host |
| `TC002_MQTT_PORT` | no | `1883` | MQTT broker port |
| `TC002_MQTT_TOPIC` | no | `ulanzi_1bf6/custom/claude_bot` | Custom App topic |
| `TC002_DURATION` | no | `3600` | Display duration in seconds |
| `TC002_TIMEZONE` | no | `Asia/Shanghai` | Timezone for ccusage |

### Pipeline steps

| Step | Script | What it does |
|---|---|---|
| 1. Read usage | `claude_usage_snapshot.js` | Runs `ccusage claude blocks --json` and `ccusage claude weekly --json`, outputs JSON with `fiveHour` / `week` token counts and `estimatedUsedPct` |
| 2. Render GIF | `render_usage.py` | Takes two percentages (0–100), generates a 52×16 animated usage GIF with mascot eye animation, outputs base64 |
| 3. Publish MQTT | `mosquitto_pub` (in `publish_usage.sh`) | Wraps the base64 GIF in a TC002 Custom App JSON payload and publishes to the MQTT topic |

### Usage bar color coding

| Range | Color | Meaning |
|---|---|---|
| 0–64% | Green | OK |
| 65–84% | Yellow | Warning |
| 85–100% | Red | Danger |

### render_usage.py — standalone renderer

```bash
# Print base64 to stdout:
python3 lab/render_usage.py 65 42

# Also write GIF to disk:
python3 lab/render_usage.py 65 42 --file /tmp/claude_bot_usage.gif
```

Requires Python 3 with Pillow (`pip install pillow`).

### claude_usage_snapshot.js — standalone usage reader

```bash
cd apps/mqtt/claude-bot
TC002_CLAUDE_5H_TOKEN_BUDGET=10000000 \
TC002_CLAUDE_WEEK_TOKEN_BUDGET=50000000 \
node lab/claude_usage_snapshot.js
```

The output includes:

| Field | Meaning |
|---|---|
| `fiveHour.totalTokens` | Latest Claude Code 5-hour block token count |
| `fiveHour.estimatedUsedPct` | Local estimate from configured token budget |
| `week.totalTokens` | Latest Claude Code weekly token count |
| `week.estimatedUsedPct` | Local estimate from configured token budget |

The budget values are calibration knobs. They are **not** official Claude account limits.

## Why two separate scripts instead of one monolith

- `claude_usage_snapshot.js` — JSON output, reusable by other tooling
- `render_usage.py` — pure renderer, reusable by WebUI / other publishers
- `publish_usage.sh` — thin orchestrator, easy to cron or hook into Claude Code lifecycle
