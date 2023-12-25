# ds-to-i2c-expander
A project that uses an ATmega8 to translate I2C to up to 40 DS18b20 sensors.

We have many DS sensors and didn't want to/couldn't connect them all to a Raspberry Pi due to lack of pins.
We therefore created a small breakout board with an ATmega168(L) on it to read the sensors and make readings available via I2C.
The devices are connected to 4 1-wire buses, which can be searched and read out through setting bits in the control register at `0x00`.


## License

MIT, but would be nice if you would link back here.
