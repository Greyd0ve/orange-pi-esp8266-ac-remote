#!/usr/bin/env bash
set -euo pipefail

AP_IFACE="${AP_IFACE:-ap0}"
AP_PHY_IFACE="${AP_PHY_IFACE:-wlan0}"
AP_SSID="${AP_SSID:-AC_Remote_AP}"
AP_PASSWORD="${AP_PASSWORD:-}"
AP_CIDR="${AP_CIDR:-192.168.10.1/24}"
CON_NAME="${CON_NAME:-ac-remote-hotspot}"

if [[ -z "$AP_PASSWORD" || "$AP_PASSWORD" == \<* ]]; then
  echo "AP_PASSWORD is not configured. Set it in /etc/default/ac-remote-hotspot." >&2
  exit 1
fi

if ! command -v nmcli >/dev/null 2>&1; then
  echo "nmcli is required. Install or enable NetworkManager first." >&2
  exit 1
fi

if ! ip link show "$AP_IFACE" >/dev/null 2>&1; then
  if command -v iw >/dev/null 2>&1 && ip link show "$AP_PHY_IFACE" >/dev/null 2>&1; then
    iw dev "$AP_PHY_IFACE" interface add "$AP_IFACE" type __ap || true
  fi
fi

nmcli device set "$AP_IFACE" managed yes || true

if nmcli -t -f NAME connection show | grep -Fxq "$CON_NAME"; then
  nmcli connection modify "$CON_NAME" \
    connection.interface-name "$AP_IFACE" \
    802-11-wireless.ssid "$AP_SSID" \
    wifi-sec.psk "$AP_PASSWORD" \
    ipv4.addresses "$AP_CIDR"
else
  nmcli connection add type wifi ifname "$AP_IFACE" con-name "$CON_NAME" ssid "$AP_SSID"
  nmcli connection modify "$CON_NAME" \
    802-11-wireless.mode ap \
    802-11-wireless.band bg \
    ipv4.method shared \
    ipv4.addresses "$AP_CIDR" \
    ipv6.method ignore \
    wifi-sec.key-mgmt wpa-psk \
    wifi-sec.psk "$AP_PASSWORD"
fi

nmcli connection up "$CON_NAME"
nmcli -f GENERAL.DEVICE,GENERAL.STATE,IP4.ADDRESS device show "$AP_IFACE" || true

