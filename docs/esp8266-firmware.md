# ESP8266 Firmware

固件目录：

```text
esp8266/esp8266_ir_gateway_v2/
├── esp8266_ir_gateway_v2.ino
└── ac_raw_data.h
```

Arduino 工程目录名必须和 `.ino` 主文件名一致。

## 开发环境

- Arduino IDE
- ESP8266 Board Core 3.1.2
- IRremoteESP8266 2.9.0
- 开发板选择：NodeMCU 1.0 或与你的 ESP8266 板子匹配的型号

## 安装库

Arduino IDE 中打开：

```text
工具 -> 管理库 -> 搜索 IRremoteESP8266 -> 安装 2.9.0
```

如果提示找不到 `IRremoteESP8266.h`，通常是库没有装到 Arduino 的 `libraries` 目录，见 `troubleshooting.md`。

## 配置 Wi-Fi

在 `esp8266_ir_gateway_v2.ino` 中修改：

```cpp
const char* WIFI_SSID = "AC_Remote_AP";
const char* WIFI_PASSWORD = "<HOTSPOT_PASSWORD>";
```

不要把真实热点密码提交到公开仓库。

## 配置红外引脚

```cpp
const uint16_t IR_LED_PIN = 4;
const uint16_t IR_CARRIER_KHZ = 38;
```

NodeMCU D2 对应 GPIO4。

## Raw 数据

`ac_raw_data.h` 必须包含这些数组：

```cpp
raw_power_on
raw_power_off
raw_cool_23
raw_cool_24
raw_cool_25
raw_cool_26
raw_fan_low
raw_fan_mid
raw_fan_high
```

仓库里的数组是占位数据，只保证代码结构和编译路径清楚。真正控制空调前，请替换为你采集到的美的空调 raw 数据。

## HTTP API

上传固件后，在串口监视器查看 ESP8266 IP，然后测试：

```bash
curl http://<ESP8266_IP>/ping
curl http://<ESP8266_IP>/api/state
curl "http://<ESP8266_IP>/api/send?cmd=cool_26"
curl "http://<ESP8266_IP>/api/ac?power=on&mode=cool&temp=26"
```

支持命令：

```text
power_on
power_off
cool_23
cool_24
cool_25
cool_26
fan_low
fan_mid
fan_high
```

## 时序补偿

固件中保留以下参数：

```cpp
const uint16_t MARK_SHORT_COMP_US = 200;
const uint16_t SPACE_SHORT_COMP_US = 210;
const uint16_t SPACE_LONG_COMP_US = 200;
const uint16_t HEADER_SPACE_COMP_US = 200;
const uint16_t FRAME_GAP_SPACE_COMP_US = 100;
```

如果空调不响应，但接收模块或手机摄像头能看到红外，重点检查 raw 数据、载波频率、发射角度、供电电流和这些补偿参数。

