#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="${1:-/home/orangepi/ac_remote_test}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "Project source: $REPO_DIR"
echo "Target project directory: $PROJECT_DIR"

if [[ "$REPO_DIR" != "$PROJECT_DIR" ]]; then
  echo "Copy or clone this repository to $PROJECT_DIR before installing services." >&2
  echo "Example: git clone <REPO_URL> $PROJECT_DIR" >&2
  exit 1
fi

cd "$PROJECT_DIR"

python3 -m venv .venv
.venv/bin/python -m pip install --upgrade pip
.venv/bin/pip install -r orange-pi/requirements.txt

if [[ ! -f orange-pi/config.json ]]; then
  cp orange-pi/config.example.json orange-pi/config.json
  echo "Created orange-pi/config.json. Edit it before starting services."
fi

sudo install -m 0644 orange-pi/systemd/ac-remote.service /etc/systemd/system/ac-remote.service
sudo install -m 0644 orange-pi/systemd/qq-bot.service /etc/systemd/system/qq-bot.service
sudo install -m 0644 orange-pi/systemd/ac-hotspot.service /etc/systemd/system/ac-hotspot.service
sudo install -m 0755 scripts/start-ac-hotspot.sh /usr/local/bin/start-ac-hotspot.sh

if [[ ! -f /etc/default/ac-remote ]]; then
  sudo tee /etc/default/ac-remote >/dev/null <<'EOF'
AC_REMOTE_CONFIG=/home/orangepi/ac_remote_test/orange-pi/config.json
APP_HOST=0.0.0.0
APP_PORT=8081
QQ_BOT_HOST=0.0.0.0
QQ_BOT_PORT=8091
EOF
  echo "Created /etc/default/ac-remote."
fi

if [[ ! -f /etc/default/ac-remote-hotspot ]]; then
  sudo tee /etc/default/ac-remote-hotspot >/dev/null <<'EOF'
AP_IFACE=ap0
AP_PHY_IFACE=wlan0
AP_SSID=AC_Remote_AP
AP_PASSWORD=<HOTSPOT_PASSWORD>
AP_CIDR=192.168.10.1/24
EOF
  echo "Created /etc/default/ac-remote-hotspot. Replace AP_PASSWORD before enabling hotspot."
fi

sudo systemctl daemon-reload
echo "Enable services after editing config:"
echo "  sudo systemctl enable --now ac-hotspot.service"
echo "  sudo systemctl enable --now ac-remote.service"
echo "  sudo systemctl enable --now qq-bot.service"

