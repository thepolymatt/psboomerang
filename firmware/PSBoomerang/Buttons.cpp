// PSBoomerang by polymatt. Copyright (c) 2026 Unscripted Engineering, LLC.
// https://github.com/thepolymatt/psboomerang
// Licensed under the GNU GPL v3.0 or later. See LICENSE.md in the repository root.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Buttons.h"
#include <Wire.h>

// PCA9555/TCA9555 registers (identical maps; each pair is port 0 then port 1; the pointer auto-toggles within a pair)
static constexpr uint8_t REG_INPUT0    = 0x00;
static constexpr uint8_t REG_POLARITY0 = 0x04;
static constexpr uint8_t REG_POLARITY1 = 0x05;
static constexpr uint8_t REG_CONFIG0   = 0x06;
static constexpr uint8_t REG_CONFIG1   = 0x07;

bool Buttons::begin()
{
  pinMode(PIN_HOME, INPUT_PULLUP); // external 10k pull-up too, so it also works as a sleep-wake pin
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_CLOCK_HZ);

  // All pins are inputs (power-on default, set anyway). The internal 100k pull-ups hold
  // them high and buttons short to GND, so invert polarity to make pressed read as 1.
  expanderOk = writeReg(REG_CONFIG0, 0xFF) && writeReg(REG_CONFIG1, 0xFF) &&
               writeReg(REG_POLARITY0, 0xFF) && writeReg(REG_POLARITY1, 0xFF) &&
               readExpander(expanderBits);
  return expanderOk;
}

void Buttons::update()
{
  // On an I2C error keep the last good expander bits; Home still works either way.
  if (expanderOk)
    readExpander(expanderBits);
  uint32_t sample = expanderBits;
  if (digitalRead(PIN_HOME) == LOW)
    sample |= 1ul << BTN_HOME;

  // Report an edge as soon as it's seen, then ignore that button for the debounce window.
  uint32_t now = millis();
  uint32_t changed = sample ^ state;
  for (uint8_t i = 0; changed; i++, changed >>= 1)
  {
    if ((changed & 1) && now - lastEdgeMs[i] >= BUTTON_DEBOUNCE_MS)
    {
      state ^= (1ul << i);
      lastEdgeMs[i] = now;
    }
  }
}

bool Buttons::readExpander(uint16_t &out)
{
  Wire.beginTransmission(EXPANDER_I2C_ADDR);
  Wire.write(REG_INPUT0);
  if (Wire.endTransmission(false) != 0)
    return false;
  if (Wire.requestFrom(EXPANDER_I2C_ADDR, (uint8_t)2) != 2)
    return false;
  uint8_t port0 = Wire.read();
  uint8_t port1 = Wire.read();
  out = (uint16_t)port1 << 8 | port0;
  return true;
}

bool Buttons::writeReg(uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(EXPANDER_I2C_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}
