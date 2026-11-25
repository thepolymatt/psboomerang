// PSBoomerang by polymatt. Copyright (c) 2026 Unscripted Engineering, LLC.
// https://github.com/thepolymatt/psboomerang
// Licensed under the GNU GPL v3.0 or later. See LICENSE.md in the repository root.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Battery.h"

// Rough resting-voltage curve for a 1-cell LiPo. It reads high while charging.
struct CurvePoint { float volts; uint8_t pct; };
static const CurvePoint CURVE[] = {
  {3.30f, 0}, {3.60f, 5}, {3.70f, 15}, {3.75f, 25}, {3.80f, 40},
  {3.85f, 55}, {3.95f, 70}, {4.05f, 85}, {4.15f, 95}, {4.20f, 100},
};

void Battery::begin()
{
  pinMode(PIN_VBUS_SENSE, INPUT); // the external divider holds it low when unplugged
  v = readVolts();
  pct = toPercent(v);
  usb = digitalRead(PIN_VBUS_SENSE);
  lastSampleMs = millis();
}

void Battery::update()
{
  if (millis() - lastSampleMs < BATTERY_SAMPLE_MS)
    return;
  lastSampleMs = millis();

  v = v * 0.8f + readVolts() * 0.2f; // smooth out radio-burst sag
  uint8_t newPct = toPercent(v);
  bool newUsb = digitalRead(PIN_VBUS_SENSE);
  if (newPct != pct || newUsb != usb)
    changed = true;
  pct = newPct;
  usb = newUsb;
}

bool Battery::consumeChange()
{
  bool c = changed;
  changed = false;
  return c;
}

float Battery::readVolts()
{
  uint32_t sum = 0;
  for (int i = 0; i < 8; i++)
    sum += analogReadMilliVolts(PIN_BATTERY);
  return sum / 8 * BATTERY_DIVIDER_RATIO / 1000.0f;
}

uint8_t Battery::toPercent(float volts)
{
  const size_t n = sizeof(CURVE) / sizeof(CURVE[0]);
  if (volts <= CURVE[0].volts)
    return 0;
  for (size_t i = 1; i < n; i++)
  {
    if (volts < CURVE[i].volts)
    {
      const CurvePoint &a = CURVE[i - 1], &b = CURVE[i];
      return a.pct + (volts - a.volts) / (b.volts - a.volts) * (b.pct - a.pct);
    }
  }
  return 100;
}
