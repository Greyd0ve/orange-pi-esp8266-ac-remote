# NapCatQQ / OneBot11

Windows 侧运行 NapCatQQ，负责接收 QQ 消息，并通过 OneBot11 HTTP POST 上报到 Orange Pi。

## 推荐流程

1. 在 Windows 上启动 NapCatQQ。
2. 启用 OneBot11 HTTP 上报。
3. 设置 HTTP POST 上报地址：

```text
http://<ORANGE_PI_IP>:8091/
```

4. 设置 Access Token，例如 `<NAPCAT_TOKEN>`。
5. 在 Orange Pi 的 `orange-pi/config.json` 中填写相同 Token。
6. 设置机器人 QQ 号到 `napcat.self_id`。

## Orange Pi 配置示例

```json
{
  "napcat": {
    "base_url": "http://<WINDOWS_HOST>:<NAPCAT_HTTP_PORT>",
    "access_token": "<NAPCAT_TOKEN>",
    "post_secret": "<ONEBOT_POST_SECRET>",
    "self_id": "<BOT_QQ>"
  }
}
```

不要提交真实 Token、真实 QQ 号。

## 消息处理规则

私聊：

- 默认只允许 `private_user_whitelist` 中的 QQ。
- 如果临时调试需要全部允许，可设置 `allow_all_private: true`，调试后建议关闭。

群聊：

- 默认只允许 `group_whitelist` 中的群。
- 群聊消息必须 @机器人。
- 如果没有正确配置 `napcat.self_id`，机器人可能无法识别自己是否被 @。

## 回复消息

`qq_bot.py` 会调用 NapCat HTTP API：

```text
POST /send_msg
```

如果 `napcat.base_url` 仍是占位符，服务只会在日志中打印本应回复的消息，不会真正发送 QQ 回复。

## 本地测试 OneBot 上报

可以用 curl 模拟私聊：

```bash
curl -X POST http://127.0.0.1:8091/ \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer <NAPCAT_TOKEN>" \
  -d '{
    "post_type": "message",
    "message_type": "private",
    "user_id": "<ADMIN_QQ>",
    "message": "空调状态"
  }'
```

请把示例里的占位符替换为本地真实值，但不要提交。

