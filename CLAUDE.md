# OUI Spy Unified Blue – Multi-Mode BLE/WiFi Surveillance Detection Firmware

Six-mode unified firmware with boot selector menu. Detects surveillance hardware, drones, and BLE tracking devices. This fork targets the **Elecrow ThinkNode M5** (ESP32-S3R8).

## Hardware

- **Board**: Elecrow ThinkNode M5 — ESP32-S3R8 (8 MB octal PSRAM), 4 MB flash
- **Buzzer**: GPIO 9 (active, PWM-capable)
- **Boot Button**: GPIO 21 (BUTTON1, hold 1.5–2s to enter selector)
- **Status LED**: blue "notification" LED on PCA9557 I2C expander (0x18) — `Wire1` SDA=48, SCL=47
- **Serial console**: UART0 (GPIO 43/44) via CH340 USB-UART bridge — *not* native USB-CDC
- **GPS** (Flock-You mode): onboard L76K (power switch on GPIO 10)
- **E-ink**: 1.54" 200×200, driven by this firmware

## Modes

| Mode | File | Purpose |
|------|------|---------|
| Boot Selector | `src/main.cpp` | Mode selection menu via serial/web |
| Detector | `src/raw/detector.cpp` | OUI-based WiFi surveillance detection |
| Foxhunter | `src/raw/foxhunter.cpp` | RSSI proximity tracker for specific BLE targets |
| Flock-You | `src/raw/flockyou_promiscious.cpp` | Flock Cam detection with GPS logging |
| Sky Spy | `src/raw/skyspy.cpp` | Drone Remote ID detection (BLE + WiFi) |
| BLE Sniff | `src/raw/blesniff.cpp` | Passive BLE advertising capture (Wireshark-ready) |

## Build & Run

```bash
pio run -e thinknode_m5
pio run -e thinknode_m5 -t upload
pio device monitor -b 115200
```

## PlatformIO Config

- **Board**: `esp32-s3-devkitc-1` base; flash 4 MB; **8 MB octal PSRAM** (`qio_opi`)
- **Flash mode**: `dio` — **QIO boot-loops on this board** (octal PSRAM). Do not change back to QIO.
- **Partition**: `partitions_4mb.csv` — ~1.875 MB app + ~2 MB spiffs (fits 4 MB flash)
- **BLE**: NimBLE-Arduino 1.4.0+
- **Web**: AsyncWebServer 3.0.6+
- **Filesystem**: LittleFS (web assets, config)

## Build Flags

```
-DCORE_DEBUG_LEVEL=0
-DCONFIG_BT_NIMBLE_ENABLED=1
-DBOARD_THINKNODE_M5
-DBOARD_HAS_PSRAM
-Isrc/raw
```

No `-DARDUINO_USB_CDC_ON_BOOT` — the M5's native-USB pins (GPIO 19/20) are wired to the GPS
module; the serial console is UART0. The M5 **does** have PSRAM (8 MB octal), so
`-DBOARD_HAS_PSRAM` is set. `-mfix-esp32-psram-cache-issue` is *not* used.

## NVS Namespaces

| Namespace | Purpose |
|-----------|---------|
| `unified-mode` | Selected boot mode |
| `ouispy-ap` | AP credentials |
| `ouispy-bz` | Buzzer toggle |
| `pcap-mode` | PCAP mode config |
| `blesniff` | BLE Sniff mode config |

Don't reuse these in mode code.

## Web Interface

- **AP IP**: 192.168.4.1 (all modes)
- Served from LittleFS partition
- Mode-specific endpoints for config and data export

## Architecture

- Anonymous namespaces for symbol isolation between modes
- Each mode is a self-contained `.cpp` file in `src/raw/`
- Boot selector in `main.cpp` routes to selected mode
- ~9700 lines total across 4 firmware implementations

## Gotchas

1. **Status LED is on the PCA9557 expander**, not a plain GPIO — use `M5Board::led()` from `board_m5.h` (which reconstructs the full output register every write; don't cache state in a static).
2. **Flash mode is DIO**, not QIO — QIO boot-loops the octal-PSRAM M5.
3. **Boot sound**: Zelda "Secret Discovered" jingle on startup
4. **Flock-You memory**: Max 200 unique detections, oldest overwritten after that — export regularly
5. **Sky Spy dual capture**: Both BLE (UUID 0xFAFF) and WiFi action frames — channel swap window may miss some
6. **AsyncWebServer + NVS**: NVS writes are synchronous — can cause brief freezes in HTTP handlers
7. **Mode persistence**: Selected mode saved to NVS, survives reboot
8. **Flash utility**: `flash.py` for automated flashing (XIAO only — do not use on the M5; see `THINKNODE_M5.md`)