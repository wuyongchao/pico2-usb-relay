# Pico 2 USB Relay 调试记录

日期：2026-10-25

## 1. 目标

使用 Raspberry Pi Pico 2（RP2350）和本地 Pico SDK 实现一个 USB HID
继电器，使原有的 Windows `usbrelay.exe` 在不修改程序及其内嵌
`usb_relay_device.dll` 的情况下完成设备枚举、开关控制和状态回读。

最终 GPIO 规划：

| 项目 | 配置 |
|---|---|
| 继电器通道 | 1 |
| 控制 GPIO | GPIO6 |
| 逻辑 ON | GPIO6 输出低 |
| 逻辑 OFF | GPIO6 输出高 |
| 启动状态 | OFF（GPIO6 高） |

## 2. 初始环境

已确认本地环境：

- Pico SDK 2.3.0
- ARM GNU Toolchain 13.2.1
- CMake 3.28.3
- `picotool`
- 板卡配置 `PICO_BOARD=pico2`
- RP2350 ARM Secure 目标

工作区最初为空，不是 Git 仓库。

## 3. 方案选择

比较了 DCTTech 和 LCUS 两类协议。

选择 DCTTech 的原因：

- 支持继电器 ID、状态回读和通道数量语义。
- 原 Windows 工具识别的 Product String 为 `USBRelay1`。
- 可通过 Feature Report 同时实现状态查询和控制。

LCUS 路径的主要问题：

- 原工具对 LCUS 支持有限。
- 状态和通道数能力不完整。
- 为兼容当前 Windows 工具仍需额外的产品名称和私有约定。

## 4. USB 枚举调试过程

### 4.1 第一版端点方向错误

首版枚举日志显示：

```text
bEndpointAddress : 0x01 (Direction=OUT)
```

目标继电器的 HID 数据端点应为：

```text
bEndpointAddress : 0x81 (Direction=IN)
```

修正为 `0x81 IN`。

### 4.2 EP0=8 导致 Device Descriptor 读取失败

为贴近原设备曾把 TinyUSB EP0 包长改为 8，Windows 随后报告：

```text
Connection Status : Device failed enumeration
Device Descriptor : VID/PID = 0000/0000
Problem Code      : 43
```

现象是只读到全零的设备描述符后半部分。RP2350 的 TinyUSB 路径最终使用
EP0=64 恢复稳定枚举。

结论：RP2350 全速设备应保持 TinyUSB 已验证的 EP0=64。原继电器是低速
设备并使用 EP0=8，但这不是 RP2350 的可行复刻参数。

### 4.3 自定义 HID 类驱动导致设备启动失败

尝试用应用层 HID 类驱动覆盖内置 HID 驱动，Windows 一度出现：

```text
Problem Code : 10 (CM_PROB_FAILED_START)
Descriptor 1 : ERROR_GEN_FAILURE
```

自定义驱动还曾导致 `usbrelay.exe` 可以列出设备，但打开设备和查询状态时
出现空句柄，进而触发 .NET `AccessViolationException`。

最终撤销自定义 HID 类驱动，使用 TinyUSB 内置 HID 驱动。描述符和协议
通过应用回调完成，设备恢复稳定启动。

## 5. Windows 原工具分析

目标程序（路径已脱敏）：

```text
<usbrelay-v1.0.3>\usbrelay.exe
SHA-256: fec04a099b7966b88cfb7615aa943afa18cf26ac7dbb4e5a0bce62753d6c96ac
```

该程序不是 Linux 开源版 `usbrelay`。它是 .NET 前端，内嵌
`usb_relay_device.dll`，使用厂商 DLL 枚举 HID 设备。

它不使用以下 Linux 版参数：

```text
PICO1_1=1
PICO1_1=0
```

实际参数为：

```text
usbrelay.exe -list
usbrelay.exe -status
usbrelay.exe -serial PICO1 -on 1
usbrelay.exe -serial PICO1 -off 1
```

静态分析确认 DLL 会：

1. 枚举 HID 接口。
2. 校验 VID/PID `16C0:05DF`。
3. 读取并匹配 Product String `USBRelay1`。
4. 通过 HID Feature Report 查询 ID 和通道状态。
5. 通过 Feature Report 发送 `FF/FD/FC/FE` 控制命令。

## 6. 最终可用 USB 配置

最终固件采用 TinyUSB 标准 HID 路径。Windows 已实测成功，因此不再要求
物理层字段与原低速继电器逐项相同。

| 字段 | 最终值 |
|---|---|
| VID/PID | `16C0:05DF` |
| Manufacturer | `www.dcttech.com` |
| Product | `USBRelay1` |
| USB 速度 | Full-Speed |
| EP0 | 64 字节 |
| HID 版本 | 1.11 |
| 数据端点 | `0x81` Interrupt IN |
| 数据端点包长 | 16 字节 |
| 轮询间隔 | 20 ms |
| Report Descriptor | 22 字节 |
| Feature 数据 | 8 字节 |

原参考设备的低速、EP0=8、HID 1.01、8 字节数据端点没有在 RP2350 上强行
复刻；这些参数曾导致枚举或打开失败。

## 7. 最终实机验证

Windows 原工具实测输出：

```text
PS> .\usbrelay.exe -list
Serial    Type
------    ----
PICO1     OneChannel

PS> .\usbrelay.exe -status
Serial   C1   C2   C3   C4   C5   C6   C7   C8
------  ---- ---- ---- ---- ---- ---- ---- ----
PICO1   OFF

PS> .\usbrelay.exe -serial PICO1 -on 1
Open channel 1 on PICO1: success

PICO1   ON

PS> .\usbrelay.exe -serial PICO1 -off 1
Close channel 1 on PICO1: success

PICO1   OFF
```

由此确认：

- 设备的 VID/PID 和 Product String 可被原工具识别。
- `PICO1` 可作为继电器 ID 使用。
- `-on 1` 和 `-off 1` 可完成 Feature Report 控制。
- 状态回读可从 `OFF` 切换到 `ON`，再切回 `OFF`。
- 未修改 `usbrelay.exe` 或 `usb_relay_device.dll`。

## 8. 尚需硬件确认

已完成的是 USB 枚举、原工具兼容和协议状态回读。尚未在本次记录中确认
GPIO6 实际电平及外部继电器负载动作。

下一步应断开负载，用万用表或 LED 检查：

```text
逻辑 OFF -> GPIO6 高
逻辑 ON  -> GPIO6 低
```

若外部驱动板为高有效，应使用：

```sh
-DUSBRELAY_ACTIVE_LOW=OFF
```

重新编译固件，不改 Windows 工具或 DLL。

Pico GPIO 不能直接驱动继电器线圈。应使用三极管、MOSFET 或光耦隔离驱动，
并配置续流二极管。涉及市电时必须使用具备足够爬电距离和隔离等级的硬件。

## 9. 构建与测试

```sh
cmake -S . -B build -DPICO_BOARD=pico2 -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

主机协议测试：

```sh
cmake -S tests -B build-host -DCMAKE_BUILD_TYPE=Release
cmake --build build-host --parallel
ctest --test-dir build-host --output-on-failure
```

构建产物由本地构建或 GitHub Release 生成，不提交到源码仓库。`picotool`
验证信息示例：

```text
target chip : RP2350
image type  : ARM Secure
pico_board  : pico2
sdk version : 2.3.0
build type  : Release
```

## 10. 经验教训

- 先确认 Windows 工具是哪个实现，再按其实际协议调试。
- `success` 可能只代表 USB 控制请求被接受，必须继续检查状态回读。
- 出现空句柄和 `AccessViolationException` 时，应先恢复可稳定打开的
  固件基线，再缩小修改范围。
- 物理速度、EP0 包长可以与原设备不同；主机兼容的关键是设备安装、
  Product String 和协议行为。
- 不使用应用层 HID 类驱动覆盖已验证的 TinyUSB 内置实现。
