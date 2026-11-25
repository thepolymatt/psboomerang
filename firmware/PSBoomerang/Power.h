// PSBoomerang by polymatt. Copyright (c) 2026 Unscripted Engineering, LLC.
// https://github.com/thepolymatt/psboomerang
// Licensed under the GNU GPL v3.0 or later. See LICENSE.md in the repository root.
// SPDX-License-Identifier: GPL-3.0-or-later

// Board power: the stick LDO (U3) and the LED load switch (U4) share STICK_EN. There's no power
// switch: the controller deep-sleeps, and a long press of Home (an RTC GPIO) wakes it.
#pragma once
#include <Arduino.h>
#include "Config.h"

class Power
{
public:
  // Call first in setup(). After a wake from deep sleep it goes straight back to sleep unless
  // Home is held for WAKE_HOLD_MS; otherwise it powers the sticks and LEDs.
  void begin();

  // Takes LED data low, cuts the stick and LED supply, waits for Home to be released, then
  // deep-sleeps until Home is pressed. Never returns: the chip resets on wake.
  [[noreturn]] void sleep();
};
