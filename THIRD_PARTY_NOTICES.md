# 第三方组件声明 (Third-Party Notices)

本仓库主体代码采用 [GPL-3.0-or-later](LICENSE) 许可证发布。
仓库内还包含若干第三方组件，分别遵循其原始许可证。本文件按照原作者要求，对这些组件做出声明。

如有遗漏或表述不准确之处，欢迎提交 Issue 或 PR。

---

## 1. BlueZ — Linux 蓝牙协议栈

**位置：**

- 二进制工具：`Z21_TC002_Demo/src/dependencies/bin/`
  - `hciattach`、`hciattach_bk`、`hciattach_bk2`、`hciconfig`、`hcitool`、`gattserverbin`
- 头文件：`Z21_TC002_Demo/src/dependencies/include/ble/`
- 静态库：`Z21_TC002_Demo/src/dependencies/lib/libgatt-server.a`

**许可证：**

- 二进制工具源自 BlueZ 的命令行工具部分，遵循 **GNU General Public License v2.0 or later (GPL-2.0-or-later)**
- 头文件与 `libgatt-server.a` 源自 BlueZ 库部分，遵循 **GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)**

**版权：** Copyright © BlueZ project authors

**源码地址：** <https://git.kernel.org/pub/scm/bluetooth/bluez.git>

**许可证全文：**

- GPL-2.0：<https://www.gnu.org/licenses/old-licenses/gpl-2.0.txt>
- LGPL-2.1：<https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt>

> 根据 GPL/LGPL 要求，凡获得本仓库副本者，均有权按照对应许可证条款获取上述组件的完整源代码。
> 本仓库主体许可证 GPL-3.0 与上述组件兼容。

---

## 2. Adafruit GFX 字体 — TomThumb

**位置：** `Z21_TC002_Demo/src/Fonts/TomThumb.h`

**许可证：** BSD-style（保留 Adafruit 与原作者版权声明）

**版权：**

- Original 3x5 font: Copyright © Brian J. Swetland
- 8-bit conversion: Copyright © Robey Pointer
- Adafruit GFX adaptation: Copyright © Adafruit Industries

**源码地址：** <https://github.com/adafruit/Adafruit-GFX-Library>

---

## 3. FlyThings SDK 及其依赖包

**说明：** 本仓库 **未直接分发** FlyThings SDK 及其依赖包，仅在 `Z21_TC002_Demo/Manifest.xml` 中通过包名 + 版本号引用。
编译时由 FlyThings IDE 自动从其依赖包仓库 <https://package.flythings.cn/> 拉取。

**涉及包：** `easyui`、`log`、`zkhardware`、`zknet`、`base-utility`、`transfer-protocols`、`audio-utility`、`ffmpeg`、`z`、`base-json`

**许可证：** 各包许可证以 FlyThings 官方说明为准。其中 `ffmpeg` 通常为 **LGPL-2.1+** 或 **GPL-2.0+**（取决于编译选项），其余 FlyThings 自有包请参考 <https://www.flythings.cn>。

---

## 4. 测试资源

**位置：**

- `Z21_TC002_Demo/resources/1KHZ.MP3` — 1KHz 测试正弦波音频，用于 `AudioTestPage` 演示
- `IDE使用说明/resources/ADBDriverInstaller.zip` — 第三方 ADB 驱动安装器
- `IDE使用说明/resources/z11sproject-configuration.zip` — IDE 工程配置示例

**说明：** 这些资源仅用于教学与演示，版权归原作者所有。如有侵权请联系仓库维护者删除。

---

## 5. 文档与截图

**位置：** `IDE使用说明/resources/` 下的所有 `*.png`、`*.jpg`、`*.gif`

**说明：** 截图涉及 FlyThings IDE 与第三方软件的界面，版权归各软件原作者所有，仅用于配套技术文档说明用途。

---

## 致谢

感谢 BlueZ、Adafruit、FlyThings、AWTRIX 等开源社区与厂商的工作，使本项目成为可能。
