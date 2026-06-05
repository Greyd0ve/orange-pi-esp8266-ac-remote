"""OneBot11 webhook service for QQ air-conditioner commands."""

from __future__ import annotations

import logging
import re
from typing import Any

import requests
from flask import Flask, jsonify, request
from requests import RequestException

from ac_config import get_config, is_placeholder


CONFIG = get_config()
ORANGE_PI_CONFIG = CONFIG["orange_pi"]
NAPCAT_CONFIG = CONFIG["napcat"]
ACCESS_CONFIG = CONFIG["access_control"]
AC_CONFIG = CONFIG["air_conditioner"]
ELECTRICITY_CONFIG = CONFIG["electricity"]

APP_BASE_URL = str(
    ORANGE_PI_CONFIG.get(
        "app_base_url", f"http://127.0.0.1:{ORANGE_PI_CONFIG.get('app_port', 8081)}"
    )
).rstrip("/")

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
logger = logging.getLogger("qq-bot")
app = Flask(__name__)


def _usable_secret(value: Any) -> bool:
    return isinstance(value, str) and bool(value.strip()) and not is_placeholder(value)


def _expected_token() -> str:
    token = str(NAPCAT_CONFIG.get("access_token", "")).strip()
    return token if _usable_secret(token) else ""


def _incoming_token() -> str:
    auth = request.headers.get("Authorization", "")
    if auth.lower().startswith("bearer "):
        return auth.split(" ", 1)[1].strip()
    return (
        request.headers.get("X-Access-Token")
        or request.headers.get("X-Self-Token")
        or request.args.get("access_token")
        or ""
    ).strip()


def _verify_request_token() -> bool:
    expected = _expected_token()
    if not expected:
        return True
    return _incoming_token() == expected


def _clean_allow_list(values: list[Any]) -> set[str]:
    return {str(value).strip() for value in values if str(value).strip() and not is_placeholder(value)}


def _is_allowed(event: dict[str, Any]) -> bool:
    message_type = str(event.get("message_type", ""))
    user_id = str(event.get("user_id", ""))
    group_id = str(event.get("group_id", ""))

    if message_type == "private":
        if ACCESS_CONFIG.get("allow_all_private", False):
            return True
        return user_id in _clean_allow_list(ACCESS_CONFIG.get("private_user_whitelist", []))

    if message_type == "group":
        if ACCESS_CONFIG.get("allow_all_groups", False):
            return True
        return group_id in _clean_allow_list(ACCESS_CONFIG.get("group_whitelist", []))

    return False


def _message_to_text(message: Any) -> str:
    if isinstance(message, str):
        return re.sub(r"\[CQ:[^\]]+\]", " ", message).strip()

    if isinstance(message, list):
        parts: list[str] = []
        for segment in message:
            if not isinstance(segment, dict):
                continue
            if segment.get("type") == "text":
                parts.append(str(segment.get("data", {}).get("text", "")))
        return " ".join(parts).strip()

    return ""


def _group_mentioned_bot(event: dict[str, Any]) -> bool:
    if event.get("message_type") != "group":
        return True

    self_id = str(NAPCAT_CONFIG.get("self_id", "")).strip()
    message = event.get("message")

    if isinstance(message, list):
        for segment in message:
            if not isinstance(segment, dict) or segment.get("type") != "at":
                continue
            qq = str(segment.get("data", {}).get("qq", "")).strip()
            if qq == "all" or (_usable_secret(self_id) and qq == self_id):
                return True
        return False

    if isinstance(message, str):
        if _usable_secret(self_id):
            return f"[CQ:at,qq={self_id}]" in message
        return "[CQ:at," in message

    return False


def _request_ac_service(path: str, params: dict[str, Any] | None = None) -> dict[str, Any]:
    response = requests.get(f"{APP_BASE_URL}{path}", params=params or {}, timeout=6)
    response.raise_for_status()
    return response.json()


def _status_text() -> str:
    data = _request_ac_service("/api/state")
    state = data.get("local_state", {})
    return (
        "空调状态："
        f"{state.get('power', 'unknown')}，"
        f"{state.get('mode', 'unknown')}，"
        f"{state.get('temp', 'unknown')}℃，"
        f"风速 {state.get('fan', 'unknown')}"
    )


def _control_send(command: str, label: str) -> str:
    _request_ac_service("/api/send", {"cmd": command})
    return f"已执行：{label}"


def _control_ac(label: str, **params: Any) -> str:
    _request_ac_service("/api/ac", params)
    return f"已执行：{label}"


def _adjust_temp(delta: int) -> str:
    data = _request_ac_service("/api/state")
    state = data.get("local_state", {})
    current = int(state.get("temp", AC_CONFIG.get("default_temp", 26)))
    min_temp = int(AC_CONFIG.get("min_temp", 17))
    max_temp = int(AC_CONFIG.get("max_temp", 30))
    target = max(min_temp, min(max_temp, current + delta))
    _request_ac_service(
        "/api/ac",
        {
            "power": "on",
            "mode": state.get("mode", AC_CONFIG.get("default_mode", "cool")),
            "temp": target,
        },
    )
    return f"已设置温度：{target}℃"


def _electricity_text() -> str:
    if not ELECTRICITY_CONFIG.get("enabled", False):
        return "电费查询接口尚未配置，已保留命令入口。"
    if is_placeholder(ELECTRICITY_CONFIG.get("api_url", "")):
        return "电费查询接口地址仍是占位符，请先在 config.json 中配置。"
    return "电费查询适配器尚未实现，请在 qq_bot.py 中接入校园网接口。"


def handle_command(text: str) -> str | None:
    normalized = re.sub(r"\s+", "", text)
    if not normalized:
        return None

    if normalized in {"空调开", "开空调", "打开空调"}:
        return _control_send("power_on", "空调开")
    if normalized in {"空调关", "关空调", "关闭空调"}:
        return _control_send("power_off", "空调关")
    if normalized == "制冷":
        temp = int(AC_CONFIG.get("default_temp", 26))
        return _control_ac(f"制冷 {temp}℃", power="on", mode="cool", temp=temp)
    if normalized == "制热":
        temp = int(AC_CONFIG.get("default_temp", 26))
        return _control_ac(f"制热 {temp}℃", power="on", mode="heat", temp=temp)
    if normalized == "送风":
        return _control_send("fan_mid", "送风")
    if normalized == "升温":
        return _adjust_temp(1)
    if normalized == "降温":
        return _adjust_temp(-1)
    if normalized == "空调状态":
        return _status_text()
    if normalized == "健康检查":
        health = _request_ac_service("/health")
        return "健康检查：" + ("正常" if health.get("ok") else "异常")
    if normalized == "电费查询":
        return _electricity_text()

    temp_match = re.fullmatch(r"(?:设置温度|温度)(\d{2})", normalized)
    if temp_match:
        temp = int(temp_match.group(1))
        return _control_ac(f"设置温度 {temp}℃", power="on", mode="cool", temp=temp)

    cool_match = re.fullmatch(r"制冷(\d{2})", normalized)
    if cool_match:
        temp = int(cool_match.group(1))
        return _control_ac(f"制冷 {temp}℃", power="on", mode="cool", temp=temp)

    return None


def _send_qq_reply(event: dict[str, Any], message: str) -> None:
    base_url = str(NAPCAT_CONFIG.get("base_url", "")).rstrip("/")
    if not base_url or is_placeholder(base_url):
        logger.info("NapCat base_url is not configured. Reply would be: %s", message)
        return

    payload: dict[str, Any] = {
        "message_type": event.get("message_type"),
        "message": message,
    }
    if event.get("message_type") == "private":
        payload["user_id"] = event.get("user_id")
    elif event.get("message_type") == "group":
        payload["group_id"] = event.get("group_id")
    else:
        return

    headers: dict[str, str] = {}
    token = _expected_token()
    if token:
        headers["Authorization"] = f"Bearer {token}"

    response = requests.post(f"{base_url}/send_msg", json=payload, headers=headers, timeout=6)
    response.raise_for_status()


@app.get("/health")
def health():
    return jsonify({"ok": True, "service": "qq-bot"})


@app.route("/", methods=["POST"])
@app.route("/onebot/v11", methods=["POST"])
def onebot_event():
    if not _verify_request_token():
        return jsonify({"ok": False, "error": "unauthorized"}), 401

    event = request.get_json(silent=True) or {}
    if event.get("post_type") != "message":
        return jsonify({"ok": True, "ignored": "not_message"})

    if not _is_allowed(event):
        logger.warning("Ignored unauthorized message: %s", event)
        return jsonify({"ok": True, "ignored": "not_allowed"})

    if not _group_mentioned_bot(event):
        return jsonify({"ok": True, "ignored": "not_mentioned"})

    text = _message_to_text(event.get("message"))
    try:
        reply = handle_command(text)
    except RequestException as exc:
        logger.exception("Control request failed")
        reply = f"执行失败：{exc}"
    except Exception as exc:  # Keep the bot alive on malformed commands.
        logger.exception("Command handling failed")
        reply = f"命令处理失败：{exc}"

    if reply:
        try:
            _send_qq_reply(event, reply)
        except RequestException:
            logger.exception("Failed to send QQ reply")
        return jsonify({"ok": True, "reply": reply})

    return jsonify({"ok": True, "ignored": "unknown_command"})


if __name__ == "__main__":
    app.run(
        host=str(ORANGE_PI_CONFIG.get("qq_bot_host", "0.0.0.0")),
        port=int(ORANGE_PI_CONFIG.get("qq_bot_port", 8091)),
    )

