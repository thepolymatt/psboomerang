// PSBoomerang by polymatt. Copyright (c) 2026 Unscripted Engineering, LLC.
// https://github.com/thepolymatt/psboomerang
// Licensed under the GNU GPL v3.0 or later. See LICENSE.md in the repository root.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Sticks.h"
#include <Preferences.h>

static const uint8_t PINS[4] = {PIN_STICK_LX, PIN_STICK_LY, PIN_STICK_RX, PIN_STICK_RY};
static const bool INVERT[4] = {INVERT_LX, INVERT_LY, INVERT_RX, INVERT_RY};

static constexpr uint32_t CAL_SETTLE_MS = 1000;     // time to let go of the sticks
static constexpr uint32_t CAL_CENTER_MS = 500;      // center averaging window
static constexpr int16_t CAL_MIN_TRAVEL_MV = 300;   // per side, or the calibration is rejected
static constexpr uint8_t CAL_VERSION = 1;           // bump if AxisCal changes
static constexpr const char *PREFS_NAMESPACE = "sticks";

void Sticks::begin()
{
  // The stick supply (STICK_EN) is already on: Power::begin() runs first.
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db); // ~0-3.1 V usable range on the S3

  sample();
  hasSavedCal = loadCal();
  if (!hasSavedCal)
  {
    // Assume the sticks are at rest at boot and guess the travel until a calibration is saved.
    for (int i = 0; i < 4; i++)
      axes[i] = {int16_t(mv[i] - STICK_DEFAULT_HALF_RANGE_MV), mv[i], int16_t(mv[i] + STICK_DEFAULT_HALF_RANGE_MV)};
  }
}

void Sticks::update()
{
  sample();

  if (cal == CalState::Center)
  {
    uint32_t elapsed = millis() - calStartMs;
    if (elapsed >= CAL_SETTLE_MS)
    {
      for (int i = 0; i < 4; i++)
        centerSum[i] += mv[i];
      centerCount++;
    }
    if (elapsed >= CAL_SETTLE_MS + CAL_CENTER_MS && centerCount)
    {
      for (int i = 0; i < 4; i++)
      {
        int16_t c = centerSum[i] / centerCount;
        pending[i] = {c, c, c};
      }
      cal = CalState::Range;
    }
  }
  else if (cal == CalState::Range)
  {
    for (int i = 0; i < 4; i++)
    {
      pending[i].min = min(pending[i].min, mv[i]);
      pending[i].max = max(pending[i].max, mv[i]);
    }
  }

  if (cal != CalState::Idle)
  {
    lx = ly = rx = ry = 0;
    return;
  }

  float n[4];
  for (int i = 0; i < 4; i++)
    n[i] = INVERT[i] ? -normalize(mv[i], axes[i]) : normalize(mv[i], axes[i]);
  rotate(n[0], n[1], STICK_L_MOUNT_DEG); // stick axes -> controller axes
  rotate(n[2], n[3], STICK_R_MOUNT_DEG);
  shape(n[0], n[1], lx, ly);
  shape(n[2], n[3], rx, ry);
}

void Sticks::startCalibration()
{
  cal = CalState::Center;
  calStartMs = millis();
  memset(centerSum, 0, sizeof(centerSum));
  centerCount = 0;
}

bool Sticks::finishCalibration()
{
  bool wasMeasuringRange = cal == CalState::Range;
  cal = CalState::Idle;
  if (!wasMeasuringRange)
    return false;

  for (int i = 0; i < 4; i++)
  {
    if (pending[i].max - pending[i].center < CAL_MIN_TRAVEL_MV ||
        pending[i].center - pending[i].min < CAL_MIN_TRAVEL_MV)
      return false;
  }
  memcpy(axes, pending, sizeof(axes));
  saveCal();
  hasSavedCal = true;
  return true;
}

void Sticks::sample()
{
  for (int i = 0; i < 4; i++)
  {
    uint32_t sum = 0;
    for (uint8_t s = 0; s < STICK_OVERSAMPLE; s++)
      sum += analogReadMilliVolts(PINS[i]);
    mv[i] = sum / STICK_OVERSAMPLE;
  }
}

bool Sticks::loadCal()
{
  Preferences prefs;
  if (!prefs.begin(PREFS_NAMESPACE, true))
    return false; // namespace doesn't exist yet: never calibrated
  bool ok = prefs.getUChar("ver", 0) == CAL_VERSION &&
            prefs.getBytes("axes", axes, sizeof(axes)) == sizeof(axes);
  prefs.end();
  return ok;
}

void Sticks::saveCal()
{
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.putUChar("ver", CAL_VERSION);
  prefs.putBytes("axes", axes, sizeof(axes));
  prefs.end();
}

// Map millivolts to -1..1 using separate spans for each side of center (sticks are rarely symmetric).
float Sticks::normalize(int16_t v, const AxisCal &c)
{
  int span = v >= c.center ? c.max - c.center : c.center - c.min;
  return span > 0 ? float(v - c.center) / span : 0.0f;
}

// Undo a stick mounted `degrees` clockwise (front view). Screen convention, +Y down.
void Sticks::rotate(float &x, float &y, float degrees)
{
  if (degrees == 0.0f)
    return;
  float r = degrees * DEG_TO_RAD, c = cosf(r), s = sinf(r);
  float rx = x * c - y * s;
  y = x * s + y * c;
  x = rx;
}

// Radial deadzone: inside the inner radius reads zero, and the rest is rescaled so output
// starts at 0 at the inner edge and hits full scale at the outer edge. Direction is kept.
void Sticks::shape(float x, float y, int16_t &outX, int16_t &outY)
{
  float r = sqrtf(x * x + y * y);
  if (r <= STICK_DEADZONE_INNER)
  {
    outX = outY = 0;
    return;
  }
  float scaled = (r - STICK_DEADZONE_INNER) / (STICK_DEADZONE_OUTER - STICK_DEADZONE_INNER);
  float k = min(scaled, 1.0f) / r;
  outX = (int16_t)lroundf(x * k * 32767.0f);
  outY = (int16_t)lroundf(y * k * 32767.0f);
}
