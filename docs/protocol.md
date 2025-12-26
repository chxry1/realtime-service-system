# Packet Protocol v1

## Header (8 bytes)

| Field  | Size | Description    |
| ------ | ---- | -------------- |
| length | 4B   | 전체 패킷 길이 |
| type   | 2B   | 패킷 타입      |
| flags  | 2B   | 제어 비트      |

## Body

| Type   | Meaning |
| ------ | ------- |
| 0x0001 | PING    |
| 0x0002 | LOGIN   |
| 0x0003 | CHAT    |
