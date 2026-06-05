# ESP8266 Firmware

Firmware directory:

```text
esp8266/esp8266_ir_gateway_v2/
└── esp8266_ir_gateway_v2.ino
```

The Arduino project directory name must match the `.ino` filename.

## Development Environment

- Arduino IDE or PlatformIO
- ESP8266 Board Core 3.1.2
- IRremoteESP8266 2.9.0 or later
- Board: NodeMCU 1.0 or the matching ESP8266 board profile

## Protocol

The captured Midea remote frames match the `COOLIX` protocol:

- Carrier: 38kHz
- 24-bit effective data
- Each byte is followed by its inverse byte, for example `B2 4D 3F C0 D0 2F`
- Effective frame example: `0xB23FD0`

The firmware uses the stateful `IRCoolixAC` API:

```cpp
#include <ir_Coolix.h>

const uint16_t kIrLed = 4;  // NodeMCU D2 = GPIO4
IRCoolixAC ac(kIrLed);
```

It no longer includes `ac_raw_data.h`, no longer calls `sendRaw()`, and does not maintain `raw_cool_26[]` style timing arrays.

## Wi-Fi Configuration

Edit these values locally before flashing:

```cpp
const char* WIFI_SSID = "AC_Remote_AP";
const char* WIFI_PASSWORD = "<WIFI_PASSWORD>";
```

Do not commit a real hotspot password.

## IR Pin

```cpp
const uint16_t kIrLed = 4;
```

NodeMCU D2 is GPIO4.

## HTTP API

After uploading the firmware, read the ESP8266 IP from the serial monitor and test:

```bash
curl http://<ESP8266_IP>/ping
curl http://<ESP8266_IP>/api/state
curl "http://<ESP8266_IP>/api/ac?power=on&mode=cool&temp=26&fan=high"
curl "http://<ESP8266_IP>/api/ac?power=off"
```

`/api/ac` parameters:

| Parameter | Values | Default |
| --- | --- | --- |
| `power` | `on`, `off` | unknown values become `on` |
| `mode` | `cool`, `heat`, `dry`, `fan`, `auto` | unknown values become `cool` |
| `temp` | `17` to `30` | missing values become `26`; out-of-range values are clamped |
| `fan` | `low`, `mid`, `high`, `auto` | unknown values become `high` |

Mode mapping:

```cpp
cool -> kCoolixCool
heat -> kCoolixHeat
dry  -> kCoolixDry
fan  -> kCoolixFan
auto -> kCoolixAuto
```

Fan mapping:

```cpp
low  -> kCoolixFanMin
mid  -> kCoolixFanMed
high -> kCoolixFanMax
auto -> kCoolixFanAuto
```

## Legacy Compatibility

The ESP8266 firmware still exposes `/api/send?cmd=...` so the existing Orange Pi Python services do not need to change immediately.

Supported legacy commands:

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

These commands are translated to `IRCoolixAC` state changes internally.
