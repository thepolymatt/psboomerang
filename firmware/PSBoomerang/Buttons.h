// PSBoomerang by polymatt. Copyright (c) 2026 Unscripted Engineering, LLC.
// https://github.com/thepolymatt/psboomerang
// Licensed under the GNU GPL v3.0 or later. See LICENSE.md in the repository root.
// SPDX-License-Identifier: GPL-3.0-or-later

// 16 buttons read through a PCA9555 I2C expander plus Home on a direct GPIO, with eager debounce.
#pragma once
#include <Arduino.h>
#include "Config.h"

class Buttons
{
public:
  bool begin();                // false if the expander doesn't answer on I2C
  void update();               // call every loop
  bool pressed(ButtonBit b) const { return state & (1ul << b); }
  uint32_t raw() const { return state; }

private:
  bool readExpander(uint16_t &out);
  bool writeReg(uint8_t reg, uint8_t value);

  bool expanderOk = false;
  uint16_t expanderBits = 0;   // last good expander read
  uint32_t state = 0;          // debounced, 1 = pressed
  uint32_t lastEdgeMs[BUTTON_BIT_COUNT] = {};
};
