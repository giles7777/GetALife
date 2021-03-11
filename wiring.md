# Wiring
(updated 2/14/21)
## ESP8266 Lolin D1 Pro
Pins used: A0:batt, D0:sleep, D4:builtin led, D5:buzzer, D7:LED, D2:PIR, D6:(reserved)

Solder connections: bridge BAT-A0, bridge SLEEP; connect +5, +3.3, GND, D5, D7

## LED Lolin 7 led board
Solder connections: bridge D7 (not D4 default); connect +5, +3.3, GND, D7
## Buzzer
Solder connections: none (D5 default);  connect +5, +3.3, GND, D5

## I2C Cable: 
D1 (SCL) D2 (SDA)
## PIR: 
bridge D2 (not D3 default)

## deprecated:
### Relay
bridge D6 (not D1 default)
