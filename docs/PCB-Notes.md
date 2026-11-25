# PSBoomerang PCB: wiring notes

Parts list: `../bom/BOM.csv`. Firmware pin constants are in `../firmware/PSBoomerang/Config.h`.

## Ordering

- Main board: `hardware/pcb/controller/fabrication/v0.4/`. Shoulder board (order 2): `hardware/pcb/shoulder/fabrication/v0.3/`. Each folder has the gerber zip, plus a BOM and CPL if you want the fab to assemble it.
- **Order the main board with ENIG finish.** HASL is not super reliable under the carbon pills.

## XIAO ESP32-S3 pin map

| XIAO pin | GPIO | Net | Notes |
|---|---|---|---|
| D0 / A0 | 1 | STICK_LX | J1 X, C7 10 nF to GND at the pin |
| D1 / A1 | 2 | STICK_LY | J1 Y, C8 10 nF |
| D2 / A2 | 3 | STICK_RX | J2 X, C9 10 nF |
| D3 / A3 | 4 | STICK_RY | J2 Y, C10 10 nF |
| D4 | 5 | I2C_SDA | R3 4.7 k to 3V3 |
| D5 | 6 | I2C_SCL | R4 4.7 k to 3V3 |
| D6 | 43 | STICK_EN | U3 EN and U4 ON, R9 100 k to GND (sticks and LEDs turn off in deep sleep or reset) |
| D7 | 44 | VBUS_SENSE | 5V → R7 22 k → D7 → R8 33 k → GND (≈3.0 V when USB is present) |
| D8 / A8 | 7 | BAT_SENSE | +BATT → R1 220 k → A8 → R2 220 k → GND, C1 100 nF at the pin |
| D9 | 8 | LED_DIN | R6 330 Ω in series to LED1 DIN |
| D10 | 9 | BTN_HOME | Home/PS carbon contact SW11 to GND, R5 10 k to 3V3. RTC-capable, so it doubles as the wake-from-sleep button |
| 3V3 | | +3V3 | Powers U2, U3 input, pull-ups |
| 5V | | VBUS | Only for VBUS_SENSE; don't power anything from it |
| GND | | GND | |
| BAT+ / BAT− (underside pads) | | +BATT / GND | Plated 2.7 × 1.4 mm slots through the main board: solder from the top with an iron |

## Blocks

**Sticks (J1 left, J2 right).** Flex: 5-pin, 0.5 mm pitch, 0.3 mm thick. **J1/J2 pinout: 1 V+, 2 Y, 3 S, 4 G, 5 X**, with the flex contacts facing the board in the bottom-contact Molex 505110.
- J1/J2 are on the bottom (B.Cu), with the XIAO. Check that each stick's flex reaches its connector without kinking or stretching. More slack is good.
- The Molex 505110 is bottom-contact, so a flipped flex simply makes no contact.
- V+ → +3V0_STICK (U3 output), with C5/C6 100 nF at each connector (bottom side, right at the V+ and G pins).
- S (stick click, shorts to GND) → U2 P14 (left) / P15 (right).

**Stick regulator (U3).** IN = +3V3 with C3 1 µF, OUT = +3V0_STICK with C4 1 µF, EN = STICK_EN. The 3.0 V output keeps stick outputs under the S3 ADC's ~3.1 V ceiling.

**Buttons (U2 PCA9555, SOIC-24).** A0/A1/A2 → GND (address 0x20). VCC = 3V3 with C2 100 nF. Each contact goes between its Pxx pin and GND; the internal 100 k pull-ups do the rest, so there are no button resistors. INT is unused and left unconnected. Home/PS (SW11) goes directly to D10, as in the pin map.

| Port | Button | Port | Button |
|---|---|---|---|
| P00 | D-pad Up (SW1) | P10 | L1 (J4 pin 1, left shoulder board) |
| P01 | D-pad Down (SW2) | P11 | R1 (J5 pin 1, right shoulder board) |
| P02 | D-pad Left (SW3) | P12 | L2 (J4 pin 2) |
| P03 | D-pad Right (SW4) | P13 | R2 (J5 pin 2) |
| P04 | South / A (SW5) | P14 | L3, J1 S |
| P05 | East / B (SW6) | P15 | R3, J2 S |
| P06 | West / X (SW7) | P16 | Select (SW9) |
| P07 | North / Y (SW8) | P17 | Start (SW10) |

If you reroute, change the `ButtonBit` enum in `Config.h` to match. It’s all configurable.

**Carbon contacts (SW1–SW11).** Interdigitated gold combs under PS3 DualShock 3 rubber pads. I specifically oversized them a bit to account for some variability in assembly and where the carbon pads will actually hit. 
- D-pad and face: 5.5 mm round contacts (4.5 mm pills), `PSBoomerang:CarbonPad_DPad` / `CarbonPad_Face`.
- Select/Start: `CarbonPad_2Pill` (8.5 × 4 mm, two 3 mm pills on a 4.5 mm pitch).
- Home: `CarbonPad_3Pill` (13 × 4 mm, three pills).
- No paste and no silkscreen inside the contact circles.

**Button contact positions.** Origin is midway between the D-pad and face-cluster centers (the board's grid origin, 148.5, 105); +X right, +Y down.

| Ref | Button | X (mm) | Y (mm) |
|---|---|---|---|
| SW1 | D-pad Up | −45.0 | −9.0 |
| SW2 | D-pad Down | −45.0 | +9.0 |
| SW3 | D-pad Left | −54.0 | 0 |
| SW4 | D-pad Right | −36.0 | 0 |
| SW8 | Y / North (Triangle) | +45.0 | −8.5 |
| SW5 | A / South (Cross) | +45.0 | +8.5 |
| SW7 | X / West (Square) | +35.5 | 0 |
| SW6 | B / East (Circle) | +54.5 | 0 |
| SW9 | Select | −13.6 | 0 |
| SW10 | Start | +13.6 | 0 |
| SW11 | Home / PS | 0 | +8.5 |

**Shoulder (bumper) boards.** One design serves both sides; build two. A JST PH 3-pin cable joins each to J4 (left) or J5 (right) on the main board's bottom side.
- Pins: 1 upper (L1/R1), 2 lower (L2/R2), 3 GND. Both ends use the same JST S3B-PH-SM4-TB, so the cable must run pin 1 to pin 1. A reversed cable still reads L1/R1 but loses L2/R2.
- Cables: crimp PHR-3 housings with SPH-002T-P0.5S contacts, or use pre-crimped PH leads. Length is set by the enclosure.
- Board: 30 × 18.6 × 1.6 mm. Switches on the front, J1 on the back with its cable leaving toward the top edge. The back silk legend reads `3 GND  2 LOWER  1 UPPER`; seen from the back, pin 1 is on the right.
- No mounting holes: the enclosure locates and holds the board.
- Parts are SMD on both faces, so a single-sided hot-plate pass won't do the whole board: hand-solder at least one face.

**Battery.** J3 (JST PH vertical SMT, B2B-PH-SM4-TB, on the bottom; the plug goes in straight down) + → +BATT → XIAO BAT+. There's no power switch: the controller deep-sleeps, and a long press of Home wakes it. The battery charges whenever USB is in.
- Sleep draw is about 25 µA (roughly 4 years per 1000 mAh), so the LiPo's own self-discharge dominates.
- Unplug J3 for long storage, or to force a hung board off.
- **Check your battery's JST polarity:** it isn't standardized between vendors.

**LED load switch (U4, TPS22917DBVR).** VIN = +BATT with C13 1 µF, VOUT = LED_VDD, ON = STICK_EN, so the LEDs power up and down with the sticks. CT and QOD are left open.
- STICK_EN is also UART0 TX (GPIO43), so the boot log toggles U4 during boot. The firmware holds it high when running and low through deep sleep.

**Status LEDs (LED1 → LED2 chain).** D9 → R6 → LED1 DIN, LED1 DOUT → R10 330 Ω → LED2 DIN. C11/C12 100 nF on LED_VDD at each LED.
- LED VDD is LED_VDD (the battery through U4), not 3V3: the Würth WL-ICLED 1315050930002 is rated 3.5–5.5 V. Firmware turns the LEDs off below 3.5 V battery.
- Its pinout differs from the WS2812B: 1 VSS, 2 DIN, 3 VDD, 4 DOUT. **Pin 1 is the silk dot at the top-left**, matching the LED's chamfered corner.
- The footprint's pads extend 1.2 mm past the body so an iron can reach them.

**XIAO ESP32-S3.**
- The plain XIAO ESP32-S3 has **no onboard antenna**, only a U.FL connector for the antenna that comes in the box. It fits nicely tucked in beside the board and the side of the case.
- **BAT pads (U1 pads 15/16):** the XIAO's BAT+/BAT− face up into the main board, each under a plated slot. Check BAT+/BAT− against the XIAO's own silkscreen first, then:
  1. Solder just one or two castellated edges first in case you need to pull it up or move it.
  2. From the top, put a generous amount of flux into the holes from the top (pad side), heat the ring and feed solder until it wets the XIAO pad below.
  3. Check there's no bridge between the two slots: that shorts the battery.
  4. Reflow with more flux if needed, but too much solder can bridge underneath.

## Assembly

- All passives, U3 and U4 are on the bottom (B.Cu); the top stays clear for the carbon contacts. Only LED1/LED2 are on top.
- The layout is spaced for an iron: 3 mm clear outside the XIAO castellations, 2 mm in front of U2 and the JST pins, 5 mm in front of the 0.5 mm-pitch FPC pins (J1/J2) for drag soldering, and at least 1.5 mm between passives.

**Before first power**
- Battery connector polarity matches the board (see Battery).
- No bridge between the BAT+/BAT− slots.
- Both stick flexes are seated contacts-to-board, with no twist.

## Editing the design

- Net classes are in the `.kicad_pro`: Default 0.25 mm / 0.2 mm clearance; Power (+BATT, VBUS, +3V3, LED_VDD) 0.5 mm; Stick (axes, L3/R3, +3V0) and GND 0.15 mm clearance, because J1/J2's pads are only 0.2 mm apart.
- Rule areas keep vias and pour out of the carbon contacts and vias out from under the XIAO.
- **Expected DRC errors:** U1 pads 1 (STICK_LX) and 14 (VBUS) overhang the board edge slightly, giving two edge-clearance errors. That's intended: the XIAO's position is set by the enclosure.
- Keep X/Y stick traces short and away from the LED data line, I2C and the antenna.
- Footprints, symbols and 3D models are all in `hardware/pcb/lib/`. **Update Footprint from Library** on J3, J4 or J5 swaps their 3D model for one KiCad doesn't ship. Re-add the model from `lib/3dmodels/` afterwards:
  - J4/J5: `S3B-PH-SM4-TB_LF__SN_.stp`, offset (0, −4.655, 0.1) mm, rotation (−90, 0, 0).
  - J3: `B2B-PH-SM4-TBT_LF__SN_.stp`, offset (−3.97, 3.225, −0.245) mm, rotation (−90, 0, 0).

## Most importantly

- Have fun :)

polymatt, 2026
