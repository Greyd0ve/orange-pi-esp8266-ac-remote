# Orange Pi Setup

以下步骤默认项目放在：

```bash
/home/orangepi/ac_remote_test
```

## 1. 准备 Python 环境

```bash
cd /home/orangepi/ac_remote_test
python3 -m venv .venv
.venv/bin/python -m pip install --upgrade pip
.venv/bin/pip install -r orange-pi/requirements.txt
```

## 2. 配置项目

```bash
cp orange-pi/config.example.json orange-pi/config.json
nano orange-pi/config.json
```

需要填写：

- `napcat.base_url`
- `napcat.access_token`
- `napcat.self_id`
- `esp8266.base_url`
- `access_control.private_user_whitelist`
- `access_control.group_whitelist`

不要把真实 `config.json` 提交到 GitHub。

## 3. 手动运行服务

```bash
.venv/bin/python orange-pi/app.py
```

另开终端：

```bash
.venv/bin/python orange-pi/qq_bot.py
```

## 4. 安装 systemd 服务

```bash
cd /home/orangepi/ac_remote_test
bash scripts/install-orange-pi-services.sh
sudo nano /etc/default/ac-remote
sudo nano /etc/default/ac-remote-hotspot
sudo systemctl enable --now ac-hotspot.service
sudo systemctl enable --now ac-remote.service
sudo systemctl enable --now qq-bot.service
```

## 5. 常用命令

```bash
sudo systemctl status ac-hotspot.service
sudo systemctl status ac-remote.service
sudo systemctl status qq-bot.service

journalctl -u ac-hotspot.service -f
journalctl -u ac-remote.service -f
journalctl -u qq-bot.service -f

curl http://127.0.0.1:8081/health
curl http://127.0.0.1:8081/api/state
curl "http://127.0.0.1:8081/api/send?cmd=cool_26"
```

## 6. 热点

`scripts/start-ac-hotspot.sh` 使用 NetworkManager / `nmcli` 创建 AP 连接。

本地配置文件：

```bash
/etc/default/ac-remote-hotspot
```

示例：

```bash
AP_IFACE=ap0
AP_PHY_IFACE=wlan0
AP_SSID=AC_Remote_AP
AP_PASSWORD=<HOTSPOT_PASSWORD>
AP_CIDR=192.168.10.1/24
```

