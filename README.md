# DS to I2C Expander
Software for Atmega**8 for connecting up to 40 DS18b20 devices to one I2C bus.

We have many DS sensors and didn't want to/couldn't connect them all to a Raspberry Pi due to lack of pins.
We therefore created a small breakout board with an ATmega168(L) on it to read the sensors and make readings available via I2C.
The devices are connected to 4 1-wire buses, which can be searched and read out through setting bits in the control register at `0x00`.

## Board
The board provides 4 independent 1-Wire buses with 4 3-pin connectors each.
The DS are powered by 5V, but the data line is pulled up to 3.3V (which is the supply voltage of the Atmega).

Additionally there are 7 GPIO pins on the board, that can also be controlled via I2C.

For monitoring/debugging, there are 2 LEDs that indicate working 3.3V and 5V voltages.
Another 2 LEDs are controlled by the mC and indicate healthy operation (blinking @ 1Hz) and ongoing readouts (second LED on).

## I2C Interface
The device address is `0x3f`.
The I2C interface exposes 254 8bit registers:
Register | Semantics | Allowed values (writing)
---|---|---
0 | Controlling 1-Wire buses | 1 / 2 / 4 / 8 / 16
1 | GPIO Data Direction | 0-127
2 | GPIO Values | 0-127
3 | CRC over following bytes| -
4 | Length n | -
5 | Data Byte 1 | -
6 | Data Byte 2| -
7 | ... | -
8 | Data Byte n-1 | -

- Register 1 allows reading/setting the data direction (input or output) of the 7 GPIO pins.
- Register 2 allows reading the state of input GPIOs and setting the state of output GPIOs.
- The CRC is the 1-Wire CRC8. It is computed over register 4 and the following (actually used!) data bytes (end defined by the content of register 4).
- Register 4 contains the number of data bytes that follow. All other registers will read as 0.

During 1-Wire operations, no I2C communication is possible (interrupts disabled)!

## 1-Wire commands
The four 1-Wire buses can be controlled by setting any of the 5 LSBs in the command register (only one at a time).

Bits 0 to 3 will trigger a device search on the corresponding 1-Wire bus.
After ~1s, the data bytes will contain an ordered list of all addresses on this bus.

Bit 4 will trigger the conversion (actual measurement of temperatures by DSs) of all devices on all buses.
After ~1s, the data bytes will contain the temperature data. This consists of 2 bytes per DS, as specified in the data sheet.

The conversion command depends on the 1-Wire addresses, that were previously found by the device search command(s).
A search on bus 0 will first reset all addresses. A search on any bus must not be triggered after a search on a bus with higher index was triggered.
It is thereby strongly advised to set bits 0 to 3 in this order for initialization and then only set bit 3 for readout.

## Test scripts
There are two python scripts, that allow sending 1-Wire commands and reading/setting GPIO values from a terminal on a Raspberry Pi.

## License
MIT, but would be nice if you would link back here.
