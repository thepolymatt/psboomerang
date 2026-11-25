// PSBoomerang by polymatt. Copyright (c) 2026 Unscripted Engineering, LLC.
// https://github.com/thepolymatt/psboomerang
// Licensed under the GNU GPL v3.0 or later. See LICENSE.md in the repository root.
// SPDX-License-Identifier: GPL-3.0-or-later

// WS2812 status LEDs. LED 0 shows controller state and battery. LED 1 (if fitted) shows the
// color the host sets over SInput, falling back to mirroring LED 0.
#pragma once
#include <Adafruit_NeoPixel.h>
#include "Config.h"
#include "Sticks.h"

struct LedInputs
{
  bool fault;                  // button expander missing
  bool calArmed;               // calibration chord held long enough; release to start
  CalState cal;
  bool connected;
  bool usbPowered;
  uint8_t battery;             // percent
  bool supplyLow;              // battery below the LEDs' minimum VDD: keep them dark
};

class StatusLed
{
public:
  void begin();
  void update(const LedInputs &in); // call every loop
  void setHostColor(uint8_t r6, uint8_t g6, uint8_t b6); // SInput sends 6-bit channels
  void flash(uint32_t color, uint32_t ms); // overrides the pattern briefly

private:
  Adafruit_NeoPixel strip{LED_COUNT, PIN_LED, NEO_GRB + NEO_KHZ800};
  uint32_t hostColor = 0;
  bool hasHostColor = false;
  uint32_t flashColor = 0;
  uint32_t flashUntilMs = 0;
  uint32_t lastFrameMs = 0;
  uint32_t shown[2] = {1, 1}; // force the first frame out
};
