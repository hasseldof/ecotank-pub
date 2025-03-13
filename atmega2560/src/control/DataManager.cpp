// Author: Christoffer H. Brix

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <util/atomic.h>
#include "../boundary/RPiIF.h"
#include "../boundary/RangingModuleIF.h"
#include "../boundary/TempSensorIF.h"
#include "../domain/SystemData.h"
#include "../switch.h"
#include "DataManager.h"

DataManager::DataManager(RPiIF& rpiIF, RangingModuleIF& rangeIF, TempSensorIF& tempIF, SystemData& sysData) :
	rpiInterface_(rpiIF), rangeInterface_(rangeIF), tempInterface_(tempIF), systemData_(sysData) {}

// The logic implemented here together with the limits set by individual interfaces should allow calling this with high frequency
void DataManager::updateSystemData() {
	//char message[50];	// For test/debug printing

	// Send the pulse; get the distance later - it takes approx. 2ms before we receive a rising edge, so we can do other stuff in the meantime
	bool triggerSent = rangeInterface_.sendTriggerPulse();
	
	if (triggerSent) {
		PORTB ^= (1<<PB3);	// For debugging/visualization
	}

	// Flip a bool if switch 7 is pressed; to simulate 91 degrees
	static bool simulateExtremeTemp = false;
	if (switchOn(7)) {
		simulateExtremeTemp = !simulateExtremeTemp;
	}

	// Hi-jack the temperature setting logic if we want to simulate 91 degrees
	float currentTemp = 0.0;
	if (simulateExtremeTemp) {
		currentTemp = 91.0;
	} else {
		currentTemp = tempInterface_.getTemp();
	}
	systemData_.setTemp(currentTemp);

	// Variable to hold whether new data is available. This is used to determine if we should send data to the RPi
	bool isNewDataAvailable = false;
	uint8_t framesReceived = 0;
	
	// Disable interrupts while checking if any data frames are available
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		framesReceived = rpiInterface_.frameCount();
	}

	// If we have new data, get it
	if (framesReceived) {
		uint8_t frameSize = 0;
		uint8_t frameBuffer[RPiIF::kFrameSize];
		uint8_t systemPowerIndex = 0;
		uint8_t setPointIndex = 0;

		for (uint8_t i = 0; i < RPiIF::kFrameSize; i++) {
			frameBuffer[i] = 0;
		}

		for (uint8_t i = 0; i < framesReceived; i++) {
			// Disable interrupts while getting the data
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				frameSize = rpiInterface_.getDataFrame(frameBuffer);
			}

			// Check if the frame is the expected size, which it should always be, since it's also checked by getDataFrame
			if (frameSize == RPiIF::kFrameSize) {
				for (uint8_t i = 0; i < frameSize; i++) {
					if (frameBuffer[i] == RPiIF::SYSTEM_POWER_ID_BYTE) {
						systemPowerIndex = i + 1;
					} else if (frameBuffer[i] == RPiIF::SETPOINT_ID_BYTE) {
						setPointIndex = i + 1;
					}
					
					// Module test printing
					//snprintf(message, sizeof(message), "Byte %d: %d\n\r", i, frameBuffer[i]);
					//rpiInterface_.send(message);
				}

				isNewDataAvailable = true;

				systemData_.setSystemPower((bool)frameBuffer[systemPowerIndex]);
				
				// Module test printing
				//snprintf(message, sizeof(message), "System power set to: %d\n\r", (int)frameBuffer[systemPowerIndex]);
				//rpiInterface_.send(message);

				float setPoint = 0;		// Variable to unpack the float into by memcpy
				memcpy(&setPoint, &frameBuffer[setPointIndex], sizeof(float));	// Unpack the float - the packed float should be the 4 bytes after the system power byte
				systemData_.setSetPoint(setPoint);
				
				// Module test printing
				//uint16_t scaledSetpoint = (uint16_t)(setPoint * 10);
				//snprintf(message, sizeof(message), "Setpoint set to: %d.%d\n\r", scaledSetpoint / 10, scaledSetpoint % 10);
				//RPiIF::getInstance().send(message);
			}
		}
	}

	// Check if we have new data from the ranging module
	// Don't update the distance if it isn't new
	if (rangeInterface_.hasUnreadDistance()) {
		systemData_.setCurrentRange(rangeInterface_.getCurrentDistance());
		systemData_.setLastRange(rangeInterface_.getLastDistance());
	}
	
	/*
	Byte Positions:
		byteSequence[0]: START_ID_BYTE
		byteSequence[1]: WATER_LEVEL_ID_BYTE
		byteSequence[2]: waterLevelPercentage (1 byte)
		byteSequence[3]: TEMPERATURE_ID_BYTE
		byteSequence[4] to [7]: currentTemp (4 bytes)
		byteSequence[8]: STOP_ID_BYTE
	*/
	static uint8_t noDataCounter = 0;
	
	// Send current data to the RPi
	if (isNewDataAvailable) {
		noDataCounter = 0;
		uint8_t byteSequence[9];
		uint16_t currentDistance = systemData_.getCurrentRange();
		
		uint8_t waterLevelPercentage = (uint8_t)round((((float)systemData_.getMaxDistance() - currentDistance) / ((float)systemData_.getMaxDistance() - systemData_.getMinDistance())) * 100);
		
		byteSequence[0] = RPiIF::START_ID_BYTE;
		byteSequence[1] = RPiIF::WATER_LEVEL_ID_BYTE;
		byteSequence[3] = RPiIF::TEMPERATURE_ID_BYTE;
		byteSequence[8] = RPiIF::STOP_ID_BYTE;
		//
		memcpy(&byteSequence[2], &waterLevelPercentage, sizeof(uint8_t));
		memcpy(&byteSequence[4], &currentTemp, sizeof(float));
		//
		RPiIF::getInstance().send(byteSequence, sizeof(byteSequence) / sizeof(uint8_t));
	} else {
		noDataCounter = noDataCounter < UINT8_MAX ? noDataCounter + 1 : 20;
	}
	
	// "Turn off" the system if no valid dataframes has been received after 20 calls
	if (noDataCounter >= 20) {
		systemData_.setSystemPower(false);
	}
}


