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
// panel powered off (hibernate), so we draw once per mode change and sleep.
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

static SPIClass* hspi = nullptr;
static GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT>* display = nullptr;

inline void begin() {
    if (display) return;
    hspi = new SPIClass(HSPI);
    hspi->begin(M5_EINK_SCLK, -1, M5_EINK_MOSI, M5_EINK_CS); // SCLK, MISO, MOSI, SS

    GxEPD2_154_D67* low = new GxEPD2_154_D67(M5_EINK_CS, M5_EINK_DC,
                                             M5_EINK_RES, M5_EINK_BUSY, *hspi);
    display = new GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT>(*low);
    display->init();
    display->setRotation(4);   // matches Meshtastic orientation for this board
    display->setPartialWindow(0, 0, 200, 200);
}

// Draw a full-screen 200x200 1-bit icon (MSB-first, PROGMEM) then hibernate.
inline void showIcon(const uint8_t* bitmap) {
    if (!display || !bitmap) return;
    display->setFullWindow();
    display->firstPage();
    do {
        display->fillScreen(GxEPD_WHITE);
        display->drawBitmap(0, 0, bitmap, 200, 200, GxEPD_BLACK, GxEPD_WHITE);
    } while (display->nextPage());
    display->hibernate();
}

// Draw the mode icon plus an optional GPS badge (40x40) in the bottom-right
// corner. badge is the badge bitmap; showBadge controls whether it's drawn.
// Used for the live GPS-status overlay (OFF=hidden, ACQUIRING=blink, LOCKED=solid).
inline void showIconWithGps(const uint8_t* bitmap, const uint8_t* badge, bool showBadge) {
    if (!display || !bitmap) return;
    display->setFullWindow();
    display->firstPage();
    do {
        display->fillScreen(GxEPD_WHITE);
        display->drawBitmap(0, 0, bitmap, 200, 200, GxEPD_BLACK, GxEPD_WHITE);
        if (showBadge && badge) {
            display->drawBitmap(156, 156, badge, 40, 40, GxEPD_BLACK, GxEPD_WHITE);
        }
    } while (display->nextPage());
    display->hibernate();
}

} // namespace M5Display
