# Codex 用量接入说明

本文说明 `claude-bot` 如何接入真实 Codex 用量。

## 两套显示方式

| 方式 | 用途 | 数据来源 |
|---|---|---|
| `blueprint.yaml` | Home Assistant 一键切换 `loop`/`usage` 状态 | 静态内嵌 demo 图 |
| `lab/publish_usage.sh` | 读取真实花费，渲染并发布到 TC002 | `ccusage codex daily` |

## 真实用量链路

```text
ccusage codex daily --json
  → lab/claude_usage_snapshot.js (计算今日/本周花费 USD)
  → lab/render_usage.py (渲染 52×16 GIF)
  → mosquitto_pub (MQTT 发布)
  → TC002
```

## 使用方式

```bash
cd apps/mqtt/claude-bot

TC002_MQTT_HOST=10.19.1.58 bash lab/publish_usage.sh
```

无需任何手动预算配置。花费金额由 ccusage 按模型定价自动计算。

## 显示内容

| 标签 | 含义 |
|---|---|
| `5H` | 今日花费 (USD) |
| `W` | 本周花费 (USD) |

显示含义是"今天花了 $8，本周花了 $74"。花费金额已排除缓存命中的免费 token。

## 颜色阈值

| 范围 | 日 | 周 |
|---|---|---|
| 绿色 | < $8 | < $50 |
| 黄色 | $8–12 | $50–80 |
| 红色 | > $12 | > $80 |

阈值基于 Codex Plus ($200/月) 的日均/周均预算。

## 脚本分工

| 脚本 | 职责 | 可独立使用 |
|---|---|---|
| `lab/claude_usage_snapshot.js` | 通过 `ccusage codex daily` 读取花费，输出 JSON（含 `costUSD`） | ✅ |
| `lab/render_usage.py` | 接收花费金额，渲染 52×16 动画 GIF，输出 base64 | ✅ |
| `lab/publish_usage.sh` | 串联上述两步 + MQTT 发布，支持 `--loop` 轮询 | ✅ |

## 限制

| 限制 | 说明 |
|---|---|
| 花费是估算值 | ccusage 按 API 定价计算，不是官方账单 |
| 不是剩余额度 | Codex 不暴露公开的额度查询 API，无法显示"还剩多少" |
| 缓存已排除 | cacheReadTokens 不计入花费 |

## 验收标准

| 条件 | 说明 |
|---|---|
| 数据来源可解释 | 花费来自 `ccusage codex daily` 读取的本地用量日志 |
| 数值可复现 | 同一时刻重复运行脚本，结果一致 |
| 画面会随数值变化 | 花费变化后，TC002 上金额和进度条跟着变化 |
| MQTT 链路稳定 | TC002 重连或切回 Custom App 后能恢复最新画面 |