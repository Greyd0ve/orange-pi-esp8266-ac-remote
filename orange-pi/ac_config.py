"""Configuration helpers for the Orange Pi services."""

from __future__ import annotations

import json
import os
from copy import deepcopy
from pathlib import Path
from typing import Any

try:
    from dotenv import load_dotenv
except ImportError:  # pragma: no cover - python-dotenv is optional at import time.
    load_dotenv = None


BASE_DIR = Path(__file__).resolve().parent

DEFAULT_CONFIG: dict[str, Any] = {
    "orange_pi": {
        "project_dir": str(BASE_DIR.parent),
        "state_file": str(BASE_DIR / "ac_state.json"),
        "app_host": "0.0.0.0",
        "app_port": 8081,
        "qq_bot_host": "0.0.0.0",
        "qq_bot_port": 8091,
        "app_base_url": "http://127.0.0.1:8081",
    },
    "napcat": {
        "base_url": "",
        "access_token": "",
        "post_secret": "",
        "self_id": "",
    },
    "esp8266": {
        "base_url": "http://192.168.10.63",
        "timeout_seconds": 5,
        "default_command": "cool_26",
    },
    "access_control": {
        "allow_all_private": False,
        "allow_all_groups": False,
        "private_user_whitelist": [],
        "group_whitelist": [],
    },
    "air_conditioner": {
        "default_power": "off",
        "default_mode": "cool",
        "default_temp": 26,
        "default_fan": "mid",
        "min_temp": 17,
        "max_temp": 30,
    },
    "electricity": {
        "enabled": False,
        "api_url": "",
        "account": "",
        "password": "",
    },
}


def is_placeholder(value: Any) -> bool:
    if not isinstance(value, str):
        return False
    text = value.strip()
    return text.startswith("<") and text.endswith(">")


def _deep_merge(base: dict[str, Any], override: dict[str, Any]) -> dict[str, Any]:
    for key, value in override.items():
        if isinstance(value, dict) and isinstance(base.get(key), dict):
            _deep_merge(base[key], value)
        else:
            base[key] = value
    return base


def _env_int(name: str, fallback: int) -> int:
    raw = os.getenv(name)
    if raw is None or raw == "":
        return fallback
    return int(raw)


def _env_bool(name: str, fallback: bool) -> bool:
    raw = os.getenv(name)
    if raw is None or raw == "":
        return fallback
    return raw.lower() in {"1", "true", "yes", "on"}


def _env_list(name: str, fallback: list[str]) -> list[str]:
    raw = os.getenv(name)
    if raw is None or raw.strip() == "":
        return fallback
    return [item.strip() for item in raw.split(",") if item.strip()]


def _apply_env_overrides(config: dict[str, Any]) -> None:
    orange_pi = config["orange_pi"]
    napcat = config["napcat"]
    esp8266 = config["esp8266"]
    access = config["access_control"]
    electricity = config["electricity"]

    orange_pi["app_host"] = os.getenv("APP_HOST", orange_pi["app_host"])
    orange_pi["app_port"] = _env_int("APP_PORT", int(orange_pi["app_port"]))
    orange_pi["qq_bot_host"] = os.getenv("QQ_BOT_HOST", orange_pi["qq_bot_host"])
    orange_pi["qq_bot_port"] = _env_int("QQ_BOT_PORT", int(orange_pi["qq_bot_port"]))
    orange_pi["app_base_url"] = os.getenv("APP_BASE_URL", orange_pi["app_base_url"])

    napcat["base_url"] = os.getenv("NAPCAT_BASE_URL", napcat["base_url"])
    napcat["access_token"] = os.getenv("NAPCAT_ACCESS_TOKEN", napcat["access_token"])
    napcat["post_secret"] = os.getenv("ONEBOT_POST_SECRET", napcat["post_secret"])
    napcat["self_id"] = os.getenv("BOT_QQ", napcat["self_id"])

    esp8266["base_url"] = os.getenv("ESP8266_BASE_URL", esp8266["base_url"])
    esp8266["timeout_seconds"] = _env_int(
        "ESP8266_TIMEOUT_SECONDS", int(esp8266["timeout_seconds"])
    )

    access["allow_all_private"] = _env_bool(
        "ALLOW_ALL_PRIVATE", bool(access["allow_all_private"])
    )
    access["allow_all_groups"] = _env_bool(
        "ALLOW_ALL_GROUPS", bool(access["allow_all_groups"])
    )
    access["private_user_whitelist"] = _env_list(
        "PRIVATE_USER_WHITELIST", access["private_user_whitelist"]
    )
    access["group_whitelist"] = _env_list("GROUP_WHITELIST", access["group_whitelist"])

    electricity["enabled"] = _env_bool("ELECTRICITY_ENABLED", bool(electricity["enabled"]))
    electricity["api_url"] = os.getenv("ELECTRICITY_API_URL", electricity["api_url"])
    electricity["account"] = os.getenv("CAMPUS_ACCOUNT", electricity["account"])
    electricity["password"] = os.getenv("CAMPUS_PASSWORD", electricity["password"])


def _candidate_config_paths(path: str | os.PathLike[str] | None) -> list[Path]:
    if path:
        return [Path(path)]

    env_path = os.getenv("AC_REMOTE_CONFIG")
    paths: list[Path] = []
    if env_path:
        paths.append(Path(env_path))
    paths.append(BASE_DIR / "config.json")
    paths.append(BASE_DIR / "config.example.json")
    return paths


def load_config(path: str | os.PathLike[str] | None = None) -> dict[str, Any]:
    if load_dotenv is not None:
        load_dotenv(BASE_DIR.parent / ".env")
        load_dotenv(BASE_DIR / ".env")

    config = deepcopy(DEFAULT_CONFIG)
    for candidate in _candidate_config_paths(path):
        if candidate.exists():
            with candidate.open("r", encoding="utf-8") as fp:
                _deep_merge(config, json.load(fp))
            break

    _apply_env_overrides(config)
    return config


_CONFIG_CACHE: dict[str, Any] | None = None


def get_config(reload: bool = False) -> dict[str, Any]:
    global _CONFIG_CACHE
    if reload or _CONFIG_CACHE is None:
        _CONFIG_CACHE = load_config()
    return _CONFIG_CACHE

