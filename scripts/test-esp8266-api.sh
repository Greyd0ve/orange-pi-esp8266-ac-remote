#!/usr/bin/env bash
set -euo pipefail

ESP8266_BASE_URL="${1:-${ESP8266_BASE_URL:-http://192.168.10.63}}"
ESP8266_BASE_URL="${ESP8266_BASE_URL%/}"

echo "Testing ESP8266 at $ESP8266_BASE_URL"

echo
echo "GET /ping"
curl -fsS "$ESP8266_BASE_URL/ping"

echo
echo
echo "GET /api/state"
curl -fsS "$ESP8266_BASE_URL/api/state"

echo
echo
echo "GET /api/send?cmd=cool_26"
curl -fsS "$ESP8266_BASE_URL/api/send?cmd=cool_26"

echo

