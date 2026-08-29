# ThinkNode M5 — Install Guide

This fork ports **OUI-SPY Unified Blue** from the Seeed Studio XIAO ESP32-S3 to the **Elecrow ThinkNode M5** (ESP32-S3-WROOM-1-N4).

All six modes are in one build: Detector, Foxhunter, Flock-You, PCAP, Sky Spy, BLE Sniff — selectable from a boot menu.

---

## What's different on the M5

| Signal | XIAO (original) | ThinkNode M5 (this fork) |
|---|---|---|
| Buzzer | GPIO 3 | GPIO 9 |
| Boot button | GPIO 0 | GPIO 21 (BUTTON1) |
| Status LED | GPIO 21 / NeoPixel | Blue LED on PCA9557 I2C expander (0x18) |
| Serial console | native USB-CDC | UART0 (GPIO 43/44) via USB-UART bridge |
| Flash / PSRAM | 8 MB + OPI PSRAM | 4 MB, **no PSRAM** |

---

## Prerequisites

- **PlatformIO Core** — `brew install platformio` (macOS) or `pip install platformio`
- A **USB-C data cable** (charge-only cables won't expose the port)
- **CH340 / CP210x driver** if your OS doesn't auto-detect the serial port (see `FLASH_INSTRUCTIONS.txt`)

---

## Build & flash

```bash
git clone https://github.com/duderayuh/oui-spy-unified-blue-thinknodem5.git
cd oui-spy-unified-blue-thinknodem5
pio run -e thinknode_m5 -t upload
```

Watch serial output:

```bash
pio device monitor -b 115200
```

> ⚠️ **Do not use `flash.py` or the web flasher** — they ship the original XIAO binaries from the `firmware/` folder. The ThinkNode M5 build is flashed with `pio run -t upload` above.

---

## Using it

- On **power-on**, hold **BUTTON1 (GPIO 21)** to force the selector menu.
- Otherwise it boots into the last-saved mode. From any mode, hold **BUTTON1 ~1.5 s** to return to the menu.
- The selector (and most modes) brings up a WiFi AP — connect and open **`192.168.4.1`**.

### Modes

| Mode | Function |
|---|---|
| 1 · Detector | BLE OUI/MAC watchlist + alerts |
| 2 · Foxhunter | RSSI proximity tracker |
| 3 · Flock-You | Promiscuous 2.4 GHz Flock Safety / Raven detection |
| 4 · PCAP | Passive WiFi packet capture |
| 5 · Sky Spy | Drone Remote ID detection |
| 6 · BLE Sniff | Passive BLE advertising capture |

---

## Hardware notes

- **Antenna** — the RP-SMA / U.FL connector feeds the **SX1262 LoRa radio (sub-GHz) only**. The 2.4 GHz WiFi/BLE radio uses the ESP32-S3 module's built-in PCB antenna, so swapping the external antenna does *not* affect scanning range.
- **E-ink screen** — not driven by this firmware. It will keep displaying whatever was last rendered (frozen), since e-ink is bistable.
- **LED** — the blue "notification" LED is driven over I2C (`board_m5.h`, PCA9557 @ 0x18).

---

## Verify on first boot

A few board-specific assumptions couldn't be confirmed without hardware in hand. Check these on first flash:

1. **Boot-button polarity** — if holding BUTTON1 doesn't enter the menu, the active level is wrong (flip the `== LOW` checks in `src/main.cpp`).
2. **LED polarity** — if the blue LED is inverted or stays dark, flip the level in `src/raw/board_m5.h` (`M5Board::led`).
3. **Buzzer type** — if it's an *active* (self-oscillating) buzzer, PWM/tone is redundant; if it sticks on, it's passive and needs PWM.

---

## Restoring Meshtastic / MeshCore

To go back to stock firmware, flash via the [Meshtastic web flasher](https://flasher.meshtastic.org/). The ESP32-S3 boot ROM is immutable, so the board can always be re-flashed — it cannot be permanently bricked by firmware.
