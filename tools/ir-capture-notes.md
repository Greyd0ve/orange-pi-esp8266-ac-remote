# IR Capture Notes

用这个文件记录每次采集和验证 raw 数据的结果。

## 环境

- 遥控器型号：
- 空调型号：
- 接收模块：
- 采集工具：
- 采样频率：
- 采集日期：

## Raw 命令清单

| 命令 | 遥控器状态 | 数组名 | 是否验证通过 | 备注 |
| --- | --- | --- | --- | --- |
| 开机 | 电源开 | `raw_power_on` | 否 |  |
| 关机 | 电源关 | `raw_power_off` | 否 |  |
| 制冷 23℃ | cool 23 | `raw_cool_23` | 否 |  |
| 制冷 24℃ | cool 24 | `raw_cool_24` | 否 |  |
| 制冷 25℃ | cool 25 | `raw_cool_25` | 否 |  |
| 制冷 26℃ | cool 26 | `raw_cool_26` | 否 |  |
| 低风 | fan low | `raw_fan_low` | 否 |  |
| 中风 | fan mid | `raw_fan_mid` | 否 |  |
| 高风 | fan high | `raw_fan_high` | 否 |  |

## 对比项

记录原遥控器和 ESP8266 发射的解调输出：

- Header mark：
- Header space：
- Bit mark：
- 0 space：
- 1 space：
- Frame gap：
- 帧长度：
- 是否有重复帧：

## 调参记录

| 参数 | 原值 | 新值 | 结果 |
| --- | --- | --- | --- |
| `MARK_SHORT_COMP_US` | 200 |  |  |
| `SPACE_SHORT_COMP_US` | 210 |  |  |
| `SPACE_LONG_COMP_US` | 200 |  |  |
| `HEADER_SPACE_COMP_US` | 200 |  |  |
| `FRAME_GAP_SPACE_COMP_US` | 100 |  |  |
| `SEND_REPEAT_COUNT` | 1 |  |  |

