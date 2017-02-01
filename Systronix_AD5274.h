/******************************************************************************/
/*!
	@file		Systronix_ModulatorShield.h
	
	@author		B Boyes (Systronix Inc)
    @license	BSD (see license.txt)	
    @section	HISTORY

	v1.0	2015 Jul 01 bboyes Start based on working Arduino .ino program

*/
/******************************************************************************/

/** TODO LIST

Wow. Nothing right now. Cool.

**/

/** --------  Description ------------------------------------------------------
 *
 * AD5274 is a digital potentiometer with 8-bits of resolution
 * AD5272 is 10-bit version
 *
 * Full scale resistance is 10K, 20K, or 100K

  When new and nothing has been programmed into eeprom,
  it is set at 50% = 10K . Since we have resistors above and below it to set the min and max
  range, the set point is not just a linear function of the pot value. For example
  with R1 = 75K and R7 = 120 ohms, the Rpot of zero gives 5 mA drive,
  50% Rpot gives 196 mA, and 100% gives 349 mA.
  This makes things more complicated than a simple DAC, but also lets us scale the output
  range in ways that would be harder with a DAC.

  With the ADDR input grounded our device 7-bit address is B0101111 and since Arduino Wire
  library expects a 7-bit address it should be happy with that.
  Commands are sent msb first, upper two bits are 00 then a 4-bit command, then 10 bits of data;
  16 bits total. For the 8-bit AD5274 the two lsb of data are don't care.

 Commands:
 00 0001 [10-bit data] writes the data to the RDAC. This can be done unlimited times.

The AD527[2|4] has one address pin which controls the two lsb of the address.
ADDR	I2C base
GND 	0x2F
Vdd 	0x2C
N/C 	0x2E (unipolar power only)

------------------------------------------------------------------------------*/

/**
 *  Has anyone already written a driver"
 *  Yes, for Linux: https://code.google.com/p/android-source-browsing/source/browse/drivers/misc/
 *  ad525x_dpot-i2c.c?repo=kernel--samsung&name=android-samsung-3.0-ics-mr1&r=a4bd394956f20d0bfc0ca6ecfac2af4150da274a
 */

/** -------- Arduino Device and Version Testing and Issues ---------------------

This library assumes Arduino 1.6.5-r2 but should work with older versions and 1.06

Should also work with Teensy
------------------------------------------------------------------------------*/

#include <Arduino.h>
#include "Wire.h"

// Preprocessor guard against multiple inclusion
#ifndef SYSTRONIX_AD5274_H
#define SYSTRONIX_AD5274_H

/**
 * I2C ADDRESS CONSTANTS
 * I2C base addresses of AD5274 dependent on ADDR pin connection
*/
#define AD5274_BASE_ADDR_GND 0x2F	/**< Default I2C address iff ADDR=GND, see notes above */
#define AD5274_BASE_ADDR_VDD 0x2C	/**< I2C address iff ADDR=VDD, see notes above */
#define AD5274_BASE_ADDR_FLOAT 0x2E	/**< I2C address iff ADDR=floating, see notes above */

/**
 * COMMAND CONSTANTS
 * See Table 12, page 21, in the Rev D datasheet
 * Commands are 16-bit writes: bits 15:14 are 0s
 * Bits 13:10 are the command value below
 * Bits 9:0 are data for the command, but not all bits are used with all commands
 */

// The NOP command is included for completeness. It is a valid I2C operation.
#define AD5274_COMMAND_NOP 0x00	// Do nothing. Why you would want to do this I don't know

// write the 10 or 8 data bits to the RDAC wiper register (it must be unlocked first)
#define AD5274_RDAC_WRITE 0x01

#define AD5274_RDAC_READ 0x02	// read the RDAC wiper register

#define AD5274_50TP_WRITE 0x03	// store RDAC setting to 50-TP

// SW reset: refresh RDAC with last 50-TP stored value
// If not 50-TP value, reset to 50% I think???
// data bits are all dont cares
#define AD5274_RDAC_REFRESH 0x04	// TODO refactor this to AD5274_SOFT_RESET

// read contents of 50-TP in next frame, at location in data bits 5:0,
// see Table 16 page 22 Rev D datasheet
// location 0x0 is reserved, 0x01 is first programmed wiper location, 0x32 is 50th programmed wiper location
#define AD5274_50TP_WIPER_READ 0x05

/**
 * Read contents of last-programmed 50-TP location
 * This is the location used in SW Reset command 4 or on POR
 */
#define AD5274_50TP_LAST_USED 0x06

#define AD5274_CONTROL_WRITE 0x07// data bits 2:0 are the control bits

#define AD5274_CONTROL_READ 0x08	// data bits all dont cares

#define AD5274_SHUTDOWN 0x09	// data bit 0 set = shutdown, cleared = normal mode


/**
 * Control bits are three bits written with command 7
 */
// enable writing to the 50-TP memory by setting this control bit C0
// default is cleared so 50-TP writing is disabled
// only 50 total writes are possible!
#define AD5274_50TP_WRITE_ENABLE 0x01

// enable writing to volatile RADC wiper by setting this control bit C1
// otherwise it is frozen to the value in the 50-TP memory
// default is cleared, can't write to the wiper
#define AD5274_RDAC_WIPER_WRITE_ENABLE 0x02

// enable high precision calibration by clearing this control bit C2
// set this bit to disable high accuracy mode (dunno why you would want to)
// default is 0 = emabled
#define AD5274_RDAC_CALIB_DISABLE 0x04

// 50TP memory has been successfully programmed if this bit is set
#define AD5274_50TP_WRITE_SUCCESS 0x08


class Systronix_AD5274
{
	protected:
		// Instance-specific properties

		// Instance-specific properties
		uint8_t _base; 	// I2C base address, see notes above
		uint8_t _bits;	// 8 or 10 bit device

   
	public:

		Systronix_AD5274(uint8_t base);		// constructor: base address

		int8_t begin(void);

		int8_t command_write(uint8_t command, uint16_t data);
		int16_t command_read(uint8_t command, uint8_t data);
		int8_t control_write_verified(uint8_t control);	// write 3 ls data bits to control reg

		boolean is_available(uint8_t *i2c_status);

		// not implemented yet:
		int8_t unlock(boolean rdac, boolean fiftyTP);		// true to unlock, false to lock, default = locked

		int8_t fiftytp_unlock(boolean unlock);		// true unlocks, false locks, default = locked
		int8_t rdac_calib(boolean enable);		// true enables, default = enabled

		int8_t wiper_write(uint16_t wiper);		// writer 'wiper' to rdac wiper register
		int8_t wiper_read(uint16_t wiper);		// read rdac wiper into 'wiper'
		uint8_t BaseAddr;


	private:

};


#endif /* SYSTRONIX_AD5274_H */
