PPD42NS, PPD42NJ - I2C interface
================================

PPD42 has not really standard PWM interface as data output. My meteo station is only able to read data from i2c, analog and SPI sensors.
Besides it works on 3v3, PPD requires 5V.
So this interface (with help of external voltage converter) provides data format conversion -> I2C, and data averaging in 12 minutes sliding window.

