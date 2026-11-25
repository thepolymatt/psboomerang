/*
 * PSBoomerang
 * Firmware for the PSBoomerang game controller: a Seeed XIAO ESP32-S3 that reports as an SInput
 * gamepad over Bluetooth LE or USB (SDL3 / Steam).
 *
 * Developed by polymatt. Copyright (c) 2026 Unscripted Engineering, LLC.
 * https://github.com/thepolymatt/psboomerang
 *
 * Licensed under the GNU General Public License v3.0 or later: free to use, modify, share and
 * sell, as long as anything you distribute based on it is released under the same license with
 * its source. Distributed without any warranty. See LICENSE.md in the repository root.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <BleGamepad.h>
#include <NimBLEDevice.h>
#include "Config.h"
#include "Report.h"
#include "UsbGamepad.h"
#include "Buttons.h"
#include "Sticks.h"
#include "Battery.h"
#include "StatusLed.h"
#include "Power.h"

BleGamepad gamepad(DEVICE_NAME, DEVICE_MANUFACTURER, 100);
BleGamepadConfiguration bleConfig;
UsbGamepad usb;
Power power;
Buttons buttons;
Sticks sticks;
Battery battery;
StatusLed led;

bool expanderOk = false;
bool calArmed = false;

static uint8_t dpadToHat(bool up, bool down, bool left, bool right)
{
  if (up && down) up = down = false;       // opposite directions cancel
  if (left && right) left = right = false;
  if (up) return right ? HAT_UP_RIGHT : left ? HAT_UP_LEFT : HAT_UP;
  if (down) return right ? HAT_DOWN_RIGHT : left ? HAT_DOWN_LEFT : HAT_DOWN;
  if (right) return HAT_RIGHT;
  if (left) return HAT_LEFT;
  return HAT_CENTERED;
}

// Calibration is entered by holding Select + L3 + R3 and starts when they're released, so the
// center reading isn't taken with a thumb still pressing a stick. Start saves it.
static void handleCalibration()
{
  static uint32_t chordSinceMs = 0;
  static bool chordHeld = false;
  static bool startWasPressed = false;

  bool chord = buttons.pressed(BTN_SELECT) && buttons.pressed(BTN_L3) && buttons.pressed(BTN_R3);
  bool start = buttons.pressed(BTN_START);
  bool startEdge = start && !startWasPressed;
  startWasPressed = start;

  if (sticks.calState() == CalState::Idle)
  {
    if (chord && !chordHeld)
      chordSinceMs = millis();
    chordHeld = chord;

    if (chord && millis() - chordSinceMs >= CALIBRATION_HOLD_MS)
      calArmed = true;
    else if (!chord && calArmed)
    {
      calArmed = false;
      sticks.startCalibration();
      Serial.println("Calibration: hands off the sticks...");
    }
  }
  else if (sticks.calState() == CalState::Range && startEdge)
  {
    bool ok = sticks.finishCalibration();
    led.flash(ok ? 0x00FF00 : 0xFF0000, 1000);
    Serial.println(ok ? "Calibration saved" : "Calibration rejected: move each stick fully in every direction");
  }
}

// Hold Home to sleep, or sleep after IDLE_SLEEP_MS without input while on battery. The press that
// woke the controller doesn't count: Home has to be released once first.
static void handleSleep()
{
  static bool homeArmed = false;
  static bool homeWasPressed = false;
  static uint32_t homeSinceMs = 0;
  static uint32_t lastButtons = 0;
  static uint32_t lastActivityMs = 0;

  uint32_t now = millis();
  bool home = buttons.pressed(BTN_HOME);
  if (!home)
    homeArmed = true;
  else if (!homeWasPressed)
    homeSinceMs = now;
  homeWasPressed = home;

  bool sticksMoved = sticks.lx || sticks.ly || sticks.rx || sticks.ry; // zero inside the deadzone
  if (buttons.raw() != lastButtons || sticksMoved || battery.usbPowered())
    lastActivityMs = now;
  lastButtons = buttons.raw();

  if ((home && homeArmed && now - homeSinceMs >= SLEEP_HOLD_MS) || now - lastActivityMs >= IDLE_SLEEP_MS)
  {
    Serial.println("Sleeping: hold Home to wake");
    Serial.flush();
    power.sleep();
  }
}

static ReportState readInputs()
{
  ReportState s = {};
  // No input goes to the host while calibrating or arming it.
  if (calArmed || sticks.calState() != CalState::Idle)
    return s;

  auto set = [&](bool on, uint8_t gamepadButton) {
    if (on) s.buttons |= 1u << (gamepadButton - 1);
  };
  // SInput mapping (see ESP32-BLE-Gamepad docs/SInputMode.md)
  set(buttons.pressed(BTN_SOUTH), SWAP_FACE_BUTTONS ? BUTTON_2 : BUTTON_1);
  set(buttons.pressed(BTN_EAST), SWAP_FACE_BUTTONS ? BUTTON_1 : BUTTON_2);
  set(buttons.pressed(BTN_WEST), SWAP_FACE_BUTTONS ? BUTTON_4 : BUTTON_3);
  set(buttons.pressed(BTN_NORTH), SWAP_FACE_BUTTONS ? BUTTON_3 : BUTTON_4);
  set(buttons.pressed(BTN_L1), BUTTON_5);
  set(buttons.pressed(BTN_R1), BUTTON_6);
  set(buttons.pressed(BTN_L3), BUTTON_7);
  set(buttons.pressed(BTN_R3), BUTTON_8);
  set(buttons.pressed(BTN_L2), BUTTON_9);   // digital trigger
  set(buttons.pressed(BTN_R2), BUTTON_10);  // digital trigger

  if (buttons.pressed(BTN_START)) s.buttons |= SPECIAL_START;
  if (buttons.pressed(BTN_SELECT)) s.buttons |= SPECIAL_SELECT;
  if (buttons.pressed(BTN_HOME)) s.buttons |= SPECIAL_HOME;

  s.hat = dpadToHat(buttons.pressed(BTN_DPAD_UP), buttons.pressed(BTN_DPAD_DOWN),
                    buttons.pressed(BTN_DPAD_LEFT), buttons.pressed(BTN_DPAD_RIGHT));
  s.lx = sticks.lx;
  s.ly = sticks.ly;
  s.rx = sticks.rx;
  s.ry = sticks.ry;
  return s;
}

static void sendReport(const ReportState &s)
{
  for (uint8_t b = BUTTON_1; b <= BUTTON_10; b++)
  {
    if (s.buttons & (1u << (b - 1))) gamepad.press(b);
    else gamepad.release(b);
  }
  (s.buttons & SPECIAL_START) ? gamepad.pressStart() : gamepad.releaseStart();
  (s.buttons & SPECIAL_SELECT) ? gamepad.pressSelect() : gamepad.releaseSelect();
  (s.buttons & SPECIAL_HOME) ? gamepad.pressHome() : gamepad.releaseHome();
  gamepad.setHat1(s.hat);
  gamepad.setLeftThumb(s.lx, s.ly);
  gamepad.setRightThumb(s.rx, s.ry);
  gamepad.sendReport();
}

static void updateBatteryReport()
{
  gamepad.setBatteryPowerInformation(POWER_STATE_PRESENT);
  gamepad.setChargingState(battery.usbPowered() ? POWER_STATE_CHARGING : POWER_STATE_NOT_CHARGING);
  gamepad.setDischargingState(battery.usbPowered() ? POWER_STATE_NOT_DISCHARGING : POWER_STATE_DISCHARGING);
  gamepad.setBatteryLevel(battery.percent());
}

// Some hosts, macOS among them, only pair when the controller asks: nothing in the SInput service
// requires encryption, so they never start it themselves. Ask on any link that isn't encrypted yet.
// The library counts a link as connected only once it's paired.
static void requestPairing()
{
  static uint32_t lastMs = 0;
  if (millis() - lastMs < 1000)
    return;
  lastMs = millis();
  NimBLEServer *server = NimBLEDevice::getServer();
  if (!server)
    return;
  for (uint16_t handle : server->getPeerDevices())
    if (!server->getPeerInfoByHandle(handle).isEncrypted())
      NimBLEDevice::startSecurity(handle);
}

static void debugPrint(Link link)
{
  static uint32_t lastMs = 0;
  if (millis() - lastMs < 100)
    return;
  lastMs = millis();
  static const char *const linkNames[] = {"none", "ble", "usb"};
  const int16_t *mv = sticks.rawMv();
  Serial.printf("mV L(%4d,%4d) R(%4d,%4d)  out L(%6d,%6d) R(%6d,%6d)  btn %05lX  bat %.2fV %u%% usb=%d link=%s\n",
                mv[0], mv[1], mv[2], mv[3], sticks.lx, sticks.ly, sticks.rx, sticks.ry,
                (unsigned long)buttons.raw(), battery.volts(), battery.percent(), battery.usbPowered(),
                linkNames[(uint8_t)link]);
}

void setup()
{
  power.begin(); // first: may go straight back to sleep, and powers the sticks and LEDs
  Serial.begin(115200);
  led.begin();

  expanderOk = buttons.begin();
  if (!expanderOk)
    Serial.println("PCA9555 not found at 0x20: check SDA/SCL, pull-ups and address pins");
  sticks.begin();
  if (!sticks.calibrated())
    Serial.println("Sticks not calibrated yet: hold Select + L3 + R3 for 3 s");
  battery.begin();

  bleConfig.setGamepadMode(GamepadMode::SInput); // must come first; it resets the options below
  bleConfig.setAutoReport(false);
  bleConfig.setButtonCount(10);        // face x4, L1/R1, L3/R3, L2/R2
  bleConfig.setIncludeRxAxis(false);   // no analog triggers: SDL then treats L2/R2 as digital triggers
  bleConfig.setIncludeRyAxis(false);
  bleConfig.setEnableRumble(false);
  bleConfig.setEnableTouchpad(false);
  bleConfig.setEnableSInputRGB(true);  // lets the host color LED 1
  gamepad.begin(&bleConfig);
  usb.begin(bleConfig.getSInputGamepadType(), bleConfig.getSInputFaceStyle());
}

void loop()
{
  static ReportState lastSent = {};
  static uint32_t lastSendUs = 0;
  static Link lastLink = Link::None;
  static bool forceSend = false; // battery/power state changed or just connected

  buttons.update();
  sticks.update();
  battery.update();
  usb.update();
  handleCalibration();
  handleSleep();
  requestPairing();

  // A computer on the USB port takes over from Bluetooth. Bluetooth stays connected but sends
  // nothing, and picks up again when the cable is unplugged.
  Link link = usb.connected() ? Link::Usb : gamepad.isConnected() ? Link::Ble : Link::None;
  if (link != Link::None)
  {
    if (battery.consumeChange() || link != lastLink)
    {
      if (link == Link::Ble)
        updateBatteryReport(); // USB reports carry the battery state in every report
      forceSend = true;
    }
    uint8_t r, g, b;
    if (link == Link::Usb && usb.takeRgb(r, g, b))
      led.setHostColor(r, g, b);
    else if (link == Link::Ble && gamepad.isRgbReceived())
      led.setHostColor(gamepad.getRgbRed(), gamepad.getRgbGreen(), gamepad.getRgbBlue());

    ReportState now = readInputs();
    if ((forceSend || now != lastSent) && micros() - lastSendUs >= REPORT_MIN_INTERVAL_US)
    {
      bool sent = true;
      if (link == Link::Usb)
        sent = usb.send(now, battery.usbPowered(), battery.percent());
      else
        sendReport(now);
      lastSendUs = micros();
      if (sent) // a failed USB send is retried on the next interval
      {
        lastSent = now;
        forceSend = false;
      }
    }
  }
  lastLink = link;

  led.update({!expanderOk, calArmed, sticks.calState(), link != Link::None, battery.usbPowered(),
              battery.percent(), battery.volts() < LED_MIN_VBAT});

  if (DEBUG_INPUT)
    debugPrint(link);
}

//  ۜ\(סּںסּَ` )/ۜ
