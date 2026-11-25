// PSBoomerang by polymatt. Copyright (c) 2026 Unscripted Engineering, LLC.
// https://github.com/thepolymatt/psboomerang
// Licensed under the GNU GPL v3.0 or later. See LICENSE.md in the repository root.
// SPDX-License-Identifier: GPL-3.0-or-later

// The controller state sent to the host, shared by the Bluetooth and USB paths.
#pragma once
#include <stdint.h>

// Everything that goes into a report, so we only send when something changed.
// buttons bits 0-9 = BUTTON_1..BUTTON_10 in SInput order (South, East, West, North, L1, R1, L3, R3,
// L2, R2), plus the special buttons below. hat is a HAT_* value from BleGamepadConfiguration.h.
struct ReportState
{
  uint16_t buttons;
  uint8_t hat;
  int16_t lx, ly, rx, ry;
  bool operator!=(const ReportState &o) const
  {
    return buttons != o.buttons || hat != o.hat || lx != o.lx || ly != o.ly || rx != o.rx || ry != o.ry;
  }
};
static constexpr uint16_t SPECIAL_START = 1u << 13, SPECIAL_SELECT = 1u << 14, SPECIAL_HOME = 1u << 15;

// Where reports go. USB wins whenever a host has enumerated the port; a charger alone doesn't count.
enum class Link : uint8_t
{
  None,
  Ble,
  Usb
};
