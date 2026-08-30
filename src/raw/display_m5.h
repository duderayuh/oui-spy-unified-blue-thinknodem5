// =============================================================================
// ThinkNode M5 e-ink display driver
// =============================================================================
// 1.54" 200x200 monochrome e-ink panel (GDEH0154D67, SSD1681 controller).
// The panel lives on its own SPI bus (HSPI), separate from the LoRa radio SPI,
// so it needs the Meshtastic GxEPD2 fork which adds a SPIClass& constructor.
//
// Pins (Meshtastic ELECROW-ThinkNode-M5 variant):
//   CS=39  DC=40  RST=41  BUSY=42  SCLK=38  MOSI=45
// Panel power comes from PCA9557 POWER_EN (pin 4), already driven HIGH by
// M5Board::begin(). E-ink is bistable: once drawn, it keeps the image with the
// panel powered off (hibernate), so we draw once per change and sleep.
//
// IMPORTANT: the display object is shared across translation units via a
// function-local static (Meyers singleton). This header is included from both
// main.cpp AND the per-mode wrappers (e.g. flock-you), and a namespace-scope
// `static` pointer would give each TU its own null copy — flock-you would draw
// to a display that doesn't exist. The accessor makes it the ONE instance.
// =============================================================================
#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_BW.h>   // pulls in epd/GxEPD2_154_D67.h

#define M5_EINK_CS    39
#define M5_EINK_DC    40
#define M5_EINK_RES   41
#define M5_EINK_BUSY  42
#define M5_EINK_SCLK  38
#define M5_EINK_MOSI  45

namespace M5Display {

using DisplayType = GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT>;

inline SPIClass*& hspi() {
    static SPIClass* p = nullptr;
    return p;
}

inline DisplayType*& display() {
    static DisplayType* p = nullptr;
    return p;
}

inline void begin() {
    if (display()) return;
    hspi() = new SPIClass(HSPI);
    hspi()->begin(M5_EINK_SCLK, -1, M5_EINK_MOSI, M5_EINK_CS); // SCLK, MISO, MOSI, SS

    GxEPD2_154_D67* low = new GxEPD2_154_D67(M5_EINK_CS, M5_EINK_DC,
                                             M5_EINK_RES, M5_EINK_BUSY, *hspi());
    display() = new DisplayType(*low);
    display()->init();
    display()->setRotation(4);   // matches Meshtastic orientation for this board
    display()->setPartialWindow(0, 0, 200, 200);
}

// Draw a full-screen 200x200 1-bit icon (MSB-first, PROGMEM) then hibernate.
inline void showIcon(const uint8_t* bitmap) {
    if (!display() || !bitmap) return;
    display()->setFullWindow();
    display()->firstPage();
    do {
        display()->fillScreen(GxEPD_WHITE);
        display()->drawBitmap(0, 0, bitmap, 200, 200, GxEPD_BLACK, GxEPD_WHITE);
    } while (display()->nextPage());
    display()->hibernate();
}

// Draw the mode icon plus an optional GPS badge (40x40) in the bottom-right
// corner. badge is the badge bitmap; showBadge controls whether it's drawn.
inline void showIconWithGps(const uint8_t* bitmap, const uint8_t* badge, bool showBadge) {
    if (!display() || !bitmap) return;
    display()->setFullWindow();
    display()->firstPage();
    do {
        display()->fillScreen(GxEPD_WHITE);
        display()->drawBitmap(0, 0, bitmap, 200, 200, GxEPD_BLACK, GxEPD_WHITE);
        if (showBadge && badge) {
            display()->drawBitmap(156, 156, badge, 40, 40, GxEPD_BLACK, GxEPD_WHITE);
        }
    } while (display()->nextPage());
    display()->hibernate();
}

// Draw a live text status screen, then hibernate. Used by flock-you to explain
// each detection beep on the panel. Layout:
//   title   (size 2, top)
//   meaning (size 2, the human-readable "what this beep means")
//   line2..line4 (size 1, details — MAC / SSID / RSSI+channel)
// Empty (null or "") lines are skipped. textWrap is off; callers should keep
// each line within the panel width (~19 chars at size 2, ~32 at size 1).
inline void showText(const char* title, const char* meaning,
                     const char* line2, const char* line3, const char* line4) {
    if (!display()) return;
    display()->setFullWindow();
    display()->firstPage();
    do {
        display()->fillScreen(GxEPD_WHITE);
        display()->setTextColor(GxEPD_BLACK);
        display()->setTextWrap(false);

        display()->setTextSize(2);
        display()->setCursor(4, 8);
        if (title) display()->print(title);

        display()->setTextSize(2);
        display()->setCursor(4, 44);
        if (meaning) display()->print(meaning);

        // Horizontal rule between the meaning and the detail block.
        display()->fillRect(4, 76, 192, 2, GxEPD_BLACK);

        display()->setTextSize(1);
        int y = 86;
        const char* lines[3] = { line2, line3, line4 };
        for (int i = 0; i < 3; i++) {
            if (lines[i] && lines[i][0]) {
                display()->setCursor(4, y);
                display()->print(lines[i]);
                y += 14;
            }
        }
    } while (display()->nextPage());
    display()->hibernate();
}

// Idle-icon memory: main.cpp stores the active mode's icon at boot so a mode
// that draws live status (flock-you) can revert to it after a quiet period —
// without pulling the 5 KB PROGMEM array into every translation unit.
inline const uint8_t*& idleIcon() {
    static const uint8_t* p = nullptr;
    return p;
}

inline void setIdleIcon(const uint8_t* bmp) { idleIcon() = bmp; }

// Redraw the stored idle icon (the mode's normal face). No-op if unset.
inline void revertToIdle() {
    if (idleIcon()) showIcon(idleIcon());
}

} // namespace M5Display
