# PSBoomerang by polymatt

I always loved the design of the old “Boomerang” concept controller that Sony unveiled alongside the PS3 in 2005. So I built one.

![PSBoomerang](./docs/images/hero_render.png)

You can check out the build video here: [It’s Back](https://youtu.be/1w722lkhs_E)

- Wireless over Bluetooth LE, or wired over USB-C (if you want full-fat Bluetooth, you’ll need a different microcontroller)
- Shows up as an SInput gamepad, supported by Steam and SDL3 games
- Joy-Con replacement TMR analog sticks with clickable L3/R3
- D-pad, face and center buttons using DualShock 3 replacement rubber pads on gold PCB contacts
- Built-in LiPo battery charged over USB-C; deep sleep with the Home button as soft-power switch
- Two RGB status LEDs, one of which the host can color
- Hand-solderable: no hot air or reflow needed

It won't pair with a PS3 or modern console wirelessly. The XIAO ESP32-S3 only supports Bluetooth LE, not the full-fat Bluetooth Classic the PS3 uses, plus modern consoles need authentication/negotiation.

As it stands, enjoy this one on PC, Steam, OR connected via USB on the PS3.

## What's in the repo

| Folder | Contents |
|---|---|
| `hardware/enclosure/` | Printable STLs (`stl/`) and modifiable STEPs (`step/`) |
| `hardware/pcb/controller/` | Main board KiCad project, plus gerbers, BOM, CPL and STEP in `fabrication/v0.4/` |
| `hardware/pcb/shoulder/` | Shoulder button board KiCad project, plus gerbers, BOM and CPL in `fabrication/v0.3/` |
| `hardware/pcb/lib/` | All custom and third-party symbols, footprints and 3D models the boards use |
| `firmware/PSBoomerang/` | Arduino firmware for the Seeed XIAO ESP32-S3 |
| `bom/BOM.csv` | Full parts list with part numbers and sourcing notes |
| `docs/PCB-Notes.md` | Pin map, circuit notes, assembly and soldering guide |

The KiCad projects open in KiCad 10 with no extra library setup.

You can also find the model files here: 

## Building one

1. **Order the PCBs.** Send the gerber zips from the two `fabrication/` folders to any PCB fab. Order the main board with **ENIG** finish, and two shoulder boards.
2. **Get the parts** in `bom/BOM.csv`, including a Seeed XIAO ESP32-S3 (plain, not Sense), two Joy-Con replacement sticks, DualShock 3 replacement rubber pads and a 1S LiPo with a JST-PH lead.
3. **Assemble** using `docs/PCB-Notes.md`. Check the battery connector polarity before plugging it in; it isn't standard between vendors.
4. **Print the enclosure** from `hardware/enclosure/stl/`.
5. **Flash the firmware** (below).
6. Have fun.

## Firmware setup

1. Install the [Arduino IDE](https://www.arduino.cc/en/software) and the **esp32** board package by Espressif (Boards Manager).
2. Install these libraries:
   - [ESP32-BLE-Gamepad](https://github.com/lemmingDev/ESP32-BLE-Gamepad) by lemmingDev, a version with SInput support (install from GitHub if the Library Manager version doesn't include `BleSInput.h`)
   - NimBLE-Arduino
   - Adafruit NeoPixel
3. Open `firmware/PSBoomerang/PSBoomerang.ino`, select the **XIAO_ESP32S3** board and set:

   | Tools setting | Value |
   |---|---|
   | USB CDC On Boot | Enabled |
   | USB Mode | USB-OTG (TinyUSB) |
   | Upload Mode | USB-OTG CDC (TinyUSB) |

4. Upload. If no port shows up, hold the XIAO's BOOT button while plugging it in.

Options such as stick orientation and face-button order are at the top of `Config.h`.

## Using it

| Action | How |
|---|---|
| Wake | Hold Home for 1 s |
| Sleep | Hold Home for 3 s. It also sleeps after 10 minutes without input on battery. |
| Pair | The status LED breathes blue while it's waiting. Pair with **PSBoomerang** in your Bluetooth settings. |
| Wired | Plug USB-C into a computer. USB takes over from Bluetooth while connected. |
| Calibrate sticks | Hold Select + L3 + R3 for 3 s, then release. Keep hands off while the LED blinks white. When it turns magenta, circle both sticks to their limits, then press Start. Green means saved, red means try again. |

Calibrate the sticks once after the first flash - if needed.

**Status LED:** green, amber or red shows the battery level (above 50%, above 20%, 20% and below). It breathes while charging. Blinking red means the button expander isn't responding.

## License

The PSBoomerang was designed by me, polymatt, and Copyright (c) 2026 Unscripted Engineering, LLC.

The hardware is licensed under CERN-OHL-S-2.0 and the firmware under GPL-3.0-or-later. You're free to build, modify and sell it, as long as anything you share stays under the same license. See [LICENSE.md](LICENSE.md).

PSBoomerang is an independent project and isn't affiliated with or endorsed by Sony Interactive Entertainment. PlayStation and DualShock are trademarks of Sony Interactive Entertainment.
