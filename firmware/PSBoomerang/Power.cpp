// PSBoomerang by polymatt. Copyright (c) 2026 Unscripted Engineering, LLC.
// https://github.com/thepolymatt/psboomerang
// Licensed under the GNU GPL v3.0 or later. See LICENSE.md in the repository root.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Power.h"
#include <driver/gpio.h>
#include <esp_sleep.h>

static constexpr uint32_t WAKE_RELEASE_MS = 50;         // Home released this long during the wake hold = not a long press
static constexpr uint32_t SLEEP_RELEASE_WAIT_MS = 2000; // before sleeping, wait this long for Home to be released

void Power::begin()
{
  // sleep() held STICK_EN low through deep sleep. Set a known level before releasing the hold.
  pinMode(PIN_STICK_EN, OUTPUT);
  digitalWrite(PIN_STICK_EN, LOW);
  gpio_hold_dis((gpio_num_t)PIN_STICK_EN);
  gpio_deep_sleep_hold_dis();

  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0)
  {
    // Woken by Home: stay awake only for a long press, so a bump in a bag doesn't drain the battery.
    pinMode(PIN_HOME, INPUT_PULLUP);
    uint32_t startMs = millis(), releasedMs = 0;
    bool released = false;
    while (millis() - startMs < WAKE_HOLD_MS)
    {
      if (digitalRead(PIN_HOME) == LOW)
        released = false;
      else if (!released)
      {
        released = true;
        releasedMs = millis();
      }
      else if (millis() - releasedMs >= WAKE_RELEASE_MS)
        sleep();
      delay(5);
    }
  }

  digitalWrite(PIN_STICK_EN, HIGH); // sticks and LEDs on
  delay(10);                        // LDO, sensor and LED start-up
}

void Power::sleep()
{
  // LED data low before the LEDs lose power, so DIN doesn't feed an unpowered LED.
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  pinMode(PIN_STICK_EN, OUTPUT);
  digitalWrite(PIN_STICK_EN, LOW);
  // GPIO43 isn't an RTC pin, so hold it low through deep sleep rather than rely on R9 alone.
  gpio_hold_en((gpio_num_t)PIN_STICK_EN);
  gpio_deep_sleep_hold_en();

  // ext0 wakes on a level, so give Home a moment to be released. If it stays pressed (squashed in
  // a bag), wake on its release instead: begin() then sees no long press and sleeps again.
  pinMode(PIN_HOME, INPUT_PULLUP);
  uint32_t startMs = millis();
  while (digitalRead(PIN_HOME) == LOW && millis() - startMs < SLEEP_RELEASE_WAIT_MS)
    delay(10);
  delay(50); // let the contact settle

  bool stuck = digitalRead(PIN_HOME) == LOW;
  esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_HOME, stuck ? 1 : 0); // R5 holds Home high while asleep
  esp_deep_sleep_start();
}
