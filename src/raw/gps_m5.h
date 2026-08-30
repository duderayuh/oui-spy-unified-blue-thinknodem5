// =============================================================================
// ThinkNode M5 GPS status driver (L76K GNSS)
// =============================================================================
// The M5 has an L76K GNSS on UART, plus a physical "GPS Switch" slide toggle
// (toggle #7 on the case). Pins (Meshtastic ELECROW-ThinkNode-M5 variant):
//   GPS switch input : GPIO10  (HIGH = enabled, LOW = disabled)
//   GPS STANDBY      : GPIO11  (HIGH = force wake/on)
//   GPS REINIT/reset : GPIO13  (LOW >100ms = reset; keep HIGH for normal)
//   GPS UART         : RX=20 (bits toward CPU), TX=19 (bits toward GPS), 9600 baud
//
// We use Serial2 (UART2) — Serial1 is already taken by flockyou mode's TX-only
// mirror on GPIO43, and Serial (UART0) is the console.
//
// State machine (exposed to the display/buzzer layer):
//   OFF       — GPS switch is off
//   ACQUIRING — switch on, no valid fix yet
//   LOCKED    — valid fix (lat/lon valid + recent)
// =============================================================================
#pragma once

#include <Arduino.h>
#include <TinyGPSPlus.h>

#define M5_GPS_SWITCH   10   // physical slide switch (input)
#define M5_GPS_STANDBY  11   // HIGH = force wake
#define M5_GPS_REINIT   13   // LOW >100ms = reset, HIGH = normal
#define M5_GPS_RX       20   // CPU RX <- GPS TX
#define M5_GPS_TX       19   // CPU TX -> GPS RX
#define M5_GPS_BAUD     9600

// Switch polarity. Meshtastic treats LOW as "disabled", so HIGH = on. If your
// particular unit reads inverted, flip this to `== LOW` / `digitalRead(...)==LOW`.
#define M5_GPS_SWITCH_ON() (digitalRead(M5_GPS_SWITCH) == HIGH)

// A fix older than this is treated as stale (revert to ACQUIRING).
#define M5_GPS_STALE_MS 5000

namespace M5GPS {

enum State : uint8_t { OFF = 0, ACQUIRING = 1, LOCKED = 2 };

// Single shared parser instance across ALL translation units. A namespace-scope
// `static TinyGPSPlus` here would give every TU its own (empty, never-pumped)
// copy — main.cpp pumps one copy, flock-you's TU would read a different one and
// always report "no fix". A function-local static in an inline function has
// external linkage and is the ONE instance everywhere (Meyers singleton).
inline TinyGPSPlus& gpsInstance() {
    static TinyGPSPlus gps;
    return gps;
}

inline void begin() {
    pinMode(M5_GPS_SWITCH, INPUT_PULLUP);
    pinMode(M5_GPS_STANDBY, OUTPUT);
    pinMode(M5_GPS_REINIT, OUTPUT);
    digitalWrite(M5_GPS_STANDBY, HIGH);  // force wake
    digitalWrite(M5_GPS_REINIT, HIGH);   // not in reset
    Serial2.begin(M5_GPS_BAUD, SERIAL_8N1, M5_GPS_RX, M5_GPS_TX);
}

// Feed any pending NMEA bytes into the parser. Non-blocking.
inline void pump() {
    TinyGPSPlus& gps = gpsInstance();
    while (Serial2.available() > 0) {
        gps.encode(Serial2.read());
    }
}

// Current GPS state given the switch + fix status.
inline State state() {
    if (!M5_GPS_SWITCH_ON()) return OFF;
    TinyGPSPlus& gps = gpsInstance();
    bool recent = (millis() - gps.location.age()) < M5_GPS_STALE_MS;
    if (gps.location.isValid() && recent) return LOCKED;
    return ACQUIRING;
}

inline double latitude()  { return gpsInstance().location.lat(); }
inline double longitude() { return gpsInstance().location.lng(); }
inline uint8_t satellites() { return gpsInstance().satellites.value(); }
// Horizontal dilution of precision (dimensionless). 0.0 when not yet valid.
inline double hdop() {
    return gpsInstance().hdop.isValid() ? gpsInstance().hdop.hdop() : 0.0;
}

} // namespace M5GPS

// 40x40 1-bit GPS crosshair badge (MSB-first, PROGMEM). Overlaid in a corner of
// the mode icon when GPS is active.
static const uint8_t ICON_GPS[200] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x01,0xff,0xc0,
  0x00,0x00,0x0f,0xff,0xf8,0x00,0x00,0x1f,0x80,0xfc,0x00,0x00,
  0x3c,0x3c,0x1e,0x00,0x00,0xf0,0x3c,0x07,0x80,0x00,0xe0,0x3c,
  0x03,0x80,0x01,0xc0,0x3c,0x01,0xc0,0x03,0x80,0x3c,0x00,0xe0,
  0x07,0x00,0x3c,0x00,0x70,0x07,0x00,0x3c,0x00,0x70,0x06,0x00,
  0x3c,0x00,0x30,0x0e,0x00,0x3c,0x00,0x38,0x0e,0x00,0x3c,0x00,
  0x38,0x0c,0x00,0x3c,0x00,0x18,0x0d,0xff,0xff,0xff,0xd8,0x0d,
  0xff,0xff,0xff,0xd8,0x1d,0xff,0xff,0xff,0xdc,0x0d,0xff,0xff,
  0xff,0xd8,0x0c,0x00,0x3e,0x00,0x18,0x0c,0x00,0x3c,0x00,0x18,
  0x0e,0x00,0x3c,0x00,0x38,0x0e,0x00,0x3c,0x00,0x38,0x06,0x00,
  0x3c,0x00,0x30,0x07,0x00,0x3c,0x00,0x70,0x07,0x00,0x3c,0x00,
  0x70,0x03,0x80,0x3c,0x00,0xe0,0x01,0xc0,0x3c,0x01,0xc0,0x00,
  0xe0,0x3c,0x03,0x80,0x00,0xf0,0x3c,0x07,0x80,0x00,0x3c,0x3c,
  0x1e,0x00,0x00,0x1f,0x80,0xfc,0x00,0x00,0x0f,0xff,0xf8,0x00,
  0x00,0x01,0xff,0xc0,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};
