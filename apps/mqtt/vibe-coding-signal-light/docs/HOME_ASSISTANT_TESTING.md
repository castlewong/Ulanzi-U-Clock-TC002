# Home Assistant 与 TC002 MQTT 真机测试指南

本文说明如何在真实 TC002 设备上测试 `vibe-coding-signal-light`。

作者：王行知

最短链路如下：

```text
Home Assistant -> MQTT broker -> TC002
```

MQTT broker 可以理解为消息中转站。Home Assistant 把消息发给 broker，TC002 也连接到同一个 broker，并从对应 topic 收到显示内容。

## 1. 基本概念

### MQTT broker

MQTT broker 是一个轻量消息服务器。

在这个应用里，它接收 Home Assistant 发布的 TC002 Custom App payload，再转发给 TC002。

常见选择：

- Home Assistant Add-on 里的 Mosquitto broker
- Mac、NAS 或服务器上运行的 Mosquitto
- 家里已经在用的其他 MQTT broker

### Topic

topic 是 MQTT 消息地址。

TC002 Custom App MQTT topic 通常是：

```text
[PREFIX]/custom/[APP_NAME]
```

示例：

```text
ulanzi_1bf6/custom/vibe_signal
```

`[PREFIX]` 通常由 MQTT 前缀和设备 MAC 后四位组成：

```text
mqtt_prefix + "_" + MAC 后四位
```

如果 MQTT 前缀是 `ulanzi`，设备 MAC 后四位是 `1bf6`，则前缀通常是：

```text
ulanzi_1bf6
```

### Payload

payload 是 TC002 实际显示的 JSON 内容。

这个 Blueprint 发布的是 Custom App payload，里面包含一个小尺寸 PNG/GIF 图片。图片以 base64 data URL 形式直接放在 MQTT 消息里，所以 Home Assistant 只需要发布一条消息。

## 2. 推荐测试顺序

不要一开始就直接测完整 Home Assistant 自动化。推荐分层测试：

1. 启动一个 MQTT broker。
2. 确认本机可以向 broker 发布和订阅消息。
3. 把 TC002 配到同一个 broker。
4. 先手动发布一个黄灯 payload。
5. 再导入 Home Assistant Blueprint。
6. 最后用 Home Assistant 状态实体切换灯效。

这样可以把 broker、topic、TC002 配置、Home Assistant 自动化几个问题拆开排查。

## 3. Mac 上用 Mosquitto 搭建 broker

安装 Mosquitto：

```bash
brew install mosquitto
```

启动后台服务：

```bash
brew services start mosquitto
```

确认命令行工具可用：

```bash
mosquitto -h
mosquitto_pub --help
mosquitto_sub --help
```

查看 Mac 的局域网 IP：

```bash
ipconfig getifaddr en0
```

示例：

```text
10.19.1.58
```

TC002 需要能访问这个 IP 的 `1883` 端口。

## 4. Broker 本地自测

开一个终端订阅消息：

```bash
mosquitto_sub -h 127.0.0.1 -t tc002/test
```

另开一个终端发布消息：

```bash
mosquitto_pub -h 127.0.0.1 -t tc002/test -m "hello tc002"
```

如果订阅窗口打印出 `hello tc002`，说明 broker 本地收发正常。

## 5. 配置 TC002 MQTT

在 TC002 的 MQTT 设置里填写：

```text
Broker host: <Mac 局域网 IP>
Broker port: 1883
Username: 本地测试可留空，除非 broker 要求认证
Password: 本地测试可留空，除非 broker 要求认证
MQTT prefix: ulanzi
```

也可以用 HTTP 接口查看 TC002 当前网络和 MQTT 配置：

```bash
curl http://<TC002_IP>/getBase
curl http://<TC002_IP>/getMqttConfig
```

如果 TC002 和 broker 不在同一个 Wi-Fi 或局域网里，设备收不到消息。

## 6. 手动发布测试

先算出 topic。

示例：

- MQTT prefix: `ulanzi`
- TC002 MAC 后四位: `1bf6`
- Custom App 名: `vibe_signal`

topic 为：

```text
ulanzi_1bf6/custom/vibe_signal
```

先发布一个简单黄灯 payload，用来快速验证链路：

```bash
mosquitto_pub -h 127.0.0.1 -t ulanzi_1bf6/custom/vibe_signal -m '{"duration":3600,"text":[],"image":[],"draw":[{"df":[0,0,52,16,"#000000"]},{"dfc":[26,8,5,"#FFCB52"]}]}'
```

预期结果：

- TC002 收到 Custom App 更新。
- 如果设备当前正在显示 `vibe_signal`，屏幕会变成黄灯。

### 已验证结果

以下环境已在 2026-06-23 验证通过：

```text
TC002 IP: 10.19.1.128
TC002 MAC: ccc4b2441bf6
MQTT broker: 10.19.1.58:1883
MQTT prefix: ulanzi
Topic: ulanzi_1bf6/custom/vibe_signal
```

通过 Mosquitto 向 topic 发布黄灯 Custom App payload 后，真实 TC002 屏幕成功变为黄灯。

基础链路确认后，再用 Blueprint 里的图片 payload 或 Home Assistant 自动化测试最终效果。图片版本使用纯黑背景的红绿灯素材，观感会比简单 `draw` 探针更好。

注意：

TC002 更新 Custom App 内容后，不一定自动切换到对应 App。如果屏幕没有变化，请先在设备上手动切到目标 Custom App，或者把消息发布到当前正在显示的 App 名。

## 7. Home Assistant 设置

### 方案 A：使用已有 Home Assistant

如果 Home Assistant 已经配置 MQTT 集成，直接使用现有 MQTT 集成即可。

如果还没有 MQTT：

1. 如果你的 Home Assistant 安装方式支持 Add-on，可安装 Mosquitto broker Add-on。
2. 创建 MQTT 用户名和密码。
3. 添加 MQTT 集成。
4. 把 TC002 配到同一个 broker。

### 方案 B：Mac broker + 现有 Home Assistant

也可以把 Mac 作为 broker，让 Home Assistant 和 TC002 都连接 Mac 的局域网 IP。

Home Assistant MQTT broker 设置：

```text
Broker: <Mac 局域网 IP>
Port: 1883
Username/password: 本地测试可留空，除非 broker 要求认证
```

TC002 MQTT broker 设置必须使用同一个 host 和 port。

## 8. 创建 Home Assistant 状态实体

第一次测试推荐创建一个 `input_select` helper，选项如下：

```text
off
idle
working
attention
blocked
```

这个 helper 就是 Blueprint 的“状态实体”。

## 9. 导入 Blueprint

导入：

```text
apps/mqtt/vibe-coding-signal-light/blueprint.yaml
```

然后用它创建自动化。

推荐配置：

```text
状态实体: 上一步创建的 input_select helper
TC002 Custom App MQTT topic: ulanzi_1bf6/custom/vibe_signal
显示时长: 3600
保留 MQTT 消息: false
```

之后切换 helper 状态：

```text
attention -> 黄灯
blocked -> 红灯
idle -> 绿灯
working -> 绿、黄、红循环
off -> 全黑熄灭
```

## 10. 接入 Claude Code / Codex

确认 Home Assistant helper 能驱动 TC002 后，就可以把编程工具接进来。

基本思路：

```text
Claude Code / Codex hook -> 更新 input_select -> Blueprint 发布 MQTT -> TC002 显示
```

推荐先用 Home Assistant API 手动测试：

```bash
curl -X POST "http://<HA_HOST>:8123/api/services/input_select/select_option" \
  -H "Authorization: Bearer <HA_LONG_LIVED_ACCESS_TOKEN>" \
  -H "Content-Type: application/json" \
  -d '{"entity_id":"input_select.tc002_vibe_status","option":"attention"}'
```

如果 TC002 变成黄灯，说明 Code Agent hook 只需要在对应时机调用这类命令即可。

建议映射：

| 工具事件 | 状态 |
|---|---|
| 用户提交任务、开始执行、工具调用中 | `attention` |
| 权限请求、失败、阻塞 | `blocked` |
| 任务正常结束 | `idle`，几秒后 `off` |

更完整的说明见 `AGENT_HOOKS.md`。

### 已验证的 webhook 链路

本地测试中已验证下面这条链路：

```text
Claude Code hook -> Home Assistant webhook -> mqtt.publish -> Mosquitto -> TC002
```

关键点：

- Home Assistant 需要显式启用 `webhook:`。
- webhook 自动化使用 `local_only: true`，只接受本机请求，适合本机 Claude Code / Codex 调试。
- Claude Code hook 命令建议使用 `>/dev/null 2>&1 || true`，避免红绿灯脚本异常影响 Claude Code 正常启动或运行。
- 不建议把 `SessionStart` 作为红绿灯 hook。启动阶段 hook 一旦异常，可能导致 Claude Code 无法启动。

## 11. 排查问题

### MQTT 发布成功，但 TC002 不变化

检查：

- TC002 和 broker 是否在同一个 Wi-Fi 或局域网。
- TC002 MQTT broker host 是否正确。
- topic 前缀是否匹配设备 MAC 后四位。
- App 名是否匹配 TC002 当前显示的 Custom App。
- broker 的 `1883` 端口是否可访问。

### Home Assistant 自动化执行了，但 broker 没收到消息

订阅目标 topic：

```bash
mosquitto_sub -h <BROKER_HOST> -t 'ulanzi_1bf6/custom/vibe_signal' -v
```

然后切换 Home Assistant helper 状态。

如果订阅窗口没有消息，问题通常在 Home Assistant 自动化或 MQTT 集成。

### Broker 本地正常，但 TC002 收不到

检查 macOS 防火墙、路由器 AP 隔离、访客网络隔离等。部分 Wi-Fi 会阻止局域网设备互相访问。

### 显示颜色不对

确认状态实体的状态文本精确匹配以下值之一：

```text
off
idle
working
attention
blocked
```

如果你改了 Blueprint 里的状态值配置，请确认实体输出也同步修改。

### 手机拍视频时出现频闪

TC002 是 LED 点阵屏，通常会通过行列扫描和 PWM 调光显示画面。iPhone 拍视频时，传感器滚动快门、视频帧率、曝光时间可能和 LED 刷新节奏不同步，于是视频里会出现频闪、亮度跳动或横向暗纹。

这类现象常见于拍摄 LED 屏、车灯、显示屏、舞台灯，不一定代表肉眼看到的画面也在频闪。可以尝试：

- 在 iPhone 相机里切换 30fps / 60fps。
- 关闭自动曝光后手动拉低曝光。
- 改变拍摄距离和角度。
- 提高环境光，减少相机自动拉高快门或 ISO。
- 如果固件或设备设置支持，尝试调整屏幕亮度。

## 12. 贡献前检查

- `blueprint.yaml` 可以被 Home Assistant 导入。
- `docs/README.md` 已说明依赖、安装、配置、topic。
- `preview/demo.gif` 能展示实际灯效。
- 至少完成一次真机或 broker 级别测试，并记录环境。

## 13. 三种 Code Agent 状态模拟

完成 Home Assistant webhook 或 helper 接入后，建议至少模拟三种真实使用状态：

| 模拟场景 | 状态值 | 预期显示 |
|---|---|---|
| Code Agent 正在运行 / 调用工具 | `attention` | 黄灯闪烁 |
| 需要用户输入 / 权限确认 / 报错 | `blocked` | 红灯闪烁 |
| 一轮任务运行完成 | `idle`，随后 `off` | 绿灯短暂亮起，然后全黑 |

如果你使用 webhook 链路，可以订阅 MQTT topic 观察消息：

```bash
mosquitto_sub -h <BROKER_HOST> -t 'ulanzi_1bf6/custom/vibe_signal' -v
```

然后让 hook 依次发送：

```text
PreToolUse 或 UserPromptSubmit -> attention
PermissionRequest 或 Error     -> blocked
Stop 或 Done                   -> idle，几秒后 off
```

本项目实测时，`Claude Code hook -> Home Assistant webhook -> mqtt.publish -> Mosquitto -> TC002` 能按顺序触发：

```text
黄灯闪烁 -> 红灯闪烁 -> 绿灯 -> 全黑熄灭
```
