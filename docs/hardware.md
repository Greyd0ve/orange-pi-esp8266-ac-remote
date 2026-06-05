# Hardware

## 硬件

- Orange Pi 4 Pro
- ESP8266 / NodeMCU
- 三针大功率红外发射模块：IN / 5V / GND
- 5V 电源
- 美的空调

## Orange Pi

- 系统：Orange Pi 1.0.6 Jammy
- 内核：Linux 5.15.147-sun60iw2
- 用户名：`orangepi`
- 项目目录：`/home/orangepi/ac_remote_test/`
- 热点接口：`ap0`
- 热点名称：`AC_Remote_AP`
- 热点 IP：`192.168.10.1/24`

热点密码请只写在本地 `/etc/default/ac-remote-hotspot` 或未提交的 `config.json` 中。

## ESP8266 与红外模块接线

必须按下面方式连接：

```text
ESP8266 D2 / GPIO4 -> 红外模块 IN
ESP8266 5V         -> 红外模块 5V
ESP8266 GND        -> 红外模块 GND
```

如果使用外部 5V 电源：

```text
外部电源 5V  -> 红外模块 5V
外部电源 GND -> 红外模块 GND
ESP8266 GND  -> 外部电源 GND
ESP8266 D2   -> 红外模块 IN
```

外部电源 GND、ESP8266 GND、红外模块 GND 必须共地。

## 红外参数

- ESP8266 引脚：NodeMCU D2 = GPIO4
- 红外载波频率：38kHz
- 默认重复次数：`SEND_REPEAT_COUNT = 1`
- 固件主文件：`esp8266_ir_gateway_v2.ino`
- Raw 数据头文件：`ac_raw_data.h`

当前固件保留这些补偿参数：

```cpp
const uint16_t MARK_SHORT_COMP_US = 200;
const uint16_t SPACE_SHORT_COMP_US = 210;
const uint16_t SPACE_LONG_COMP_US = 200;
const uint16_t HEADER_SPACE_COMP_US = 200;
const uint16_t FRAME_GAP_SPACE_COMP_US = 100;
```

