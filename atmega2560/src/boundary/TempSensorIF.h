#ifndef TEMPSENSORIF_H
#define TEMPSENSORIF_H
#include <avr/io.h>
#include <stdio.h>		//Erase if not using UART

class TempSensorIF{
public:
	static TempSensorIF& getInstance() { return instance_; }
	float getTemp();
	
private:
	TempSensorIF() = default;
	~TempSensorIF() = default;
	
	void initDS18B20();
	void skipRom();
	void sendByte(uint8_t Byte);
	uint16_t readScratchpad();
	uint8_t readByte();
	uint8_t checkBit();

	static TempSensorIF instance_;
	static constexpr int8_t scratchPadLength_ = 2;	//Would typically be 9 in size
	static constexpr uint8_t MSB_ = 1;
	static constexpr uint8_t LSB_ = 0;
	uint8_t skipRom_ = 0b11001100;					//[CCh]	Rom command
	uint8_t ConvertT_ = 0b01000100;					//[44h] Function command
	uint8_t ReadScratchpad_ = 0b10111110;			//[BEh] Function command
	
	int scratchPad_[scratchPadLength_];				//Would typically be 9 in size
};

#endif //TEMPSENSORIF_H
