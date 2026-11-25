// PSBoomerang by polymatt. Copyright (c) 2026 Unscripted Engineering, LLC.
// https://github.com/thepolymatt/psboomerang
// Licensed under the GNU GPL v3.0 or later. See LICENSE.md in the repository root.
// SPDX-License-Identifier: GPL-3.0-or-later

// Wired mode: the same SInput gamepad as over Bluetooth, as a USB HID interface next to the serial
// port. Needs Tools > USB Mode > USB-OTG (TinyUSB) (checked in Config.h).
#pragma once
#include <Arduino.h>
#include <USB.h>
#include <USBHID.h>
#include <atomic>
#include "Report.h"

class UsbGamepad : public USBHIDDevice
{
public:
  UsbGamepad(); // must be a global: USB starts before setup(), and the HID interface has to exist by then
  void begin(uint8_t gamepadType, uint8_t faceStyle); // SInput features, same values as the BLE config
  void update();                                      // call every loop: answers host requests
  bool connected() const;                             // enumerated by a host and not suspended
  bool send(const ReportState &s, bool usbPowered, uint8_t batteryPercent);
  bool takeRgb(uint8_t &r6, uint8_t &g6, uint8_t &b6); // true once per color the host sets

  // USBHIDDevice. The two set callbacks run on the TinyUSB task, so they only set flags.
  uint16_t _onGetDescriptor(uint8_t *buffer) override;
  void _onOutput(uint8_t reportId, const uint8_t *buffer, uint16_t len) override;
  void _onSetFeature(uint8_t reportId, const uint8_t *buffer, uint16_t len) override;

private:
  void onCommand(uint8_t reportId, const uint8_t *data, uint16_t len);
  void sendFeatures();

  USBHID hid;
  uint8_t type = 1, style = 1;
  std::atomic<bool> featuresRequested{false};
  std::atomic<uint32_t> rgb{0}; // 0x01RRGGBB while a new color is pending
};
