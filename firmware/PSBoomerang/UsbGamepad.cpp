// PSBoomerang by polymatt. Copyright (c) 2026 Unscripted Engineering, LLC.
// https://github.com/thepolymatt/psboomerang
// Licensed under the GNU GPL v3.0 or later. See LICENSE.md in the repository root.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "UsbGamepad.h"
#include "Config.h"
#include <esp32-hal-tinyusb.h>
#include <BleGamepadConfiguration.h> // BUTTON_*, HAT_*, SINPUT_USB_VID/PID
#include <BleSInput.h>               // SInput report layout

static constexpr uint32_t SEND_TIMEOUT_MS = 20;

// The 139-byte SInput report descriptor from HandHeldLegend/SINPUT-LIB-HID, as the BLE library uses.
static const uint8_t kDescriptor[] = {
    0x05, 0x01, 0x09, 0x05, 0xA1, 0x01, 0x85, 0x01, 0x06, 0x00, 0xFF, 0x09, 0x01, 0x15, 0x00, 0x25,
    0xFF, 0x75, 0x08, 0x95, 0x02, 0x81, 0x02, 0x05, 0x09, 0x19, 0x01, 0x29, 0x20, 0x15, 0x00, 0x25,
    0x01, 0x75, 0x01, 0x95, 0x20, 0x81, 0x02, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x32, 0x09,
    0x35, 0x09, 0x33, 0x09, 0x34, 0x16, 0x00, 0x80, 0x26, 0xFF, 0x7F, 0x75, 0x10, 0x95, 0x06, 0x81,
    0x02, 0x06, 0x00, 0xFF, 0x09, 0x20, 0x15, 0x00, 0x26, 0xFF, 0xFF, 0x75, 0x20, 0x95, 0x01, 0x81,
    0x02, 0x09, 0x21, 0x16, 0x00, 0x80, 0x26, 0xFF, 0x7F, 0x75, 0x10, 0x95, 0x06, 0x81, 0x02, 0x09,
    0x22, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x1D, 0x81, 0x02, 0x85, 0x02, 0x09, 0x23,
    0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x3F, 0x81, 0x02, 0x85, 0x03, 0x09, 0x24, 0x15,
    0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x2F, 0x91, 0x02, 0xC0};
static_assert(sizeof(kDescriptor) == 139, "SInput descriptor length");

static void put16(uint8_t *p, int16_t v)
{
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)((uint16_t)v >> 8);
}

UsbGamepad::UsbGamepad()
{
  // SDL and Steam recognize SInput by this VID/PID, as over Bluetooth. With USB CDC On Boot the core
  // starts USB before setup(), so these have to be set during static construction.
  USB.VID(SINPUT_USB_VID);
  USB.PID(SINPUT_USB_PID_GENERIC);
  USB.productName(DEVICE_NAME);
  USB.manufacturerName(DEVICE_MANUFACTURER);
  hid.addDevice(this, sizeof(kDescriptor));
}

void UsbGamepad::begin(uint8_t gamepadType, uint8_t faceStyle)
{
  type = gamepadType;
  style = faceStyle;
  hid.begin();
}

bool UsbGamepad::connected() const
{
  return tud_mounted() && !tud_suspended();
}

uint16_t UsbGamepad::_onGetDescriptor(uint8_t *buffer)
{
  memcpy(buffer, kDescriptor, sizeof(kDescriptor));
  return sizeof(kDescriptor);
}

// Output reports arrive here from the interrupt OUT endpoint, and as SET_REPORT on the control pipe.
void UsbGamepad::_onOutput(uint8_t reportId, const uint8_t *buffer, uint16_t len)
{
  onCommand(reportId, buffer, len);
}

void UsbGamepad::_onSetFeature(uint8_t reportId, const uint8_t *buffer, uint16_t len)
{
  onCommand(reportId, buffer, len);
}

void UsbGamepad::onCommand(uint8_t reportId, const uint8_t *data, uint16_t len)
{
  if (reportId != SINPUT_REPORT_ID_OUTPUT_CMDDAT || len < 1)
    return;
  switch (data[0])
  {
  case SINPUT_COMMAND_FEATURES:
    featuresRequested = true; // answered from update(): sending here would block the TinyUSB task
    break;
  case SINPUT_COMMAND_JOYSTICKRGB:
    if (len >= 4)
      rgb = 0x01000000u | (uint32_t)data[1] << 16 | (uint32_t)data[2] << 8 | data[3];
    break;
  default: // no rumble motors; player LED number isn't shown
    break;
  }
}

void UsbGamepad::update()
{
  if (featuresRequested.exchange(false))
    sendFeatures();
}

bool UsbGamepad::takeRgb(uint8_t &r6, uint8_t &g6, uint8_t &b6)
{
  uint32_t v = rgb.exchange(0);
  if (!v)
    return false;
  r6 = v >> 16;
  g6 = v >> 8;
  b6 = v;
  return true;
}

// Matches what the BLE library reports for the configuration in setup(): both sticks, digital
// triggers, 10 buttons plus D-pad, Start, Select and Home, host RGB, no rumble or IMU.
void UsbGamepad::sendFeatures()
{
  uint8_t r[SINPUT_REPORT_LEN_INPUT] = {};
  r[0] = SINPUT_COMMAND_FEATURES;
  r[SINPUT_FEAT_IDX_PROTOCOL_VERSION] = 1;
  r[SINPUT_FEAT_IDX_CAPS0] = SINPUT_FEAT_CAP_PLAYERLED | SINPUT_FEAT_CAP_LEFT_STICK | SINPUT_FEAT_CAP_RIGHT_STICK;
  r[SINPUT_FEAT_IDX_CAPS1] = SINPUT_FEAT_CAP_JOYSTICKRGB;
  r[SINPUT_FEAT_IDX_TYPE] = type;
  r[SINPUT_FEAT_IDX_STYLE] = style;
  r[SINPUT_FEAT_IDX_USAGE_MASK_0] = SINPUT_BTN0_SOUTH | SINPUT_BTN0_EAST | SINPUT_BTN0_WEST | SINPUT_BTN0_NORTH |
                                    SINPUT_BTN0_DUP | SINPUT_BTN0_DDOWN | SINPUT_BTN0_DLEFT | SINPUT_BTN0_DRIGHT;
  r[SINPUT_FEAT_IDX_USAGE_MASK_1] = SINPUT_BTN1_LSTICK | SINPUT_BTN1_RSTICK | SINPUT_BTN1_LSHOULDER |
                                    SINPUT_BTN1_RSHOULDER | SINPUT_BTN1_LTRIGGER | SINPUT_BTN1_RTRIGGER;
  r[SINPUT_FEAT_IDX_USAGE_MASK_2] = SINPUT_BTN2_START | SINPUT_BTN2_BACK | SINPUT_BTN2_GUIDE;
  bool ok = hid.SendReport(SINPUT_REPORT_ID_INPUT_CMDDAT, r, sizeof(r), SEND_TIMEOUT_MS);
  Serial.println(ok ? "USB: sent SInput features to host" : "USB: failed to send SInput features");
}

bool UsbGamepad::send(const ReportState &s, bool usbPowered, uint8_t batteryPercent)
{
  auto on = [&](uint8_t button) { return (s.buttons >> (button - 1)) & 1; };
  uint8_t h = s.hat;
  bool up = h == HAT_UP || h == HAT_UP_RIGHT || h == HAT_UP_LEFT;
  bool down = h == HAT_DOWN || h == HAT_DOWN_RIGHT || h == HAT_DOWN_LEFT;
  bool left = h == HAT_LEFT || h == HAT_UP_LEFT || h == HAT_DOWN_LEFT;
  bool right = h == HAT_RIGHT || h == HAT_UP_RIGHT || h == HAT_DOWN_RIGHT;

  uint8_t m[SINPUT_REPORT_LEN_INPUT] = {};
  m[SINPUT_IN_IDX_PLUG_STATUS] = usbPowered ? SINPUT_PLUG_STATUS_CHARGING : SINPUT_PLUG_STATUS_ON_BATTERY;
  m[SINPUT_IN_IDX_CHARGE_LEVEL] = batteryPercent;
  m[SINPUT_IN_IDX_BUTTONS_0] = (on(BUTTON_1) ? SINPUT_BTN0_SOUTH : 0) | (on(BUTTON_2) ? SINPUT_BTN0_EAST : 0) |
                               (on(BUTTON_3) ? SINPUT_BTN0_WEST : 0) | (on(BUTTON_4) ? SINPUT_BTN0_NORTH : 0) |
                               (up ? SINPUT_BTN0_DUP : 0) | (down ? SINPUT_BTN0_DDOWN : 0) |
                               (left ? SINPUT_BTN0_DLEFT : 0) | (right ? SINPUT_BTN0_DRIGHT : 0);
  m[SINPUT_IN_IDX_BUTTONS_1] = (on(BUTTON_7) ? SINPUT_BTN1_LSTICK : 0) | (on(BUTTON_8) ? SINPUT_BTN1_RSTICK : 0) |
                               (on(BUTTON_5) ? SINPUT_BTN1_LSHOULDER : 0) | (on(BUTTON_6) ? SINPUT_BTN1_RSHOULDER : 0) |
                               (on(BUTTON_9) ? SINPUT_BTN1_LTRIGGER : 0) | (on(BUTTON_10) ? SINPUT_BTN1_RTRIGGER : 0);
  m[SINPUT_IN_IDX_BUTTONS_2] = (s.buttons & SPECIAL_START ? SINPUT_BTN2_START : 0) |
                               (s.buttons & SPECIAL_SELECT ? SINPUT_BTN2_BACK : 0) |
                               (s.buttons & SPECIAL_HOME ? SINPUT_BTN2_GUIDE : 0);
  put16(&m[SINPUT_IN_IDX_LEFT_X], s.lx);
  put16(&m[SINPUT_IN_IDX_LEFT_Y], s.ly);
  put16(&m[SINPUT_IN_IDX_RIGHT_X], s.rx);
  put16(&m[SINPUT_IN_IDX_RIGHT_Y], s.ry);
  return hid.SendReport(SINPUT_REPORT_ID_INPUT, m, sizeof(m), SEND_TIMEOUT_MS);
}
