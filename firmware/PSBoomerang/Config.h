// PSBoomerang by polymatt. Copyright (c) 2026 Unscripted Engineering, LLC.
// https://github.com/thepolymatt/psboomerang
// Licensed under the GNU GPL v3.0 or later. See LICENSE.md in the repository root.
// SPDX-License-Identifier: GPL-3.0-or-later

// Board configuration: pins, button map, tuning. Pins match the main board (see the pin map in docs/PCB-Notes.md).
#pragma once
#include <Arduino.h>

// With USB CDC On Boot disabled, Serial is UART0 on GPIO43/44, which are STICK_EN and VBUS_SENSE here.
#if !ARDUINO_USB_CDC_ON_BOOT
#error "Set Tools > USB CDC On Boot > Enabled (Serial must not use GPIO43/44)"
#endif
// Wired mode adds a USB gamepad next to the serial port, which needs the TinyUSB stack.
#if ARDUINO_USB_MODE
#error "Set Tools > USB Mode > USB-OTG (TinyUSB), and Tools > Upload Mode > USB-OTG CDC (TinyUSB)"
#endif

#define DEVICE_NAME         "PSBoomerang"
#define DEVICE_MANUFACTURER "polymatt"

// ---- XIAO ESP32-S3 pin plan ----
constexpr uint8_t PIN_STICK_LX   = A0;  // D0 / GPIO1
constexpr uint8_t PIN_STICK_LY   = A1;  // D1 / GPIO2
constexpr uint8_t PIN_STICK_RX   = A2;  // D2 / GPIO3
constexpr uint8_t PIN_STICK_RY   = A3;  // D3 / GPIO4
constexpr uint8_t PIN_I2C_SDA    = D4;  // GPIO5
constexpr uint8_t PIN_I2C_SCL    = D5;  // GPIO6
constexpr uint8_t PIN_STICK_EN   = D6;  // GPIO43 (UART TX, toggles during boot log; harmless here), stick LDO (U3) + LED load switch (U4) enable
constexpr uint8_t PIN_VBUS_SENSE = D7;  // GPIO44 (UART RX, input at boot), USB 5V through a 22k/33k divider
constexpr uint8_t PIN_BATTERY    = A8;  // D8 / GPIO7, battery through a 220k/220k divider
constexpr uint8_t PIN_LED        = D9;  // GPIO8, WS2812 data in
constexpr uint8_t PIN_HOME       = D10; // GPIO9, Home/PS button to GND, 10k pull-up (RTC-capable: the wake-from-sleep button)

// ---- PCA9555 button expander (TCA9555 is register- and pin-compatible) ----
constexpr uint8_t EXPANDER_I2C_ADDR = 0x20; // A0-A2 tied to GND
constexpr uint32_t I2C_CLOCK_HZ     = 400000;

// Bits 0-15 are the expander pins (P00-P07 = bits 0-7, P10-P17 = bits 8-15); reorder
// freely to suit the PCB routing. BTN_HOME is wired straight to the XIAO (PIN_HOME).
enum ButtonBit : uint8_t
{
  BTN_DPAD_UP = 0,  // P00
  BTN_DPAD_DOWN,    // P01
  BTN_DPAD_LEFT,    // P02
  BTN_DPAD_RIGHT,   // P03
  BTN_SOUTH,        // P04  A / Cross
  BTN_EAST,         // P05  B / Circle
  BTN_WEST,         // P06  X / Square
  BTN_NORTH,        // P07  Y / Triangle
  BTN_L1,           // P10
  BTN_R1,           // P11
  BTN_L2,           // P12
  BTN_R2,           // P13
  BTN_L3,           // P14  left stick click
  BTN_R3,           // P15  right stick click
  BTN_SELECT,       // P16
  BTN_START,        // P17
  BTN_HOME,         // PIN_HOME (PS / Guide)
  BUTTON_BIT_COUNT
};

constexpr uint8_t BUTTON_DEBOUNCE_MS = 5; // lockout after an edge; the first edge is reported immediately

// ---- Sticks ----
constexpr uint8_t STICK_OVERSAMPLE = 4;      // ADC reads averaged per axis per scan
constexpr float   STICK_DEADZONE_INNER = 0.06f; // radial, fraction of full deflection
constexpr float   STICK_DEADZONE_OUTER = 0.95f; // deflection at or beyond this reports 100%
constexpr int     STICK_DEFAULT_HALF_RANGE_MV = 1100; // used until the first calibration is saved

// Flip these so that, relative to the stick's own body, right = +X and down = +Y (SDL convention),
// i.e. what you'd want if the stick were mounted upright.
constexpr bool INVERT_LX = false;
constexpr bool INVERT_LY = false;
constexpr bool INVERT_RX = false;
constexpr bool INVERT_RY = false;

// Angle each stick is mounted at, in degrees clockwise as seen from the front of the controller.
// Any angle works (90, -90, 30...). Calibration is unaffected: it runs in the stick's own axes.
// To check: with DEBUG_INPUT on, push straight up and confirm the output is (0, -32767).
constexpr float STICK_L_MOUNT_DEG = 180.0f;
constexpr float STICK_R_MOUNT_DEG = -100.0f;

// ---- Face buttons ----
// SInput hosts (Steam, SDL3 games) place the face buttons by position: false is correct there.
// Hosts that see a generic gamepad (Safari, and so joypad.ai on a Mac) read them in Nintendo order,
// which swaps top with left and bottom with right. true pre-swaps them for those hosts, and then
// they read swapped in Steam instead.
constexpr bool SWAP_FACE_BUTTONS = true;

// ---- Chords ----
// Holding Select + L3 + R3 for this long enters stick calibration.
constexpr uint32_t CALIBRATION_HOLD_MS = 3000;

// ---- Battery ----
constexpr float    BATTERY_DIVIDER_RATIO = 2.0f; // 220k / 220k
constexpr uint32_t BATTERY_SAMPLE_MS = 1000;

// ---- Power ----
// The controller deep-sleeps (~25 uA) and a long press of Home wakes it.
constexpr uint32_t SLEEP_HOLD_MS = 3000;           // hold Home this long to sleep
constexpr uint32_t WAKE_HOLD_MS  = 1000;           // and this long to wake; shorter presses go back to sleep
constexpr uint32_t IDLE_SLEEP_MS = 10 * 60 * 1000; // sleep after this long without input, on battery only

// ---- Status LED ----
constexpr uint8_t LED_COUNT = 2;
constexpr uint8_t LED_BRIGHTNESS = 40; // 0-255; these sit close to the player's eyes
constexpr float   LED_MIN_VBAT = 3.5f; // Würth WL-ICLED minimum VDD; LEDs run from the battery

// ---- Reporting ----
constexpr uint32_t REPORT_MIN_INTERVAL_US = 5000; // cap BLE reports at 200 Hz
constexpr bool DEBUG_INPUT = true; // print raw stick mV and button bits to Serial at 10 Hz
