# Wiring

## Lolin D1 mini Pro
**Pin usage**
Pin | Purpose
--- | -------
A0  | battery
D0  | sleep
D2  | PIR via I2C cable
D4  | builtin led
D5  | buzzer
D6  | reserved for potential LEDs 2
D7  | LEDs 1

**Construction**
- Bridge pads
  - BAT-A0
  - SLEEP
- Solder header pins
  - GND
  - 3V3
  - 5V
  - 12
  - 13
  - 14

## LEDs
**Construction**
- Cut Trace
  - D4
- Bridge pad
  - D7
- Solder header pins
  - GND
  - 3V3
  - 5V
  - D7

## Buzzer
**Construction**
- Solder 1N4148 diode
  - Anode to 3V3
  - Cathode to 5V
- Solder header pins
  - GND
  - 3V3
  - 5V
  - D5

## PIR
Connected to Lolin D1 mini Pro via I2C Cable
