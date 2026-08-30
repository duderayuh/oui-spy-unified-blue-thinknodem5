# ThinkNode M5 — Install Guide

This fork ports **OUI-SPY Unified Blue** from the Seeed Studio XIAO ESP32-S3 to the **Elecrow ThinkNode M5** (ESP32-S3R8, 8 MB octal PSRAM + 4 MB flash).

All six modes are in one build: Detector, Foxhunter, Flock-You, PCAP, Sky Spy, BLE Sniff — selectable from a boot menu.

> ### ⚠️ DISCLAIMER — USE AT YOUR OWN RISK
> This firmware is provided **"AS IS"**, with **no warranty of any kind** (express or implied), including but not limited to fitness for a particular purpose, merchantability, or non-infringement.
>
> By flashing or using this firmware you agree that **you assume all risk**. The authors and contributors are **not liable for any damage**, loss, or cost arising from its use — including, without limitation:
> - damage to your hardware, the ThinkNode M5, connected devices, or any other equipment,
> - bricked or non-functional devices (even though the ESP32-S3 boot ROM is recoverable, no outcome is guaranteed),
> - data loss, battery issues, or unintended radio behaviour,
> - any consequences of operating radio equipment where doing so may be restricted.
>
> Some modes perform **promiscuous Wi-Fi/BLE sniffing, packet capture, and passive surveillance detection**. Use them **only where you are authorized and where such use is lawful**. You are solely responsible for complying with all applicable laws. If in doubt, don't use it.

---

## What's different on the M5

| Signal | XIAO (original) | ThinkNode M5 (this fork) |
|---|---|---|
| Buzzer | GPIO 3 | GPIO 9 |
| Boot button | GPIO 0 | GPIO 21 (BUTTON1) |
| Status LED | GPIO 21 / NeoPixel | Blue LED on PCA9557 I2C expander (0x18) |
| Serial console | native USB-CDC | UART0 (GPIO 43/44) via USB-UART bridge (CH340) |
| Flash / PSRAM | 8 MB + OPI PSRAM | 4 MB + **8 MB octal PSRAM** (ESP32-S3R8) |
| GPS | none | Onboard L76K (switch on GPIO 10) |
| E-ink | none | 1.54" 200×200 driven by this firmware |

---

## Flashing the prebuilt binary (no build required)

The ready-to-flash image is **`firmware-thinknodem5/merged-flash.bin`** (4 MB, all four partitions merged — flash it at address **`0x0`**).

### Option A — esptool.js web flasher (easiest)

1. Plug the ThinkNode M5 into your computer with a **USB-C data cable**.
2. Open the **esptool.js web flasher**: <https://espressif.github.io/esptool-js/>
3. Click **Connect**, pick the serial port (it enumerates as **CH340** — VendorID `0x1a86`, ProductID `0x7522`).
4. In the **Flash Address** field enter **`0x0`**.
5. For the file, choose **`firmware-thinknodem5/merged-flash.bin`** from this repo.
6. Click **Program**. Wait for `Wrote 4194304 bytes` and a `Hash of data verified` message.
7. The device reboots automatically into the OUI-SPY boot selector.

> If your OS doesn't see the serial port, install the **WCH CH340 driver** first (see `FLASH_INSTRUCTIONS.txt`).

### Option B — PlatformIO build & flash

```bash
git clone https://github.com/duderayuh/oui-spy-unified-blue-thinknodem5.git
cd oui-spy-unified-blue-thinknodem5
pio run -e thinknode_m5 -t upload
```

Watch serial output:

```bash
pio device monitor -b 115200
```

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
| 3 · Flock-You | Promiscuous 2.4 GHz Flock Safety / Raven detection (live detection text + GPS stamp on e-ink) |
| 4 · PCAP | Passive WiFi packet capture |
| 5 · Sky Spy | Drone Remote ID detection |
| 6 · BLE Sniff | Passive BLE advertising capture |

---

## Hardware notes

- **Antenna** — the RP-SMA / U.FL connector feeds the **SX1262 LoRa radio (sub-GHz) only**. The 2.4 GHz WiFi/BLE radio uses the ESP32-S3 module's built-in PCB antenna, so swapping the external antenna does *not* affect scanning range.
- **E-ink screen** — driven by this firmware: per-mode icons, a GPS status badge, and (in Flock-You) live "what this beep means" text that auto-reverts to the icon after 30 s.
- **Rotary dial backlight** — powered via the PCA9557 expander; the knob itself is a mechanical power/brightness switch.
- **LED** — the blue "notification" LED is driven over I2C (`board_m5.h`, PCA9557 @ 0x18).

---

## Restoring Meshtastic / MeshCore

To go back to stock firmware, flash via the [Meshtastic web flasher](https://flasher.meshtastic.org/). The ESP32-S3 boot ROM is immutable, so the board can always be re-flashed — it cannot be permanently bricked by firmware.
