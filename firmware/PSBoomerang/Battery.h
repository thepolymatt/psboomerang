// PSBoomerang by polymatt. Copyright (c) 2026 Unscripted Engineering, LLC.
// https://github.com/thepolymatt/psboomerang
// Licensed under the GNU GPL v3.0 or later. See LICENSE.md in the repository root.
// SPDX-License-Identifier: GPL-3.0-or-later

// LiPo voltage (through a divider) to percent, plus USB power detection.
#pragma once
#include <Arduino.h>
#include "Config.h"

class Battery
{
public:
  void begin();
  void update();               // call every loop; samples every BATTERY_SAMPLE_MS

  uint8_t percent() const { return pct; }
  float volts() const { return v; }
  bool usbPowered() const { return usb; }
  bool consumeChange();        // true once after percent or USB state changes

private:
  static float readVolts();
  static uint8_t toPercent(float volts);

  float v = 0;
  uint8_t pct = 0;
  bool usb = false;
  bool changed = true;
  uint32_t lastSampleMs = 0;
};
