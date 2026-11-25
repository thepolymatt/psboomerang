// PSBoomerang by polymatt. Copyright (c) 2026 Unscripted Engineering, LLC.
// https://github.com/thepolymatt/psboomerang
// Licensed under the GNU GPL v3.0 or later. See LICENSE.md in the repository root.
// SPDX-License-Identifier: GPL-3.0-or-later

// Two TMR analog sticks on the ESP32-S3 ADC: calibration (saved to flash), mounting-angle
// correction, radial deadzone.
#pragma once
#include <Arduino.h>
#include "Config.h"

enum class CalState : uint8_t { Idle, Center, Range };

class Sticks
{
public:
  void begin();
  void update();               // call every loop

  // Outputs, -32767..32767, +X = right, +Y = down
  int16_t lx = 0, ly = 0, rx = 0, ry = 0;

  bool calibrated() const { return hasSavedCal; }
  const int16_t *rawMv() const { return mv; } // LX, LY, RX, RY

  // Calibration: startCalibration() -> hands off for ~1.5 s (center) -> circle both
  // sticks to their limits -> finishCalibration() saves. Returns false (and keeps the
  // old calibration) if an axis didn't travel far enough.
  void startCalibration();
  bool finishCalibration();
  CalState calState() const { return cal; }

private:
  struct AxisCal { int16_t min, center, max; };

  void sample();
  bool loadCal();
  void saveCal();
  static float normalize(int16_t mv, const AxisCal &c);
  static void rotate(float &x, float &y, float degrees);
  static void shape(float x, float y, int16_t &outX, int16_t &outY);

  int16_t mv[4] = {};
  AxisCal axes[4];
  AxisCal pending[4];
  bool hasSavedCal = false;

  CalState cal = CalState::Idle;
  uint32_t calStartMs = 0;
  int32_t centerSum[4] = {};
  uint16_t centerCount = 0;
};
