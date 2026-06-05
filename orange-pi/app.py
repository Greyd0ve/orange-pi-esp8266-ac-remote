"""Orange Pi air-conditioner control service."""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any

from flask import Flask, jsonify, request
from requests import RequestException

from ac_config import get_config
from ir_test import Esp8266IRClient


CONFIG = get_config()
AC_CONFIG = CONFIG["air_conditioner"]
ORANGE_PI_CONFIG = CONFIG["orange_pi"]

app = Flask(__name__)
ir_client = Esp8266IRClient(CONFIG)


def _state_file() -> Path:
    configured = Path(str(ORANGE_PI_CONFIG.get("state_file", "ac_state.json")))
    if configured.parent.exists():
        return configured
    return Path(__file__).with_name(configured.name)


def _default_state() -> dict[str, Any]:
    return {
        "power": AC_CONFIG.get("default_power", "off"),
        "mode": AC_CONFIG.get("default_mode", "cool"),
        "temp": int(AC_CONFIG.get("default_temp", 26)),
        "fan": AC_CONFIG.get("default_fan", "mid"),
        "last_command": None,
    }


def read_state() -> dict[str, Any]:
    state_path = _state_file()
    if not state_path.exists():
        return _default_state()
    try:
        with state_path.open("r", encoding="utf-8") as fp:
            state = json.load(fp)
    except (json.JSONDecodeError, OSError):
        return _default_state()

    merged = _default_state()
    merged.update(state)
    return merged


def write_state(state: dict[str, Any]) -> None:
    state_path = _state_file()
    state_path.parent.mkdir(parents=True, exist_ok=True)
    with state_path.open("w", encoding="utf-8") as fp:
        json.dump(state, fp, ensure_ascii=False, indent=2)


def _json_body() -> dict[str, Any]:
    if not request.is_json:
        return {}
    body = request.get_json(silent=True)
    return body if isinstance(body, dict) else {}


def _param(name: str) -> str | None:
    body = _json_body()
    value = request.args.get(name, body.get(name))
    if value is None:
        return None
    return str(value).strip()


def _validate_temp(temp: int) -> int:
    min_temp = int(AC_CONFIG.get("min_temp", 17))
    max_temp = int(AC_CONFIG.get("max_temp", 30))
    if temp < min_temp or temp > max_temp:
        raise ValueError(f"temperature must be between {min_temp} and {max_temp}")
    return temp


def _state_patch_for_command(command: str) -> dict[str, Any]:
    if command == "power_on":
        return {"power": "on"}
    if command == "power_off":
        return {"power": "off"}

    cool_match = re.fullmatch(r"cool_(\d{2})", command)
    if cool_match:
        return {"power": "on", "mode": "cool", "temp": int(cool_match.group(1))}

    fan_match = re.fullmatch(r"fan_(low|mid|high)", command)
    if fan_match:
        return {"power": "on", "mode": "fan", "fan": fan_match.group(1)}

    return {}


def _error_response(message: str, status: int = 400):
    return jsonify({"ok": False, "error": message}), status


@app.get("/")
def index():
    return jsonify(
        {
            "ok": True,
            "service": "orange-pi-ac-remote",
            "endpoints": ["/health", "/api/state", "/api/send", "/api/ac"],
        }
    )


@app.get("/health")
def health():
    result: dict[str, Any] = {"ok": True, "service": "ac-remote"}
    try:
        result["esp8266"] = ir_client.ping()
    except RequestException as exc:
        result["ok"] = False
        result["esp8266_error"] = str(exc)
    return jsonify(result), 200 if result["ok"] else 503


@app.route("/api/state", methods=["GET"])
def api_state():
    payload: dict[str, Any] = {"ok": True, "local_state": read_state()}
    try:
        payload["esp8266_state"] = ir_client.state()
    except RequestException as exc:
        payload["esp8266_error"] = str(exc)
    return jsonify(payload)


@app.route("/api/send", methods=["GET", "POST"])
def api_send():
    command = _param("cmd")
    if not command:
        return _error_response("missing cmd")

    try:
        result = ir_client.send_command(command)
    except RequestException as exc:
        return _error_response(f"ESP8266 request failed: {exc}", 502)

    state = read_state()
    state.update(_state_patch_for_command(command))
    state["last_command"] = command
    write_state(state)

    return jsonify({"ok": True, "command": command, "state": state, "esp8266": result})


@app.route("/api/ac", methods=["GET", "POST"])
def api_ac():
    power = _param("power")
    mode = _param("mode")
    fan = _param("fan")
    temp_value = _param("temp")

    temp: int | None = None
    if temp_value is not None and temp_value != "":
        try:
            temp = _validate_temp(int(temp_value))
        except ValueError as exc:
            return _error_response(str(exc))

    try:
        result = ir_client.set_ac(power=power, mode=mode, temp=temp, fan=fan)
    except RequestException as exc:
        return _error_response(f"ESP8266 request failed: {exc}", 502)

    state = read_state()
    if power:
        state["power"] = power
    if mode:
        state["mode"] = mode
    if temp is not None:
        state["temp"] = temp
    if fan:
        state["fan"] = fan
    state["last_command"] = "api_ac"
    write_state(state)

    return jsonify({"ok": True, "state": state, "esp8266": result})


if __name__ == "__main__":
    app.run(
        host=str(ORANGE_PI_CONFIG.get("app_host", "0.0.0.0")),
        port=int(ORANGE_PI_CONFIG.get("app_port", 8081)),
    )

