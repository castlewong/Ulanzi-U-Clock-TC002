# TC002 Claude Bot — Home Assistant 与 MQTT 真机测试指南

本文说明如何在真实 TC002 设备上测试 `claude-bot`。

## 1. 基本概念

```text
Home Assistant -> MQTT broker -> TC002
```

## 2. MQTT broker

推荐使用 Home Assistant Add-on 里的 Mosquitto broker。

Mac 本地测试：

```bash
brew install mosquitto
brew services start mosquitto
```

## 3. 配置 TC002 MQTT

在 TC002 的 MQTT 设置里填写：

```text
Broker host: <Mac 局域网 IP>
Broker port: 1883
MQTT prefix: ulanzi
```

查看 TC002 当前配置：

```bash
curl http://<TC002_IP>/getMqttConfig
```

## 4. Custom App topic

格式：

```text
[PREFIX]/custom/[APP_NAME]
```

示例：

```text
ulanzi_1bf6/custom/claude_bot
```

## 5. 手动测试 MQTT

```bash
# 测试 loop 动画
mosquitto_pub -h 127.0.0.1 -t ulanzi_1bf6/custom/claude_bot -m '{"duration":3600,"text":[],"image":[{"data":"data:image/gif;base64,<BASE64>","position":[0,0]}],"draw":[]}'

# 测试 usage 画面
mosquitto_pub -h 127.0.0.1 -t ulanzi_1bf6/custom/claude_bot -m '{"duration":3600,"text":[],"image":[{"data":"data:image/gif;base64,<BASE64>","position":[0,0]}],"draw":[]}'
```

## 6. 导入 Blueprint

1. 在 Home Assistant 中导入 `blueprint.yaml`
2. 创建 `input_select` helper，选项：`loop`、`usage`
3. 用 Blueprint 创建自动化
4. 设置 `TC002 Custom App MQTT topic` 为你的设备 topic

## 7. 状态映射

| 状态值 | 显示效果 |
|---|---|
| `loop` | Claude Bot 循环动画 |
| `usage` | Claude Bot 用量条（静态） |

## 8. 排障

### MQTT 发布成功但 TC002 不变化

- 确认 TC002 和 broker 在同一局域网
- 确认 topic 前缀匹配设备 MAC 后四位
- 手动在 TC002 上切换到目标 Custom App

### broker 本地正常但 TC002 收不到

- 检查 macOS 防火墙
- 检查路由器 AP 隔离
