# Claude Code 用量接入说明

本文说明 `claude-bot` 如何接入真实 Claude Code 用量。

## 两套显示方式

| 方式 | 用途 | 数据来源 |
|---|---|---|
| `blueprint.yaml` | Home Assistant 一键切换 `loop`/`usage` 状态 | 静态内嵌 demo 图 |
| `lab/publish_usage.sh` | 读取真实用量，渲染并发布到 TC002 | `ccusage` 读取本地 Claude Code 日志 |

## 真实用量链路

```text
ccusage claude blocks/ weekly --json
  → lab/claude_usage_snapshot.js (提取百分比)
  → lab/render_usage.py (渲染 52×16 GIF)
  → mosquitto_pub (MQTT 发布)
  → TC002
```

## 使用方式

```bash
cd apps/mqtt/claude-bot

TC002_CLAUDE_5H_TOKEN_BUDGET=10000000 \
TC002_CLAUDE_WEEK_TOKEN_BUDGET=50000000 \
TC002_MQTT_HOST=127.0.0.1 \
TC002_MQTT_TOPIC=ulanzi_1bf6/custom/claude_bot \
bash lab/publish_usage.sh
```

## 参数语义

沿用 WebUI 原型的两个百分比：

| 参数 | 含义 |
|---|---|
| `five_hour_pct` | Claude Code 5 小时窗口用量百分比，范围 `0-100` |
| `week_pct` | Claude Code 周窗口用量百分比，范围 `0-100` |

显示含义是"5 小时窗口已使用 65%，周窗口已使用 42%"。

## 用量条颜色

| 范围 | 颜色 | 含义 |
|---|---|---|
| 0–64% | 绿色 | 正常 |
| 65–84% | 黄色 | 告警 |
| 85–100% | 红色 | 危险 |

## 脚本分工

| 脚本 | 职责 | 可独立使用 |
|---|---|---|
| `lab/claude_usage_snapshot.js` | 通过 `ccusage` 读取本机 Claude Code 日志，输出 JSON（含 `estimatedUsedPct`） | ✅ |
| `lab/render_usage.py` | 接收两个百分比，渲染 52×16 动画 GIF，输出 base64 | ✅ |
| `lab/publish_usage.sh` | 串联上述两步 + MQTT 发布，支持 `--loop` 轮询 | ✅ |

## 限制

| 限制 | 说明 |
|---|---|
| 不是官方额度 | `ccusage` 读取的是本地用量日志，不是 Claude 账号后台的官方剩余额度 |
| 需要手动校准 | token budget 需要用户自己按订阅和经验设置 |
| 依赖 ccusage | 需要 `npx ccusage` 可用（Node.js 环境） |
| 依赖 Pillow | 渲染需要 Python 3 + Pillow |

## 验收标准

| 条件 | 说明 |
|---|---|
| 数据来源可解释 | 百分比来自 `ccusage` 读取的本地用量日志 + 用户配置的 token budget |
| 数值可复现 | 同一时刻重复运行脚本，结果一致 |
| 画面会随数值变化 | 改变百分比后，TC002 上两条用量条的颜色和长度跟着变化 |
| MQTT 链路稳定 | TC002 重连或切回 Custom App 后能恢复最新画面（配合 `retain` 选项） |
