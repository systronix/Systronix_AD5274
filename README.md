# Systronix_AD5274
Arduino library for the Analog Devices AD5272 and AD5274 Digital Rheostats with I2C interface.
The AD5274 is 256-position (8 bits) and the AD5272 is 1024-position (12 bits).
There are 20 kΩ, 50 kΩ, 100 kΩ nominal resistance versions. We have mostly used the 20K, 256-position.
This library supports both 8- adn 10- bit versions.

## AD5272 and AD5274 Key Features
 - I2C Interface. including 400 kHz fast mode
 - address input with three possible values for three sub-addresses in I2C space
 - reset input
 - 50-times programmable wiper memory (you don't have to us this), which restores to the programmed value on reset.
 - if you have not used the 50-TP memory, reset loads a midscale value
 - single or dual supply operation for unipolar or bipolar applications
 - unlimited setting lifetime via I2C interface
 - same code can work on both resolution devices, the two lsb are ignored on the 8-bit device
 - LFCSP or MSOP (what we use) 10 lead packages
 - AD5274 [tech info and data sheet](http://www.analog.com/en/products/digital-to-analog-converters/digital-potentiometers/ad5274.html)
 - $2.61 each, $1.92 @100 from Digikey [AD5274BRMZ-20](http://www.digikey.com/product-detail/en/analog-devices-inc/AD5274BRMZ-20/AD5274BRMZ-20-ND/2237574)

## How and Why We Use It
 - set point for a programmable current sink used to precisely drive a laser or LED, it is an input to the opamp which drives a FET
 - we could have used a DAC but this device has a programmable setpoint which does not require configuration once set.

### Comments
 - Tested with Arduino Uno and both AD5274 and AD5272, on a custom Arduino shield
 - See the source code for plenty of explanatory comments
 - The 50-TP memory is a bit unconventional, see the data sheet and code for comments and examples

### TODO
 - add more comments about 50-TP memory and the example memory dump program
 - add doxygen docs
