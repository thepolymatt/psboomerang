// PSBoomerang by polymatt. Copyright (c) 2026 Unscripted Engineering, LLC.
// https://github.com/thepolymatt/psboomerang
// Licensed under the GNU GPL v3.0 or later. See LICENSE.md in the repository root.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "StatusLed.h"

static constexpr uint32_t RED     = 0xFF0000;
static constexpr uint32_t AMBER   = 0xFF8000;
static constexpr uint32_t GREEN   = 0x00FF00;
static constexpr uint32_t BLUE    = 0x0000FF;
static constexpr uint32_t WHITE   = 0xFFFFFF;
static constexpr uint32_t MAGENTA = 0xFF00FF;

static bool blink(uint32_t periodMs)
{
  return (millis() % periodMs) < periodMs / 2;
}

// Triangle wave between 15% and 100%, so "breathing" never fully goes dark.
static uint8_t breathe(uint32_t periodMs)
{
  uint32_t t = millis() % periodMs;
  uint32_t half = periodMs / 2;
  uint32_t ramp = t < half ? t : periodMs - t;
  return 38 + ramp * (255 - 38) / half;
}

static uint32_t scale(uint32_t color, uint8_t level)
{
  uint8_t r = ((color >> 16) & 0xFF) * level / 255;
  uint8_t g = ((color >> 8) & 0xFF) * level / 255;
  uint8_t b = (color & 0xFF) * level / 255;
  return (uint32_t)r << 16 | (uint32_t)g << 8 | b;
}

void StatusLed::begin()
{
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  strip.clear();
  strip.show();
}

void StatusLed::setHostColor(uint8_t r6, uint8_t g6, uint8_t b6)
{
  auto to8 = [](uint8_t v) { return (uint8_t)min(255, (v * 255 + 31) / 63); };
  hostColor = Adafruit_NeoPixel::Color(to8(r6), to8(g6), to8(b6));
  hasHostColor = hostColor != 0;
}

void StatusLed::flash(uint32_t color, uint32_t ms)
{
  flashColor = color;
  flashUntilMs = millis() + ms;
}

void StatusLed::update(const LedInputs &in)
{
  if (millis() - lastFrameMs < 20)
    return;
  lastFrameMs = millis();

  uint32_t status;
  if ((int32_t)(flashUntilMs - millis()) > 0)
    status = blink(200) ? flashColor : 0;
  else if (in.fault)
    status = blink(200) ? RED : 0;
  else if (in.calArmed)
    status = WHITE;
  else if (in.cal == CalState::Center)
    status = blink(150) ? WHITE : 0;          // hands off the sticks
  else if (in.cal == CalState::Range)
    status = MAGENTA;                          // circle both sticks, then press Start
  else if (!in.connected)
    status = scale(BLUE, breathe(2000));       // advertising / waiting to pair
  else
  {
    uint32_t level = in.battery > 50 ? GREEN : in.battery > 20 ? AMBER : RED;
    if (in.usbPowered)
      status = scale(level, breathe(3000));    // charging
    else if (in.battery <= 10)
      status = blink(1000) ? level : 0;
    else
      status = level;
  }

  uint32_t colors[2] = {status, hasHostColor && in.connected ? hostColor : status};
  if (in.supplyLow)
    colors[0] = colors[1] = 0;
  if (colors[0] == shown[0] && colors[1] == shown[1])
    return;
  for (uint8_t i = 0; i < LED_COUNT && i < 2; i++)
    strip.setPixelColor(i, colors[i]);
  strip.show();
  shown[0] = colors[0];
  shown[1] = colors[1];
}
