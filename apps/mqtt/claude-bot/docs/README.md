# TC002 Claude Bot

[![导入到 Home Assistant](https://my.home-assistant.io/badges/blueprint_import.svg)](https://my.home-assistant.io/redirect/blueprint_import/?blueprint_url=https%3A%2F%2Fraw.githubusercontent.com%2FUlanziTechnology%2FUlanzi-U-Clock-TC002%2Fmain%2Fapps%2Fmqtt%2Fclaude-bot%2Fblueprint.yaml)

## 简介

这个应用把 Ulanzi U-Clock TC002 变成一个 Claude Bot 吉祥物显示器，显示 Clawd 吉祥物的循环动画或用量条。

作者：王行知

它的核心是一份 Home Assistant Blueprint。Blueprint 监听一个 Home Assistant 实体的状态变化，然后通过 MQTT 向 TC002 的 Custom App topic 发布显示内容。

支持的状态：

| 状态值 | 显示效果 |
|---|---|
| `loop` | Claude Bot 循环动画（idle, blink, look, hop, wiggle, sleep） |
| `usage` | Claude Bot 用量条（静态 demo，显示 5 小时和周用量） |

> 注意：Blueprint 版的 `usage` 是一张静态 demo GIF，只能证明显示链路通了。真实用量接入请使用 `lab/` 目录下的脚本，见下方「真实用量接入」一节。

## 预览

见 `preview/demo.gif`。

## 依赖

- Home Assistant，并已启用 MQTT 集成
- 一个 Home Assistant 和 TC002 都能访问的 MQTT broker
- TC002 已连接到同一个 MQTT broker
- TC002 上有一个 Custom App，名称需要和 MQTT topic 里的 `[APP_NAME]` 对应

## 安装

1. 在 Home Assistant 中导入 `blueprint.yaml`。
2. 创建或选择一个用于保存状态的实体。第一次测试推荐使用 `input_select` helper。
3. 用这个 Blueprint 创建自动化。
4. 把 `TC002 Custom App MQTT topic` 设置为你的设备 topic。

topic 格式：

```text
[PREFIX]/custom/[APP_NAME]
```

示例：

```text
ulanzi_1bf6/custom/claude_bot
```

`[PREFIX]` 通常是 MQTT 前缀加设备 MAC 地址后四位。比如 MQTT 前缀为 `ulanzi`，设备 MAC 后四位为 `1bf6`，则前缀通常为 `ulanzi_1bf6`。

## 与 Claude Code / Codex 搭配使用

如果只切换 `loop` / `usage` 两个状态，推荐链路：

```text
Claude Code / Codex hook
  -> 更新 Home Assistant 状态实体
  -> Blueprint 发布 MQTT
  -> TC002 显示 Claude Bot
```

第一次测试可以手动切换 `input_select` helper。确认 TC002 能正常显示后，再把 Claude Code、Codex 或其它编程助手的 hook 接进来。

建议事件映射：

| Code Agent 行为 | 建议状态值 | 显示效果 |
|---|---|---|
| Agent 空闲时 | `loop` | 循环动画 |
| 显示用量时 | `usage` | 用量条 |

## 真实用量接入

真实用量需要先把 Claude Code 用量转换成两个 `0-100` 的百分比，再生成新的 52x16 画面并发布到 TC002。

`lab/publish_usage.sh` 已经串联了完整链路：

```bash
cd apps/mqtt/claude-bot

# 单次发布：
TC002_CLAUDE_5H_TOKEN_BUDGET=10000000 \
TC002_CLAUDE_WEEK_TOKEN_BUDGET=50000000 \
TC002_MQTT_HOST=127.0.0.1 \
TC002_MQTT_TOPIC=ulanzi_1bf6/custom/claude_bot \
bash lab/publish_usage.sh

# 持续轮询（每 300 秒）：
TC002_CLAUDE_5H_TOKEN_BUDGET=10000000 \
TC002_CLAUDE_WEEK_TOKEN_BUDGET=50000000 \
bash lab/publish_usage.sh --loop 300
```

```text
ccusage (本地 Claude Code 用量日志)
  → lab/claude_usage_snapshot.js (提取 token 数 → 百分比)
  → lab/render_usage.py (渲染 52×16 动画 GIF)
  → mosquitto_pub (发布到 TC002 Custom App MQTT topic)
```

三个脚本彼此独立，可以分别在不同场景复用：

| 脚本 | 职责 |
|---|---|
| `lab/claude_usage_snapshot.js` | 读取用量，输出带百分比的 JSON |
| `lab/render_usage.py` | 接收百分比，输出 base64 GIF |
| `lab/publish_usage.sh` | 串联上述两步 + MQTT 发布 |

用量条颜色随百分比变化：

| 范围 | 颜色 | 含义 |
|---|---|---|
| 0–64% | 绿色 | 正常 |
| 65–84% | 黄色 | 告警 |
| 85–100% | 红色 | 危险 |

详见 `lab/README.md`。

如果使用 Home Assistant REST API，可以让 hook 调用下面的命令更新 helper：

```bash
curl -X POST "http://<HA_HOST>:8123/api/services/input_select/select_option" \
  -H "Authorization: Bearer <HA_LONG_LIVED_ACCESS_TOKEN>" \
  -H "Content-Type: application/json" \
  -d '{"entity_id":"input_select.tc002_claude_bot_status","option":"loop"}'
```

把最后的 `option` 改成 `usage`，就可以切换显示。

## 配置项

| 配置项 | 说明 |
|---|---|
| 状态实体 | 用来驱动 Claude Bot 的 Home Assistant 实体 |
| TC002 Custom App MQTT topic | 目标 topic，通常是 `[PREFIX]/custom/[APP_NAME]` |
| 显示时长 | 写入 TC002 Custom App payload 的 `duration` 值 |
| 保留 MQTT 消息 | 是否让 broker 保留最后一条消息，便于设备重连后恢复显示 |
| 状态值 | 每种显示效果对应的状态文本 |

## MQTT Payload

Blueprint 发布的是 TC002 Custom App JSON payload。图片通过 `image` 字段以内嵌 base64 data URL 的方式发送，不需要单独上传图片，也不需要外部图床。

示例结构：

```json
{
  "duration": 3600,
  "text": [],
  "image": [
    {
      "data": "data:image/gif;base64,...",
      "position": [0, 0]
    }
  ],
  "draw": []
}
```

## 真机测试

详细步骤见 `HOME_ASSISTANT_TESTING.md`。

快速检查流程：

1. 查看 TC002 当前 IP 和 MQTT 配置：

   ```bash
   curl http://<TC002_IP>/getBase
   curl http://<TC002_IP>/getMqttConfig
   ```

2. 确认 TC002 和 Home Assistant 使用同一个 MQTT broker。

3. 确认 topic。比如 MQTT 前缀为 `ulanzi`，设备 MAC 后四位为 `1bf6`，Custom App 名为 `claude_bot`，则 topic 为：

   ```text
   ulanzi_1bf6/custom/claude_bot
   ```

4. 确认 TC002 当前正在显示对应的 Custom App，例如 `claude_bot`。更新 Custom App 内容不一定会自动切换到该 App。

5. 在 Home Assistant 中依次切换状态实体为 `loop`、`usage`。

6. TC002 应分别显示循环动画和静态 demo 用量条。

底层 MQTT 链路可以先用 `draw` payload 快速验证：

```bash
mosquitto_pub -h <BROKER_HOST> -t ulanzi_1bf6/custom/claude_bot -m '{"duration":3600,"text":[],"image":[],"draw":[{"df":[0,0,52,16,"#000000"]}]}'
```

## 已知问题

- 当前版本把小图片直接内嵌在 `blueprint.yaml` 中，安装最简单，但后续修改图形时需要同步更新 base64。
- 当前 `usage` 画面是静态 demo 图，不能代表真实 Claude Code 剩余额度。
- TC002 收到 Custom App 更新后，不一定会自动切换到该 App。测试时建议先在设备上手动切到目标 Custom App，或者发布到当前正在显示的 App 名。
- 用手机相机录制 TC002 时，视频里可能出现频闪或横向滚动暗纹。这通常是 LED 点阵扫描刷新 / PWM 调光与相机快门不同步导致的，不一定代表肉眼看到的画面也在闪。
- TC002 的 MQTT topic 规则可能随官方固件变化，topic 已做成可配置项。

## 许可证

GPL-3.0-or-later。
