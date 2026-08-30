// =============================================================================
// ThinkNode M5 board abstraction
// =============================================================================
// The M5 has no plain-GPIO user LED or NeoPixel. Its two LEDs (blue "notification"
// and red "power") live on a PCA9557 I2C GPIO expander at address 0x18, wired to
// the second I2C bus (Wire1: SDA=48, SCL=47). This header provides the minimal
// driver the firmware needs to blink the blue LED as a detect indicator.
//
// Pin map sourced from the Meshtastic ELECROW-ThinkNode-M5 variant:
//   https://github.com/meshtastic/firmware/tree/master/variants/esp32s3/ELECROW-ThinkNode-M5
//
// NOTE: this header is included from multiple translation units (main.cpp AND the
// per-mode wrappers, each inside its own anonymous namespace). Do NOT cache output
// state in a static variable here — each TU would get its own copy and clobber the
// expander. led() reconstructs the full output register every write instead.
// =============================================================================
#pragma once

#include <Arduino.h>
#include <Wire.h>

#define M5_BUZZER_PIN      9     // active buzzer (PWM-capable)
#define M5_BOOT_PIN        21    // BUTTON1 (hold to return to selector)
#define M5_BUTTON2_PIN     14    // BUTTON2 (unused by this firmware)

// PCA9557 GPIO expander
#define PCA9557_ADDR       0x18
#define PCA9557_SDA        48
#define PCA9557_SCL        47
#define PCA_LED_BLUE       1     // "notification" LED  (HIGH = on)
#define PCA_LED_ENABLE     2     // LED power supply (OR'd with VBUS power)
#define PCA_LED_RED        3     // "power" LED (hardware-blinks while charging)
#define PCA_POWER_EN       4     // power for peripherals (eink+GPS+LoRa+sensor)
#define PCA_EINK_EN        5     // e-ink backlight power

// PCA9557 registers
#define PCA9557_REG_INPUT     0x00
#define PCA9557_REG_OUTPUT    0x01
#define PCA9557_REG_POLARITY  0x02
#define PCA9557_REG_CONFIG    0x03

namespace M5Board {

inline void _writeReg(uint8_t reg, uint8_t val) {
    Wire1.beginTransmission(PCA9557_ADDR);
    Wire1.write(reg);
    Wire1.write(val);
    Wire1.endTransmission();
}

// Initialize Wire1 + the PCA9557. Idempotent — safe to call from more than one
// place. Powers on the peripheral rail AND the backlight rail (PCA_EINK_EN,
// pin 5 — Meshtastic comments this as "really just backlight power"). The
// rotary knob is a mechanical power/brightness switch; our only job is to keep
// its power rail HIGH so the knob actually controls it. Also powers the LED
// rail (PCA_LED_ENABLE, pin 2) so the status LEDs work on battery, not just VBUS.
inline void begin() {
    Wire1.begin(PCA9557_SDA, PCA9557_SCL);
    _writeReg(PCA9557_REG_CONFIG, 0x00);   // all used pins -> outputs
    uint8_t out = (1 << PCA_POWER_EN)      // peripherals (eink+GPS+LoRa+sensor)
                | (1 << PCA_LED_ENABLE)    // LED power supply
                | (1 << PCA_EINK_EN);      // backlight power (rotary knob)
    _writeReg(PCA9557_REG_OUTPUT, out);
}

// Blue "notification" LED. HIGH = on. Reconstructs the whole output register so
// the peripheral-power + backlight bits are never clobbered regardless of which
// TU calls it.
inline void led(bool on) {
    uint8_t out = (1 << PCA_POWER_EN)      // keep peripherals powered
                | (1 << PCA_LED_ENABLE)    // keep LED rail powered
                | (1 << PCA_EINK_EN);      // keep backlight powered
    if (on) out |= (1 << PCA_LED_BLUE);
    _writeReg(PCA9557_REG_OUTPUT, out);
}

} // namespace M5Board
