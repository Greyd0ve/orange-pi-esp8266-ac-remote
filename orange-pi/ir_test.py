"""ESP8266 infrared gateway client.

This file keeps the old "IR test" entry point name, but the execution layer now
talks to ESP8266 over HTTP instead of driving infrared hardware on Orange Pi.
"""

from __future__ import annotations

import argparse
from typing import Any

import requests

from ac_config import get_config


COMMAND_ALIASES = {
    "on": "power_on",
    "off": "power_off",
    "power_on": "power_on",
    "power_off": "power_off",
    "cool_23": "cool_23",
    "cool_24": "cool_24",
    "cool_25": "cool_25",
    "cool_26": "cool_26",
    "fan_low": "fan_low",
    "fan_mid": "fan_mid",
    "fan_high": "fan_high",
}


class Esp8266IRClient:
    def __init__(self, config: dict[str, Any] | None = None) -> None:
        self.config = config or get_config()
        esp_config = self.config["esp8266"]
        self.base_url = str(esp_config["base_url"]).rstrip("/")
        self.timeout = int(esp_config.get("timeout_seconds", 5))

    def _request(self, path: str, params: dict[str, Any] | None = None) -> dict[str, Any]:
        if not self.base_url:
            raise RuntimeError("ESP8266 base_url is empty. Please configure esp8266.base_url.")

        response = requests.get(
            f"{self.base_url}{path}",
            params=params or {},
            timeout=self.timeout,
        )
        response.raise_for_status()
        try:
            return response.json()
        except ValueError:
            return {"ok": True, "text": response.text}

    def ping(self) -> dict[str, Any]:
        return self._request("/ping")

    def state(self) -> dict[str, Any]:
        return self._request("/api/state")

    def send_command(self, command: str) -> dict[str, Any]:
        normalized = COMMAND_ALIASES.get(command, command)
        return self._request("/api/send", {"cmd": normalized})

    def set_ac(
        self,
        power: str | None = None,
        mode: str | None = None,
        temp: int | None = None,
        fan: str | None = None,
    ) -> dict[str, Any]:
        params: dict[str, Any] = {}
        if power:
            params["power"] = power
        if mode:
            params["mode"] = mode
        if temp is not None:
            params["temp"] = temp
        if fan:
            params["fan"] = fan
        return self._request("/api/ac", params)


def power_on() -> dict[str, Any]:
    return Esp8266IRClient().send_command("power_on")


def power_off() -> dict[str, Any]:
    return Esp8266IRClient().send_command("power_off")


def cool_26() -> dict[str, Any]:
    return Esp8266IRClient().send_command("cool_26")


def main() -> None:
    parser = argparse.ArgumentParser(description="Test ESP8266 IR gateway HTTP API.")
    subparsers = parser.add_subparsers(dest="command", required=True)

    subparsers.add_parser("ping")
    subparsers.add_parser("state")

    send_parser = subparsers.add_parser("send")
    send_parser.add_argument("cmd", help="Command name, for example power_on or cool_26")

    ac_parser = subparsers.add_parser("ac")
    ac_parser.add_argument("--power", choices=["on", "off"])
    ac_parser.add_argument("--mode", choices=["cool", "heat", "fan", "dry", "auto"])
    ac_parser.add_argument("--temp", type=int)
    ac_parser.add_argument("--fan", choices=["low", "mid", "high"])

    args = parser.parse_args()
    client = Esp8266IRClient()

    if args.command == "ping":
        result = client.ping()
    elif args.command == "state":
        result = client.state()
    elif args.command == "send":
        result = client.send_command(args.cmd)
    elif args.command == "ac":
        result = client.set_ac(args.power, args.mode, args.temp, args.fan)
    else:  # pragma: no cover
        raise ValueError(args.command)

    print(result)


if __name__ == "__main__":
    main()

