#include "TempSensorIF.h"
#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>

TempSensorIF TempSensorIF::instance_;

float TempSensorIF::getTemp() {
	//Transactions sequence: Step 1 2 3
	initDS18B20();					//Reset & Presence
	skipRom();						//Skip Rom [CCh]
	sendByte(ConvertT_);				//Convert T[44h] function command.
	
	initDS18B20();					//From Operation Example 1, master issues reset pulse again, after conversion
	skipRom();						//Skip Rom [CCh]
	sendByte(ReadScratchpad_);		//Read scratchpad command

	float temperatureC = (float)readScratchpad()/16.0;		//Convert the bits to temperature as a float
	return temperatureC;
}

void TempSensorIF::initDS18B20(){	
	DDRE |= 0b00010000;				//Set PE4 to output
	PORTE &= ~0b00010000;			//Master tx Reset pulse
	_delay_us(480);					//Master tx
	DDRE &= ~0b00010000;			//Set PE4 as input
	_delay_us(60);					//Waits for DS18B20
	
	while(0 == (PINE & (1 << PINE4))){}	//Checks if Presence pulse is received
		
	_delay_us(480);					//Master rx (480 us)
}

void TempSensorIF::skipRom(){
	DDRE |= 0b00010000;				//Set PE4 as output
	sendByte(skipRom_);				//Skip Rom [CCh]
}

void TempSensorIF::sendByte(uint8_t Byte){
	uint8_t temporary = 0;
	for (int i = 0; i < 8; i++){
		temporary = Byte & 0x1;
		if(temporary & 1){					//Check if the bit is LOW or HIGH
			PORTE &= ~0b00010000;			//Send LOW
			_delay_us(5);					//Master Write "1" slot
			PORTE |= 0b00010000;			//Send HIGH
			_delay_us(55);					//Master Write "1" slot
		}
		else{
			PORTE &= ~0b00010000;			//Send LOW
			_delay_us(55);					//Master Write "0" slot
			PORTE |= 0b00010000;			//Send HIGH
			_delay_us(5);					//Master Write "0" slot
		}
		Byte >>= 1;
	}
}

uint16_t TempSensorIF:: readScratchpad(){	//Stores the 16 temperature bits
	uint8_t buffer[scratchPadLength_];
	for (int8_t i = 0; i < scratchPadLength_; i++){
		buffer[i] = readByte();
	}
	return (buffer[MSB_]<<8) | buffer[LSB_];		//Returns 16-bit temperature data
}



//The part of handling bits/bytes is kindly borrowed from stecman https://gist.github.com/stecman/9ec74de5e8a5c3c6341c791d9c233adc
uint8_t TempSensorIF::readByte(){			//Stores a byte in buffer
	uint8_t buffer = 0x0;
	DDRE &= ~0b00010000;					//Set PE4 as input
	for (uint8_t bit = 0x01; bit; bit <<= 1)	{
		if(checkBit())
		{
			buffer |= bit;
		}
	}
	return buffer;
}

uint8_t TempSensorIF::checkBit(){			
	//Read time slot
	DDRE |= 0b00010000;						//Set PE4 as output 
	PORTE &= ~0b00010000;					//Send LOW to PE4
	_delay_us(1);							//Read 1/0 slot
	
	DDRE &= ~0b00010000;					//Set PE4 as input, release the bus
	_delay_us(10);

	uint8_t result = (PINE & (1 << PINE4)) ? 1 : 0;		//If PE4 == 1, set to 1, else set to 0
	_delay_us(50);

	return result;
}