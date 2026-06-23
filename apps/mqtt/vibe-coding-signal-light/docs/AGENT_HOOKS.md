# Claude Code / Codex 接入说明

这个红绿灯不是单纯的 Home Assistant 小玩具，它的主要场景是 Vibe Coding：当 Claude Code、Codex、CI 或本地脚本在运行时，TC002 能像桌面状态灯一样提示当前状态。

作者：王行知

## 推荐架构

```text
Code Agent hook
  -> Home Assistant 状态实体
  -> Blueprint
  -> MQTT broker
  -> TC002 Custom App
```

这里的 Code Agent 可以是 Claude Code、Codex，也可以是其它能执行 shell command 的工具。

## 状态约定

| 状态值 | 灯效 | 适合场景 |
|---|---|---|
| `off` | 全黑熄灭 | 没有任务运行 |
| `idle` | 绿灯 | 一轮任务正常结束，短暂提示成功 |
| `attention` | 黄灯闪烁 | Agent 正在运行、调用工具、需要关注 |
| `blocked` | 红灯闪烁 | 权限请求、命令失败、测试失败、流程阻塞 |
| `working` | 绿、黄、红循环 | 手动测试动画，不建议长期用于自动 hook |

## Home Assistant helper

推荐创建一个 `input_select` helper，例如：

```text
input_select.tc002_vibe_status
```

选项：

```text
off
idle
working
attention
blocked
```

Blueprint 监听这个实体，一旦状态变化，就把对应灯效发给 TC002。

## 用 Home Assistant API 更新状态

创建 Home Assistant Long-Lived Access Token 后，可以用 REST API 更新 helper：

```bash
curl -X POST "http://<HA_HOST>:8123/api/services/input_select/select_option" \
  -H "Authorization: Bearer <HA_LONG_LIVED_ACCESS_TOKEN>" \
  -H "Content-Type: application/json" \
  -d '{"entity_id":"input_select.tc002_vibe_status","option":"attention"}'
```

把 `option` 换成其它状态值即可。

## 推荐：使用 Home Assistant webhook

如果不想把 Home Assistant Token 写进 Claude Code / Codex hook，推荐使用 Home Assistant webhook。hook 只向本机 HA webhook 发送状态或完整 TC002 payload，再由 Home Assistant 自动化执行 `mqtt.publish`。

链路：

```text
Claude Code / Codex hook
  -> http://127.0.0.1:8125/api/webhook/tc002_vibe_signal_hook
  -> Home Assistant automation
  -> mqtt.publish
  -> TC002
```

Home Assistant 需要启用 `webhook:`，并添加类似自动化：

```yaml
- id: tc002_vibe_signal_webhook_to_mqtt
  alias: TC002 Vibe Signal Webhook to MQTT
  triggers:
    - platform: webhook
      webhook_id: tc002_vibe_signal_hook
      allowed_methods:
        - POST
      local_only: true
  actions:
    - service: mqtt.publish
      data:
        topic: "{{ trigger.json.topic | default('ulanzi_1bf6/custom/vibe_signal') }}"
        payload: "{{ trigger.json.payload | to_json }}"
        qos: 0
        retain: false
  mode: queued
```

本地测试：

```bash
curl -X POST "http://127.0.0.1:8125/api/webhook/tc002_vibe_signal_hook" \
  -H "Content-Type: application/json" \
  -d '{"topic":"ulanzi_1bf6/custom/vibe_signal","payload":{"duration":3,"text":[],"image":[],"draw":[{"df":[0,0,52,16,"#000000"]},{"dfc":[25,8,5,"#FFCB52"]}]}}'
```

如果 TC002 变成黄灯，说明 `HA webhook -> MQTT -> TC002` 这段链路已经通了。之后 Claude Code / Codex hook 只需要向这个 webhook 发送对应状态或 payload。

## 建议封装脚本

可以新建一个本地脚本，例如 `tc002-vibe-status.sh`：

```bash
#!/usr/bin/env bash
set -euo pipefail

STATUS="${1:-attention}"
HA_HOST="${HA_HOST:-http://127.0.0.1:8123}"
HA_ENTITY="${HA_ENTITY:-input_select.tc002_vibe_status}"

curl -sS -X POST "$HA_HOST/api/services/input_select/select_option" \
  -H "Authorization: Bearer $HA_TOKEN" \
  -H "Content-Type: application/json" \
  -d "{\"entity_id\":\"$HA_ENTITY\",\"option\":\"$STATUS\"}" >/dev/null
```

使用前设置：

```bash
export HA_HOST="http://127.0.0.1:8123"
export HA_TOKEN="你的 Home Assistant Long-Lived Access Token"
export HA_ENTITY="input_select.tc002_vibe_status"
```

手动测试：

```bash
./tc002-vibe-status.sh attention
./tc002-vibe-status.sh blocked
./tc002-vibe-status.sh idle
./tc002-vibe-status.sh off
```

## Claude Code 接入思路

Claude Code 支持在关键生命周期执行 hook。不同版本的配置格式可能会调整，建议以你当前 Claude Code 文档为准，但映射关系可以保持稳定：

| Claude Code 场景 | 建议状态 |
|---|---|
| 用户提交任务 | `attention` |
| 工具调用前 / 命令运行中 | `attention` |
| 权限请求 | `blocked` |
| 工具失败 / 测试失败 | `blocked` |
| 一轮任务结束 | `idle`，几秒后 `off` |

hook 命令示例：

```bash
/path/to/tc002-vibe-status.sh attention
/path/to/tc002-vibe-status.sh blocked
/path/to/tc002-vibe-status.sh idle
```

## Codex 接入思路

Codex 或其它支持 hook / shell command 的编程工具，也可以使用同一套脚本。

建议映射：

| Codex 场景 | 建议状态 |
|---|---|
| 开始处理用户请求 | `attention` |
| 正在运行工具、命令或修改文件 | `attention` |
| 需要用户确认、权限受限、执行失败 | `blocked` |
| 当前任务完成 | `idle`，几秒后 `off` |

## 多工具同时运行时

如果你经常同时开多个 Claude Code / Codex session，建议不要让每个 hook 简单地“结束就熄灯”。更稳的做法是维护一个本地状态文件，记录当前活跃 session：

```text
session A: attention
session B: blocked
session C: done
```

聚合规则建议：

```text
任意 session = blocked   -> 红灯
任意 session = attention -> 黄灯
没有活跃 session          -> 绿灯短暂提示，然后熄灭
```

这样一个任务结束时，不会误把另一个仍在运行的任务灯效关掉。

## 最小可用方案

如果你只是想先跑通，不需要复杂状态聚合：

1. Claude Code / Codex 开始运行时，执行 `tc002-vibe-status.sh attention`。
2. 遇到错误或权限时，执行 `tc002-vibe-status.sh blocked`。
3. 正常结束时，执行 `tc002-vibe-status.sh idle`。
4. 几秒后执行 `tc002-vibe-status.sh off`。

等确认真机效果稳定后，再把多 session 聚合逻辑补上。
