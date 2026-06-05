# Hardware

## Parts

- Orange Pi 4 Pro
- ESP8266 / NodeMCU
- High-power 3-pin IR transmitter module: IN / 5V / GND
- 5V power supply
- Midea air conditioner

## Orange Pi

- OS: Orange Pi 1.0.6 Jammy
- Kernel: Linux 5.15.147-sun60iw2
- User: `orangepi`
- Project directory: `/home/orangepi/ac_remote_test/`
- Hotspot interface: `ap0`
- Hotspot SSID: `AC_Remote_AP`
- Hotspot IP: `192.168.10.1/24`

Keep the hotspot password only in local files such as `/etc/default/ac-remote-hotspot` or an untracked `config.json`.

## ESP8266 And IR Module Wiring

Use this wiring:

```text
ESP8266 D2 / GPIO4 -> IR module IN
ESP8266 5V         -> IR module 5V
ESP8266 GND        -> IR module GND
```

If the IR module uses an external 5V power supply:

```text
External 5V  -> IR module 5V
External GND -> IR module GND
ESP8266 GND  -> External GND
ESP8266 D2   -> IR module IN
```

External power GND, ESP8266 GND, and IR module GND must be connected together.

## IR Parameters

- ESP8266 pin: NodeMCU D2 = GPIO4
- Carrier: 38kHz
- Protocol: COOLIX
- Firmware: `esp8266_ir_gateway_v2.ino`
- IR library class: `IRCoolixAC`

The firmware no longer uses raw timing arrays or `ac_raw_data.h` for normal A/C control.
