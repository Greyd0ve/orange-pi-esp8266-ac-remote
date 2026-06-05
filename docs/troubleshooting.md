# Troubleshooting

## ESP8266 Cannot Connect To The Hotspot

Check:

```bash
journalctl -u ac-hotspot.service -f
nmcli connection show
nmcli device status
ip addr show ap0
```

Common causes:

- ESP8266 only supports 2.4GHz Wi-Fi.
- `WIFI_SSID` or `WIFI_PASSWORD` in the firmware does not match the Orange Pi hotspot settings.
- `ap0` was not created. Check whether the physical wireless interface is really `wlan0`.
- The hotspot password is still a placeholder.

Use the Arduino serial monitor at `115200` baud to see whether the board is stuck at `Connecting to Wi-Fi`.

## Orange Pi Cannot curl ESP8266

Check:

```bash
ip addr show ap0
ping <ESP8266_IP>
curl -v http://<ESP8266_IP>/ping
```

Common causes:

- The ESP8266 IP differs from `esp8266.base_url` in `orange-pi/config.json`.
- ESP8266 did not start the HTTP server.
- ESP8266 connected to another Wi-Fi network.
- Orange Pi hotspot did not assign a `192.168.10.x` address.

Read the real ESP8266 IP from the serial monitor.

## IR LED Flashes But The Air Conditioner Does Not Respond

Phone cameras can show that the IR LED emits light, but they cannot prove the protocol is correct.

Check:

- The firmware includes `ir_Coolix.h` and uses `IRCoolixAC`.
- The control URL is `/api/ac?power=on&mode=cool&temp=26&fan=high`.
- The IR pin is still GPIO4 / NodeMCU D2.
- The IR module points at the A/C receiver window.
- The module has enough 5V current.
- External 5V power, ESP8266 GND, and IR module GND are connected together.
- Your A/C model actually accepts COOLIX-compatible frames.

## 51 MCU Can Receive Frames But The A/C Does Not Respond

Many 51 MCU capture setups use a demodulating IR receiver. That output removes the 38kHz carrier and shows only mark/space timing.

For this project, the important conclusion from the capture is the 24-bit COOLIX payload pattern:

```text
B2 4D 3F C0 D0 2F
```

Each byte is followed by its inverse byte, so the effective 24-bit value is:

```text
0xB23FD0
```

The ESP8266 firmware now lets IRremoteESP8266 generate COOLIX timing and the 38kHz carrier instead of replaying captured raw arrays.

## Timing Looks Short Or Long

The current firmware does not use manual raw timing compensation. If timing looks wrong:

- Confirm the installed IRremoteESP8266 version supports `IRCoolixAC`.
- Confirm the board profile is ESP8266 / NodeMCU and the CPU clock is normal.
- Compare emitted frames with a demodulating receiver if needed.
- Avoid reintroducing `sendRaw()` compensation unless COOLIX support is proven incompatible with your exact A/C.

## Arduino Cannot Find IRremoteESP8266.h Or ir_Coolix.h

Install the library:

1. Open Arduino IDE Library Manager.
2. Search `IRremoteESP8266`.
3. Install version `2.9.0` or later.
4. Restart Arduino IDE.

The library should live under:

```text
Documents/Arduino/libraries/IRremoteESP8266/
```

Do not copy the library source into this repository.

## Arduino Library Directory And Project Directory Are Mixed Up

Correct structure:

```text
Arduino/
├── libraries/
│   └── IRremoteESP8266/
└── esp8266_ir_gateway_v2/
    └── esp8266_ir_gateway_v2.ino
```

Repository structure:

```text
esp8266/
└── esp8266_ir_gateway_v2/
    └── esp8266_ir_gateway_v2.ino
```

Open `esp8266_ir_gateway_v2.ino` directly. If Arduino IDE asks to move it, make sure the final folder name remains `esp8266_ir_gateway_v2`.

## Repeats

`IRCoolixAC::send()` uses the protocol's default repeat behavior from IRremoteESP8266. The firmware no longer exposes `SEND_REPEAT_COUNT`.

If a command is unreliable, first check power, aiming, and protocol compatibility before changing library repeat behavior.
