# ESP32 스마트글래스 BLE 탐지기

주변의 BLE 광고 패킷을 스캔해서 **스마트글래스(Ray-Ban/Oakley Meta, Even Realities, Snap Spectacles 등)로 보이는 기기만** 골라 알려주는 ESP32 펌웨어입니다.

## 원리

스마트글래스는 휴대폰과 연결하려고 BLE 광고(advertising) 패킷을 주기적으로 내보냅니다. 이 펌웨어는 그 패킷을 두 가지 기준으로 판별합니다.

| 기준 | 설명 | 신뢰도 |
|---|---|---|
| **Company ID** (Manufacturer Data 앞 2바이트) | Bluetooth SIG가 제조사마다 부여한 고정 번호입니다. MAC 주소와 달리 무작위로 바뀌지 않습니다. | 높음 |
| **기기 이름** 키워드 | `Ray-Ban`, `Even G1` 같은 이름입니다. 주로 페어링 모드일 때만 보입니다. | 낮음~중간 |

등록된 Company ID (`src/signatures.h`):

- `0x0D53` Luxottica (Ray-Ban/Oakley Meta 프레임 제조사) → HIGH
- `0x01AB`, `0x058E` Meta → MEDIUM (Quest 헤드셋도 같은 ID를 써서 이름까지 일치해야 HIGH)
- `0x03C2` Snap Spectacles → HIGH

Even Realities, XREAL, Rokid 등은 현재 기기 이름으로만 탐지합니다. 이 ID들은 공개된 오픈소스 프로젝트(Nearby Glasses, NoPeek)에서 보고된 값이므로, 실제 기기로 확인해 보시기를 권장합니다.

## 준비물

- ESP32 보드 (ESP32 DevKit, S3, C3 모두 가능. ESP32-S2에는 블루투스가 없어서 안 됩니다)
- (선택) 액티브 부저, LED

## 빌드 및 업로드

```bash
pip install platformio
cd esp32-glasses-detector
pio run -e esp32dev -t upload    # S3 보드는 -e esp32s3, C3 보드는 -e esp32c3
pio device monitor
```

Arduino IDE를 쓰신다면 `NimBLE-Arduino` 2.x 라이브러리를 설치하고, `src/main.cpp`와 `src/signatures.h`를 스케치 폴더에 넣으면 됩니다.

## 출력 예시

```
========== 스마트글래스 감지 ==========
 제조사/모델 : Luxottica (Ray-Ban/Oakley Meta)
 신뢰도      : HIGH
 신호 세기   : -63 dBm (가까움(~3m))
 BLE 주소    : 5a:1c:..:..
 Company ID  : 0x0D53
========================================
```

HIGH는 LED/부저가 3번, MEDIUM은 1번 울립니다. 5초마다 주변 감지 목록도 출력합니다.

## 설정 (`src/main.cpp` 상단)

- `RSSI_THRESHOLD`: 이보다 약한 신호는 무시합니다. 범위를 좁히려면 `-70` 정도로 올리세요.
- `MIN_ALERT_CONFIDENCE`: `CONF_LOW`로 바꾸면 이름만 일치하는 기기도 알립니다.
- `BUZZER_PIN`, `LED_PIN`: 연결한 핀 번호입니다.
- `RAW_DUMP = true`: 모든 광고 패킷을 출력합니다. 내 안경을 옆에 두고 켜서 Company ID와 이름을 직접 수집할 때 씁니다.

## 한계

- **이미 폰에 연결된 안경**은 광고를 드물게 보내거나 아예 보내지 않을 수 있어서 놓칠 수 있습니다. 전원을 켤 때, 케이스에서 꺼낼 때, 재연결할 때 잘 잡힙니다.
- **MAC 주소가 무작위로 바뀌어서** 같은 안경이 시간이 지나면 다른 기기로 보일 수 있습니다.
- **오탐**: Meta ID는 Quest 헤드셋 등과 공유합니다.
- **RSSI 거리는 대략값**입니다. 사람 몸, 벽, 주머니 때문에 크게 달라집니다.
- **카메라 촬영 여부는 알 수 없습니다.** 근처에 스마트글래스가 있다는 것만 알 수 있습니다.
- 수동 수신만 하고 상대 기기에 접속하거나 간섭하지 않습니다.
