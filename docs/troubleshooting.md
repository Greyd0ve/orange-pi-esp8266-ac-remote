# Troubleshooting

## ESP8266 无法连接热点

检查顺序：

1. ESP8266 只支持 2.4GHz Wi-Fi，确认 Orange Pi 热点是 2.4GHz。
2. 确认 `WIFI_SSID` 和 `WIFI_PASSWORD` 与 `/etc/default/ac-remote-hotspot` 一致。
3. 查看热点服务日志：

```bash
journalctl -u ac-hotspot.service -f
nmcli connection show
nmcli device status
ip addr show ap0
```

4. 打开 Arduino 串口监视器，波特率 `115200`，看是否卡在 `Connecting to Wi-Fi`。
5. 如果 `ap0` 不存在，确认物理无线接口名是不是 `wlan0`，必要时修改 `AP_PHY_IFACE`。

## Orange Pi 无法 curl ESP8266

检查：

```bash
ip addr show ap0
ping <ESP8266_IP>
curl -v http://<ESP8266_IP>/ping
```

常见原因：

- ESP8266 实际 IP 和 `config.json` 中的 `esp8266.base_url` 不一致。
- ESP8266 没有成功启动 HTTP server。
- Orange Pi 热点没有给 ESP8266 分配到 `192.168.10.x` 地址。
- ESP8266 连接到了别的 Wi-Fi。

建议从 ESP8266 串口监视器读取真实 IP。

## 红外模块手机能看到闪光但空调不响

手机摄像头看到闪光只能证明红外 LED 有发光，不代表协议正确。

重点检查：

- `ac_raw_data.h` 是否是真实美的遥控器采集数据。
- 红外载波是否为 38kHz。
- 红外模块是否对准空调接收窗，距离是否太远。
- 供电电流是否不足，大功率红外模块建议使用稳定 5V。
- 使用外部 5V 时是否共地。
- raw 数据是否被重复补偿，导致时序偏长。

## 51 单片机能收到但空调不响应

很多 51 红外接收方案使用解调接收头，输出的是去掉 38kHz 载波后的数字波形。空调接收端需要的是带 38kHz 载波调制的红外光。

可能原因：

- 51 能看到波形，但 ESP8266 发射的载波频率或占空比不对。
- raw 时序和原遥控器差异太大。
- 采集数据来自解调模块，mark / space 需要重新校准。
- 空调协议可能是完整状态帧，不是简单按键码。

建议用逻辑分析仪同时对比原遥控器和 ESP8266 发射的解调输出。

## 红外时序偏短或偏长

如果空调不响应，但接收端能看到完整帧，可以调这些参数：

```cpp
MARK_SHORT_COMP_US
SPACE_SHORT_COMP_US
SPACE_LONG_COMP_US
HEADER_SPACE_COMP_US
FRAME_GAP_SPACE_COMP_US
```

建议：

- 每次只改一个方向，小步调整。
- 先把补偿全部设为 `0`，确认 raw 数据本身是否可靠。
- 对比原遥控器和 ESP8266 的 header mark、header space、bit mark、0/1 space、frame gap。
- 不要只凭手机摄像头判断时序。

## Arduino 找不到 IRremoteESP8266.h

解决方法：

1. Arduino IDE 打开库管理器。
2. 搜索 `IRremoteESP8266`。
3. 安装版本 `2.9.0`。
4. 关闭并重新打开 Arduino IDE。

如果仍然找不到，检查库是否在：

```text
Documents/Arduino/libraries/IRremoteESP8266/
```

不要把库文件夹放进本项目的 `esp8266_ir_gateway_v2` 工程目录。

## Arduino 库目录和工程目录混乱

正确结构：

```text
Arduino/
├── libraries/
│   └── IRremoteESP8266/
└── esp8266_ir_gateway_v2/
    ├── esp8266_ir_gateway_v2.ino
    └── ac_raw_data.h
```

本仓库中的结构：

```text
esp8266/
└── esp8266_ir_gateway_v2/
    ├── esp8266_ir_gateway_v2.ino
    └── ac_raw_data.h
```

打开 Arduino 工程时，选择 `esp8266_ir_gateway_v2.ino`。如果 Arduino IDE 提示移动文件，确认移动后的目录仍叫 `esp8266_ir_gateway_v2`。

## SEND_REPEAT_COUNT 取 1 或 2 的区别

`SEND_REPEAT_COUNT = 1`：

- 更接近原始单次按键。
- 对电源这类可能带切换语义的命令更安全。
- 推荐作为初始值。

`SEND_REPEAT_COUNT = 2`：

- 可能提升远距离或弱供电时的成功率。
- 如果命令是完整状态帧，通常问题不大。
- 如果某个命令被空调理解为切换动作，重复两次可能产生意外结果。

建议先用 `1` 调通，确认每个 raw 命令都是完整状态帧后，再尝试 `2`。

