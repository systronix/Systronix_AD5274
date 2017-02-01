#include <Arduino.h>
#include <Systronix_AD5274.h>
#include <Wire.h>

Systronix_AD5274 my_ad5274(AD5274_BASE_ADDR_GND);

// in this IDE, pin 2 is IOL.2 and we drive it low to enable our blue LED
#define LED_ON_L 2

void setup() {
  // put your setup code here, to run once:

	  delay (2000);      // give some time to open monitor window
	  Serial.begin(115200);     // use max baud rate
	  // Teensy3 doesn't reset with Serial Monitor as do Teensy2/++2, or wait for Serial Monitor window
	  // Wait here for 10 seconds to see if we will use Serial Monitor, so output is not lost
	  while((!Serial) && (millis()<10000));    // wait until serial monitor is open or timeout

	  Serial.print("AD5274 Register Test Code at 0x");
	  Serial.println(my_ad5274.BaseAddr, HEX);

	  my_ad5274.begin();

	  digitalWrite(LED_ON_L, HIGH);      // be sure LED is off; this must be done before making the pin an output!
	  pinMode(2, OUTPUT);

	  digitalWrite(LED_ON_L, LOW);      // turn on output

}

int16_t read_from_ad5274 = 0;

uint16_t data_16_to_write = 0;
int8_t status = 0;

int8_t i2c_avail = false;
int16_t i2c_busy_count=0;
uint8_t i2c_status = 0;

void loop() {
  // put your main code here, to run repeatedly:

	for (uint8_t command = 0; command <12; command++)
	{
		Serial.print("Try read command: ");
		Serial.print(command);
		read_from_ad5274 = my_ad5274.command_read(command, 0x00);
		Serial.print(" got ");
		if (read_from_ad5274 < 0)
		{
			Serial.print(read_from_ad5274, HEX);
			Serial.print("/");
			Serial.println(read_from_ad5274);


		}
		else
		{
			Serial.println(read_from_ad5274, HEX);
		}
	}

	read_all_50tp();


	status = my_ad5274.control_write_verified(0x0);
	status += my_ad5274.control_write_verified(AD5274_RDAC_WIPER_WRITE_ENABLE);
	if (status > 0)
	{
		Serial.print(" Error count after control writes=");
		Serial.println(status);
	}

	// unlock RDAC
	status = my_ad5274.command_write(AD5274_CONTROL_WRITE, AD5274_RDAC_WIPER_WRITE_ENABLE);
	read_from_ad5274 = my_ad5274.command_read(AD5274_CONTROL_READ, 0x00);
	if (read_from_ad5274 & AD5274_RDAC_WIPER_WRITE_ENABLE)
	{
		Serial.print("RDAC unlock successful: ");
		Serial.println(read_from_ad5274);
	}
	else
	{
		Serial.print("RDAC unlock failed: ");
		Serial.println(read_from_ad5274);
	}

	read_from_ad5274 = my_ad5274.command_read(AD5274_RDAC_READ, 0x00);
	Serial.print(" RDAC read 0x");
	Serial.println(read_from_ad5274, HEX);

	// write and then read RDAC register
	data_16_to_write = 0x3FF;

	status += write_and_read_rdac (data_16_to_write);
	delay(2000);

	data_16_to_write = 0x1FF;
	status += write_and_read_rdac (data_16_to_write);
	delay(2000);

	Serial.println("SW Reset");
	status += my_ad5274.command_write(AD5274_RDAC_REFRESH, 0x0);
//	Serial.println("Now read RDAC");
//	delay(1);


	uint8_t i2c_busy_code = 0;
	while (false == my_ad5274.is_available(&i2c_status))
	{
		i2c_busy_count++;
		i2c_busy_code = i2c_status;
	}
	Serial.print("I2C was busy ");
	Serial.print(i2c_busy_count);
	Serial.print(" times and code was ");
	Serial.print(i2c_busy_code);
	/**
	 * 1:data too long to fit in transmit buffer
	 * 2:received NACK on transmit of address
	 * 3:received NACK on transmit of data
	 * 4:other error
	 */
	if (2 == i2c_busy_code)
		{
		Serial.println(" which is I2C Address NACK and is expected");
		}
	else
	{
		Serial.println(" unexpected reason");
	}

	read_from_ad5274 = my_ad5274.command_read(AD5274_RDAC_READ, 0x00);
	Serial.print(" RDAC after SW reset: 0x");
	Serial.println(read_from_ad5274, HEX);

	// unlock RDAC
	status = my_ad5274.command_write(AD5274_CONTROL_WRITE, AD5274_RDAC_WIPER_WRITE_ENABLE);
	read_from_ad5274 = my_ad5274.command_read(AD5274_CONTROL_READ, 0x00);
	if (read_from_ad5274 & AD5274_RDAC_WIPER_WRITE_ENABLE)
	{
		Serial.print("RDAC unlock successful: ");
		Serial.println(read_from_ad5274);
	}
	else
	{
		Serial.print("RDAC unlock failed: ");
		Serial.println(read_from_ad5274);
	}

	// from spreadsheet, 50% should be 0x1AF
	status += write_and_read_rdac (0x1AF);


	if (status > 0)
	{
		Serial.print(" RDAC read/write errors: ");
		Serial.println(status);
	}

	while (true)
	{

	}

}


/**
 *
 */
int8_t write_and_read_rdac (uint16_t data_16_to_write)
{
	int8_t status = 0;

	status += my_ad5274.command_write(AD5274_RDAC_WRITE, data_16_to_write);
	read_from_ad5274 = my_ad5274.command_read(AD5274_RDAC_READ, 0x00);
	Serial.print("RDAC wrote 0x");
	Serial.print(data_16_to_write, HEX);
	Serial.print(" read 0x");
	Serial.println(read_from_ad5274, HEX);

	return status;
}


/*
 * Read and print out the contents of all 50-TP memory locations
 */
int8_t read_all_50tp (void)
{
	Serial.println ("Reading all 50TP locations (legal are 0x01 through 0x32) - only nonzero data locations are shown");
	for (uint16_t location = 0; location < 0x400; location++)
	{
		read_from_ad5274 = my_ad5274.command_read(AD5274_50TP_WIPER_READ, location);
		if (read_from_ad5274 < 0)
		{
			Serial.print("@0x");
			Serial.print(location,HEX);
			Serial.print(" error ");
			Serial.println(read_from_ad5274);
		}
		else if (read_from_ad5274 > 0)
		{
			Serial.print("@0x");
			Serial.print(location,HEX);
			Serial.print(" ");
			Serial.print(read_from_ad5274, HEX);
			Serial.print("/");
			Serial.println(read_from_ad5274);
		}
	}
}
