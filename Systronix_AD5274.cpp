/*
 * Systronix_LM3406.cpp
 *
 *  Created on: 2015 Mar 08
 *      Author: bboyes
 */

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include "Systronix_AD5274.h"

#include <inttypes.h>

 byte _DEBUG = 1;
 
 
 

/**
 * Constructor
 * @param base the I2C base address
 * @see AD5274_BASE_ADDR_GND
 * @see AD5274_BASE_ADDR_VDD
 * @see AD5274_BASE_ADDR_FLOAT
 */
Systronix_AD5274::Systronix_AD5274(uint8_t base)
{
	_base = base;
	BaseAddr = base;
}

/**
 * Join I2C as master
 *
 * @return 0 if no I2C errors, else the error count
 */
int8_t Systronix_AD5274::begin(void) {
	uint8_t b = 0;
	Wire.begin();	// join I2C as master
//	  Wire.beginTransmission(_base);
//	  // write data to control register only 2 lsbs matter
//	  // returns # of bytes written
//	  b = Wire.write(data);
//	  // returns 0 if no error
//	  b+= Wire.endTransmission();

	 return b;

}

/**
 * Write to the control register, then read it back and verify
 * the read data matches the write data.
 *
 * @param control only 3 lsb are used, others ignored but if nonzero increments error count.
 * This can be a value or a combination of manifest constants. The default, POR value for
 * these bits is 000
 *
 * @see AD5274_50TP_WRITE_ENABLE
 * @see AD5274_RDAC_WIPER_WRITE_ENABLE
 * @see AD5274_RDAC_CALIB_DISABLE
 *
 * @return 0 if successful, nonzero for total number of errors which occurred
 *
 * @TODO this seems redundant. Instead reuse the more generic command_write function.
 */
int8_t Systronix_AD5274::control_write_verified(uint8_t control)
{
	uint8_t error_count = 0;
	uint8_t data = 0;

	uint8_t recvd = 0xFF;	// init with impossible value
	int8_t count = 0;
	uint16_t data_to_write = 0;

	if (control > 7)	// bad value, what is caller's intent?
	{
		// AD5274 will ignore all bits except 2:0 so we can proceed
		// but this is still not good, so bump the error counter
		error_count++;
	}
	data_to_write = (AD5274_CONTROL_WRITE<<10) | control;
	Wire.beginTransmission(_base);
	data = (data_to_write >> 8);		// ms byte to write
	count = Wire.write(data);
	data = data_to_write & 0x0FF;		// ls byte to write
	count += Wire.write(data);
	if (count!=2) error_count += (2-count);	// add number of bad writes

	error_count+= Wire.endTransmission(true);


	// now see if control reg was correctly written by reading it

	// write the control read command, data except the command value are don't cares
	data_to_write = AD5274_CONTROL_READ<<10;
	Wire.beginTransmission(_base);
	data = (data_to_write >> 8);		// ms byte to write
	count = Wire.write(data);
	data = data_to_write & 0x0FF;		// ls byte to write
	count += Wire.write(data);
	if (count!=2) error_count += (2-count);	// add number of bad writes

	error_count+= Wire.endTransmission(true);

	// now request the read data from the control register
	count = Wire.requestFrom(_base, (uint8_t)2, (uint8_t) true);

	if (2 != count)
	{
		Serial.print(" Error: I2C control read didn't return 2 but ");
		Serial.println(count);
		error_count += (2-count);	// add number of bad reads
	}
	recvd = Wire.read();	// ms byte is don't care, discard this
	recvd = Wire.read();	// ls byte bits 2:0 holds the control reg value

	if (recvd != control)
		{
			Serial.print(" Error: Control reg write and read don't match: ");
			Serial.print(control, HEX);
			Serial.print("/");
			Serial.println(recvd, HEX);
		}

	Serial.print("Control register value now ");
	Serial.println(recvd, HEX);

	return error_count;
}

/**
 * Send a command read and request data from the AD5274
 * To read, we send a  read command and (possibly) needed location data
 *
 * @param command valid values are
 * AD5274_RDAC_READ - reads current wiper value
 * AD5274_50TP_WIPER_READ also needs six bits of location data (which of 50, 0x01 to 0x32)
 * AD5274_50TP_LAST_READ - read the most recently programmed 50-TP location
 * AD5274_CONTROL_READ - read the value of the control register, in 4 lsbs
 *
 * @param write_datum is only used with read of 50-TP wiper memory, and only six lsb are used
 *
 * @return if positive it is read data; at most the 10 lsb are meaningful
 * if negative, it's the count of errors where more negative is more errors
 * if -100 the only error is "bad command", and no read was attempted
 * if a smallish but negative number there were I2C errors on attempted read, and the absolute value is the error count
 *
 *
 */
int16_t Systronix_AD5274::command_read(uint8_t command, uint8_t write_datum)
{
	uint8_t error_count = 0;
	uint8_t data = 0;
	uint16_t data_to_write = 0;
	int16_t read_datum = 0;

	// this will hold two data bytes as they are read
	uint8_t recvd = 0xFF;	// init with impossible value
	int8_t count = 0;

	if (AD5274_50TP_WIPER_READ == command)
	{
		// shift the command over into bits 13:10
		data_to_write = command<<10;
		// also need to send 6-bit 50-TP location
		data_to_write |= write_datum;
	}
	else if ( (AD5274_RDAC_READ == command) || (AD5274_50TP_LAST_USED == command) || (AD5274_CONTROL_READ == command) )
	{
		// command is in range and something possible to read
		// shift the command over into bits 13:10
		data_to_write = command<<10;
	}
	else
	{
		// It's either a bad command (out of range, > AD5274_SHUTDOWN), or its not a readable command
		// Bad command, we can't reasonably proceed
		error_count = 100;
		read_datum = error_count * -1;
	}

	/**
	 * At this point, if error_count == 0 we have a valid read command
	 */
	if (0 == error_count)
	{
//		Serial.println(" No errors so far");
		Wire.beginTransmission(_base);		// I2C slave address
		data = (data_to_write >> 8);		// ms byte to write
		count = Wire.write(data);
		data = data_to_write & 0x0FF;		// ls byte to write
		count += Wire.write(data);
		if (count!=2) error_count += (2-count);	// add number of bad writes
		error_count+= Wire.endTransmission(true);

//		Serial.print(" Error count after command write=");
//		Serial.println(error_count);

		// now request the read data from the control register
		count = Wire.requestFrom(_base, (uint8_t)2, (uint8_t) true);
		if (2 != count)
		{
			Serial.print(" Error: I2C command ");
			Serial.print(command);
			Serial.print(" read didn't return 2 but ");
			Serial.println(count);
			if (count!=2) error_count += (2-count);	// add number of bad reads
		}
		recvd = Wire.read();	// ms byte
		read_datum = recvd << 8;

		recvd = Wire.read();	// ls byte
		read_datum |= recvd;

		// if errors, return the count, make it negative first
		if (error_count > 0)
			{
			Serial.print(" Error count after command_read =");
			Serial.println(error_count);
			read_datum = -1 * error_count;
			}
	}

	return read_datum;
}

/**
 * Write a command to AD5274, Legal write commands are
 * AD5274_RDAC_WRITE write lsb 10- or 8- bit data to RDAC (it must be unlocked to allow this!) (uses sent data)
 * AD5274_50TP_WRITE store RDAC value to next 50-TP location (it must be unlocked to allow this!) (no data need be sent)
 * AD5274_RDAC_REFRESH (software rest) refresh RDAC with last 50-TP stored value (no data need be sent)
 * AD5274_CONTROL_WRITE write data bits 2:0 to the control register (3 lsb data are used)
 * AD5274_SHUTDOWN set data bit 0 to shutdown, clear it for normal operation
 *
 * @param command - the type of write we will do
 *
 * @param write_datum16 - the data we will use if the command supports it. At most the 10 lsb can be used.
 * To be safe we clip off any data in bits 15..10
 *
 * @return 0 if successful, negative value if errors. There is some encoding. Take the absolute value
 * of the negative return value, if bit 4 is set the datum was bad. If bit 5 is set the command was bad.
 * The lower 3 bits hold the count of I2C errors.
 *
 */
int8_t Systronix_AD5274::command_write(uint8_t command, uint16_t write_datum16)
{
	uint8_t error_count = 0;
	uint8_t data = 0;

	int8_t return_val = 0;
	int8_t count = 0;
	uint16_t data_to_write = 0;

	if (write_datum16 > 0x3FF)
	{
		// data in bits 13:10 will clobber the command when we OR in write_datum16
		write_datum16 &= 0x3FF;	// clip off any bad high bits even though we are going to error out and not use them
		// this is an error in the code using this function, we should not proceed with the write
		error_count |= 0x10;
	}

	if ( (AD5274_RDAC_WRITE == command) || (AD5274_CONTROL_WRITE == command) || (AD5274_SHUTDOWN == command) )
	{
		// these commands need to use data we send over
		// shift the command over into bits 13:10
		data_to_write = command<<10;
		// also need to send 10-bit or 8-bit wiper value, or 3 control bits, or shutdown bit
		data_to_write |= write_datum16;
	}
	else if ( (AD5274_50TP_WRITE == command) || (AD5274_RDAC_REFRESH == command) )
	{
		// shift the command over into bits 13:10
		data_to_write = command<<10;
		// no data needed
	}
	else
	{
		// It's either a bad command (out of range, > AD5274_SHUTDOWN), or its not a writeable command
		// Bad command, we can't reasonably proceed
		error_count |= 0x20;
	}

	// if no errors so far, command is valid and datum is too
	if (0 == error_count)
	{
		Wire.beginTransmission(_base);
		data = (data_to_write >> 8);		// ms byte to write
		count = Wire.write(data);
		data = data_to_write & 0x0FF;		// ls byte to write
		count += Wire.write(data);
		if (count!=2) error_count += (2-count);	// add number of bad writes

		error_count+= Wire.endTransmission(true);
	}

	// if errors, return the count, make it negative first
	if (error_count > 0)
		{
		Serial.print(" Error count after command_write = 0x");
		Serial.println(error_count, HEX);
		return_val = -1 * error_count;
		}

	return return_val;

}

/**
 * Can we use the I2C interface? It can be busy 350 msec after a 50-TP write
 * (datasheet p21), and some msec or less after a software reset (observed,
 * not documented). This is the way we are instructed to see if the part
 * is once again available. The data sheet calls this "I2C interface polling".
 *
 * @param i2c_status is a pointer to a uint8_t to hold the value for the reason I2C
 * was not available. This could be
 * 	0:success
 * 	1:data too long to fit in transmit buffer
 * 	2:received NACK on transmit of address
 * 	3:received NACK on transmit of data
 * 	4:other error
 * 	@see https://www.arduino.cc/en/Reference/WireEndTransmission
 *
 * @return true if the device can be accessed, else it is busy
 */
boolean Systronix_AD5274::is_available(uint8_t *i2c_status)
{
	int8_t error_count = 0;
	boolean i2c_available = false;

	Wire.beginTransmission(_base);
	error_count= Wire.endTransmission(true);

	*i2c_status = error_count;

	if (error_count > 0)
		{
		i2c_available = false;
//		Serial.print(" is_available is false because endTransmission =");
//		Serial.println(error_count);
		}
	else
	{
		 i2c_available = true;
	}

	return  i2c_available;
}

/**
 * unlock RDAC and/or 50TP memory, or lock them both
 * true unlocks
 */
int8_t Systronix_AD5274::unlock(boolean rdac, boolean fiftyTP)
{

	return -90;	// error, this routine is not complete!
}




