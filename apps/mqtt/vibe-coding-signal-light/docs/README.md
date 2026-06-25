# TC002 Vibe Coding 红绿灯

[![导入到 Home Assistant](https://my.home-assistant.io/badges/blueprint_import.svg)](https://my.home-assistant.io/redirect/blueprint_import/?blueprint_url=https%3A%2F%2Fraw.githubusercontent.com%2FUlanziTechnology%2FUlanzi-U-Clock-TC002%2Fmain%2Fapps%2Fmqtt%2Fvibe-coding-signal-light%2Fblueprint.yaml)

## 简介

这个应用把 Ulanzi U-Clock TC002 变成一个桌面状态红绿灯，适合显示编程助手、CI 任务、本地脚本、自动化流程等工作状态。

作者：王行知

它的核心是一份 Home Assistant Blueprint。Blueprint 监听一个 Home Assistant 实体的状态变化，然后通过 MQTT 向 TC002 的 Custom App topic 发布显示内容。

这个应用真正面向的是 Vibe Coding 场景：Claude Code、Codex、CI、本地脚本等工具在开始运行、调用工具、遇到权限/错误、运行结束时，把状态写入 Home Assistant；Home Assistant 再通过 MQTT 驱动 TC002 红绿灯。

当前显示素材为 52x16 PNG/GIF，小尺寸图片直接内嵌在 `blueprint.yaml` 里。灯体之外的背景像素为纯黑色，三盏灯的横向间距保持一致。

为了减少手机拍摄 LED 点阵时的暗部频闪，未亮起的灯体不会保留暗红、暗黄、暗绿像素，而是完全纯黑；只有当前亮起的灯体会绘制。

支持的状态：

| 状态值 | 显示效果 |
|---|---|
| `off` | 全黑熄灭 |
| `idle` | 绿灯 |
| `working` | 绿、黄、红灯循环 |
| `attention` | 黄灯闪烁 |
| `blocked` | 红灯闪烁 |

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
ulanzi_1bf6/custom/vibe_signal
```

`[PREFIX]` 通常是 MQTT 前缀加设备 MAC 地址后四位。比如 MQTT 前缀为 `ulanzi`，设备 MAC 后四位为 `1bf6`，则前缀通常为 `ulanzi_1bf6`。

## 与 Claude Code / Codex 搭配使用

推荐链路：

```text
Claude Code / Codex hook
  -> 更新 Home Assistant 状态实体
  -> Blueprint 发布 MQTT
  -> TC002 显示红绿灯
```

第一次测试可以手动切换 `input_select` helper。确认 TC002 能正常显示后，再把 Claude Code、Codex 或其它编程助手的 hook 接进来。

建议事件映射：

| Code Agent 行为 | 建议状态值 | 灯效 |
|---|---|---|
| 没有任务运行 | `off` | 全黑熄灭 |
| 一轮任务正常结束 | `idle` | 绿灯提示 |
| 用户提交任务、Agent 正在运行、正在调用工具 | `attention` | 黄灯闪烁 |
| 权限确认、命令失败、测试失败、流程阻塞 | `blocked` | 红灯闪烁 |
| 手动测试动画 | `working` | 绿、黄、红循环 |

如果使用 Home Assistant REST API，可以让 hook 调用下面的命令更新 helper：

```bash
curl -X POST "http://<HA_HOST>:8123/api/services/input_select/select_option" \
  -H "Authorization: Bearer <HA_LONG_LIVED_ACCESS_TOKEN>" \
  -H "Content-Type: application/json" \
  -d '{"entity_id":"input_select.tc002_vibe_status","option":"attention"}'
```

把最后的 `option` 改成 `idle`、`blocked`、`off`，就可以驱动不同灯效。

Claude Code、Codex 或其它支持 hook / shell command 的工具，可以在对应生命周期里调用这个命令：

| Hook 场景 | 建议命令 |
|---|---|
| 用户提交任务 / 开始执行 | 设置为 `attention` |
| 工具调用前后 / 命令运行中 | 设置为 `attention` |
| 权限请求 / 失败 / 阻塞 | 设置为 `blocked` |
| 一轮任务结束 | 设置为 `idle`，几秒后可再设置为 `off` |

如果你不想让每个 hook 直接写复杂 `curl`，可以自己封装一个脚本，例如 `tc002-vibe-status.sh attention`，脚本内部再调用 Home Assistant API。这样 Claude Code、Codex、CI 都可以复用同一套状态出口。

更详细的接入思路见 `AGENT_HOOKS.md`。

## 配置项

| 配置项 | 说明 |
|---|---|
| 状态实体 | 用来驱动红绿灯的 Home Assistant 实体 |
| TC002 Custom App MQTT topic | 目标 topic，通常是 `[PREFIX]/custom/[APP_NAME]` |
| 显示时长 | 写入 TC002 Custom App payload 的 `duration` 值 |
| 保留 MQTT 消息 | 是否让 broker 保留最后一条消息，便于设备重连后恢复显示 |
| 状态值 | 每种灯效对应的状态文本 |

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

3. 确认 topic。比如 MQTT 前缀为 `ulanzi`，设备 MAC 后四位为 `1bf6`，Custom App 名为 `vibe_signal`，则 topic 为：

   ```text
   ulanzi_1bf6/custom/vibe_signal
   ```

4. 确认 TC002 当前正在显示对应的 Custom App，例如 `vibe_signal`。更新 Custom App 内容不一定会自动切换到该 App。

5. 在 Home Assistant 中依次切换状态实体为 `attention`、`blocked`、`idle`、`off`。

6. TC002 应分别显示黄灯、红灯、绿灯、熄灭。

底层 MQTT 链路可以先用 `draw` payload 快速验证：

```bash
mosquitto_pub -h <BROKER_HOST> -t ulanzi_1bf6/custom/vibe_signal -m '{"duration":3600,"text":[],"image":[],"draw":[{"df":[0,0,52,16,"#000000"]},{"dfc":[26,8,5,"#FFCB52"]}]}'
```

## 已知问题

- 当前版本把小图片直接内嵌在 `blueprint.yaml` 中，安装最简单，但后续修改图形时需要同步更新 base64。
- TC002 收到 Custom App 更新后，不一定会自动切换到该 App。测试时建议先在设备上手动切到目标 Custom App，或者发布到当前正在显示的 App 名。
- 用手机相机录制 TC002 时，视频里可能出现频闪或横向滚动暗纹。这通常是 LED 点阵扫描刷新 / PWM 调光与相机快门不同步导致的，不一定代表肉眼看到的画面也在闪。
- TC002 的 MQTT topic 规则可能随官方固件变化，topic 已做成可配置项。

## 许可证

GPL-3.0-or-later。
