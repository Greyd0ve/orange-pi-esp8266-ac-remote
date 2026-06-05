# orange-pi-esp8266-ac-remote

基于 Orange Pi 4 Pro、ESP8266 和红外发射模块的宿舍美的空调远程控制项目。

目标链路：

```text
QQ 消息 -> Windows NapCatQQ / OneBot11 -> Orange Pi 4 Pro -> ESP8266 -> 红外发射模块 -> 美的空调
```

## 目录结构

```text
orange-pi-esp8266-ac-remote/
├── README.md
├── LICENSE
├── .gitignore
├── .env.example
├── docs/
│   ├── architecture.md
│   ├── hardware.md
│   ├── orange-pi-setup.md
│   ├── esp8266-firmware.md
│   ├── napcat-onebot.md
│   └── troubleshooting.md
├── orange-pi/
│   ├── ac_config.py
│   ├── app.py
│   ├── qq_bot.py
│   ├── ir_test.py
│   ├── requirements.txt
│   ├── config.example.json
│   └── systemd/
│       ├── ac-remote.service
│       ├── qq-bot.service
│       └── ac-hotspot.service
├── esp8266/
│   └── esp8266_ir_gateway_v2/
│       └── esp8266_ir_gateway_v2.ino
├── scripts/
│   ├── start-ac-hotspot.sh
│   ├── install-orange-pi-services.sh
│   └── test-esp8266-api.sh
└── tools/
    └── ir-capture-notes.md
```

## 架构图

```mermaid
flowchart LR
    QQ["QQ 私聊 / 群聊 @机器人"] --> NapCat["Windows NapCatQQ<br/>OneBot11"]
    NapCat -- "HTTP POST :8091" --> QQBot["Orange Pi<br/>qq_bot.py"]
    QQBot -- "HTTP :8081" --> App["Orange Pi<br/>app.py"]
    App -- "HTTP GET" --> ESP["ESP8266<br/>IR Gateway"]
    ESP -- "38kHz COOLIX IR" --> IR["大功率红外发射模块"]
    IR --> AC["美的空调"]
```

## 硬件清单

- Orange Pi 4 Pro
- Orange Pi 1.0.6 Jammy，Linux 5.15.147-sun60iw2
- ESP8266 / NodeMCU
- 三针大功率红外发射模块：IN / 5V / GND
- 美的空调遥控器，用于确认 COOLIX 协议与按键行为
- 可选外部 5V 电源

接线：

```text
ESP8266 D2 / GPIO4 -> 红外模块 IN
5V                -> 红外模块 5V
GND               -> 红外模块 GND
```

如果红外模块使用外部 5V 电源，外部电源 GND、ESP8266 GND、红外模块 GND 必须共地。

## 软件环境

Orange Pi：

- Python 3
- Flask
- requests
- systemd
- NetworkManager / nmcli

ESP8266：

- Arduino IDE
- ESP8266 Board Core 3.1.2
- IRremoteESP8266 2.9.0
- 固件目录：`esp8266/esp8266_ir_gateway_v2/`

Windows：

- NapCatQQ
- OneBot11 HTTP 上报

## 快速开始

在 Orange Pi 上：

```bash
cd /home/orangepi
git clone <REPO_URL> ac_remote_test
cd /home/orangepi/ac_remote_test
python3 -m venv .venv
.venv/bin/pip install -r orange-pi/requirements.txt
cp orange-pi/config.example.json orange-pi/config.json
nano orange-pi/config.json
```

至少需要修改：

- `napcat.base_url`：Windows NapCat HTTP API 地址
- `napcat.access_token`：填本地真实 Token，不要提交
- `napcat.self_id`：机器人 QQ 号，不要提交
- `esp8266.base_url`：例如 `http://192.168.10.63`
- `access_control.private_user_whitelist` / `group_whitelist`

启动 Orange Pi 控制服务：

```bash
.venv/bin/python orange-pi/app.py
```

另开一个终端启动 QQ bot：

```bash
.venv/bin/python orange-pi/qq_bot.py
```

测试 ESP8266：

```bash
bash scripts/test-esp8266-api.sh http://192.168.10.63
```

## Orange Pi API

`app.py` 默认监听 `0.0.0.0:8081`。

```bash
curl http://127.0.0.1:8081/health
curl http://127.0.0.1:8081/api/state
curl "http://127.0.0.1:8081/api/send?cmd=power_on"
curl "http://127.0.0.1:8081/api/send?cmd=power_off"
curl "http://127.0.0.1:8081/api/send?cmd=cool_26"
curl "http://127.0.0.1:8081/api/ac?power=on&mode=cool&temp=26&fan=high"
```

## ESP8266 API

ESP8266 默认提供：

```text
GET /ping
GET /api/state
GET /api/ac?power=on&mode=cool&temp=26&fan=high
GET /api/ac?power=off
```

当前 ESP8266 固件使用 IRremoteESP8266 的 `IRCoolixAC` 状态接口发送 COOLIX 协议，不再维护 raw 数组。为兼容 Orange Pi 现有调用，仍保留旧的 `/api/send?cmd=...` 映射层：

- `power_on`
- `power_off`
- `cool_23`
- `cool_24`
- `cool_25`
- `cool_26`
- `fan_low`
- `fan_mid`
- `fan_high`

推荐新接口：

```bash
curl "http://<ESP8266_IP>/api/ac?power=on&mode=cool&temp=26&fan=high"
curl "http://<ESP8266_IP>/api/ac?power=off"
```

## QQ 命令

私聊机器人，或在群聊中 @机器人 后发送：

| 命令 | 行为 |
| --- | --- |
| 空调开 | 发送 `power_on` |
| 空调关 | 发送 `power_off` |
| 制冷 | 按默认温度进入制冷 |
| 制热 | 使用 COOLIX heat 模式 |
| 送风 | 发送 `fan_mid` |
| 升温 | 本地状态温度 +1 后调用 `/api/ac` |
| 降温 | 本地状态温度 -1 后调用 `/api/ac` |
| 设置温度26 | 设置制冷 26℃ |
| 空调状态 | 查询 Orange Pi 本地状态和 ESP8266 状态 |
| 健康检查 | 检查 Orange Pi 和 ESP8266 连通性 |
| 电费查询 | 保留入口，不提交校园网账号密码 |

## systemd 部署

在仓库已经位于 `/home/orangepi/ac_remote_test` 后：

```bash
cd /home/orangepi/ac_remote_test
bash scripts/install-orange-pi-services.sh
sudo nano /etc/default/ac-remote
sudo nano /etc/default/ac-remote-hotspot
sudo nano orange-pi/config.json
sudo systemctl enable --now ac-hotspot.service
sudo systemctl enable --now ac-remote.service
sudo systemctl enable --now qq-bot.service
```

查看日志：

```bash
journalctl -u ac-hotspot.service -f
journalctl -u ac-remote.service -f
journalctl -u qq-bot.service -f
```

## 调试方法

1. 先让 ESP8266 连上 Orange Pi 热点，在串口监视器确认 IP。
2. 在 Orange Pi 上执行 `curl http://<ESP8266_IP>/ping`。
3. 执行 `curl "http://<ESP8266_IP>/api/ac?power=on&mode=cool&temp=26&fan=high"`，确认红外模块有闪光。
4. 执行 `curl "http://127.0.0.1:8081/api/send?cmd=cool_26"`，确认 Orange Pi 控制服务可转发。
5. 在 NapCat 中确认 OneBot11 HTTP 上报地址是 `http://<ORANGE_PI_IP>:8091/`。
6. 最后用 QQ 私聊或群聊 @机器人 测试。

## 安全注意事项

- 不要提交真实 `config.json`、`.env`、`ac_state.json` 或日志。
- README 和示例配置中的 Token、QQ 号、校园网账号密码都必须保持占位符。
- 仓库只提交 `orange-pi/config.example.json`。
- 热点密码建议只写在 `/etc/default/ac-remote-hotspot` 或本地 `config.json`，不要写进公开仓库。
- 红外模块使用外部 5V 时必须共地，否则可能出现有闪光但发射不稳定。

更多细节见 `docs/` 目录。

