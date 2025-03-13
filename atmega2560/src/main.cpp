// Author: Christoffer H. Brix

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <util/delay.h>
#include "boundary/RPiIF.h"
#include "boundary/RangingModuleIF.h"
#include "boundary/TempSensorIF.h"
#include "boundary/PumpIF.h"
#include "boundary/HeatingUnitIF.h"
#include "control/BrugVarmtVand.h"
#include "control/DataManager.h"
#include "control/RegulateTemp.h"
#include "domain/SystemData.h"
#include "switch.h"

//void calibrate(SystemData& sysData, DataManager& dataManager, uint16_t& maxDistance, uint16_t& minDistance);
uint16_t constexpr kMaxDistance = 133;
uint16_t constexpr kMinDistance = 33;

int main(void)
{
	// Class instantiation
	SystemData systemData(kMaxDistance, kMinDistance);
	DataManager dataManager(RPiIF::getInstance(), RangingModuleIF::getInstance(), TempSensorIF::getInstance(), systemData);
	BrugVarmtVand brugVarmtVand(systemData);

	// Used for "simulating" 91 degrees in DataManager
	initSwitchPort();

	/***************************************
	//char buffer[256];			// For Debug printing
	
	while (!switchOn(7)) {}		// Press switch 7 to start calibrating
	uint16_t maxDistance = 0;	// Set by calibrate
	uint16_t minDistance = 0;	// Set by calibrate
	
	// Will signal when entered.
	// Hold down SW6 while tank is at minimum desired water level/to set max distance. LED0 lights up when enough samples have been taken
	// Hold down SW5 while tank is at maximum desired water level/to set min distance. LED1 lights up when enough samples have been taken
	// All leds lights up for a second when complete
	calibrate(systemData, dataManager, maxDistance, minDistance);
	******************************************/
	
	// Just set all of the pins on PORTB as output instead of using the init() function of HeatingUnitIF and PumpIF, since we use the LEDs
	DDRB = 0xFF;
	PORTB = 0;
	
	// Global interrupts enabled
	sei();

	while (1) {
		// This allows us to limit executions to approx. every 4.1 ms. (1/16e6)*65535*1000 = 4.1 ms; not accounting for execution time of functions.
		// Define which tick for offsetting it.
		static uint16_t tick = 0;

		// Actual main loop
		if (tick == 32767) {
			dataManager.updateSystemData();

			if (systemData.getSystemPower()) {
				PORTB |= (1<<PB0);
				RegulateTemp::regulateTemp(systemData);
				brugVarmtVand.checkDistance();
			} else {
				PORTB &= ~(1<<PB0);
				HeatingUnitIF::stop();
				PumpIF::stop();
			}
		}
		
		// Debug printing tick
		if (tick == 65535) {
			//bool systemPower = systemData.getSystemPower();
			//float setPoint = systemData.getSetPoint();
			//uint16_t range = systemData.getCurrentRange();
			//uint16_t lastRange = systemData.getLastRange();
			//float temp = systemData.getTemp();
			//
			//uint16_t scaledSetpoint = (uint16_t)(setPoint * 10);
			//uint16_t scaledTemp = (uint16_t)(temp * 10);
			
			//snprintf(buffer, sizeof(buffer), "SysPow: %d\n\rSetPoint: %d.%d\n\rRange: %d\n\rLastRange: %d\n\rTemp: %d.%d\n\r", (uint8_t)systemPower, scaledSetpoint / 10, scaledSetpoint % 10, range, lastRange, scaledTemp / 10, scaledTemp % 10);
			//RPiIF::getInstance().send(buffer);
			tick = 0;
		}
		tick++;
	}
	
}

// Unused since we hardcoded the max and min distances
void calibrate(SystemData& sysData, DataManager& dataManager, uint16_t& maxDistance, uint16_t& minDistance) {
	PORTB |= (1<<PB0) | (1<<PB1);;
	_delay_ms(1000);
	PORTB = 0;
	
	const int numSamples = 20;
	uint16_t sumMax = 0, sumMin = 0;
	int countMax = 0, countMin = 0;

	while (countMax < numSamples || countMin < numSamples) {
		if (switchOn(6)) {
			while (switchOn(6) && countMax < numSamples) {
				dataManager.updateSystemData();
				sumMax += sysData.getCurrentRange();
				countMax++;
				_delay_ms(70);		// Short delay to control not breaking the interrupt logic in rangingmodule
			}
			if (countMax >= numSamples) {
				PORTB |= (1<<PB0);
				_delay_ms(1000);
				PORTB &= ~(1<<PB0);
			}
		}

		if (switchOn(5)) {
			while (switchOn(5) && countMin < numSamples) {
				dataManager.updateSystemData();
				sumMin += sysData.getCurrentRange();
				countMin++;
				_delay_ms(70);		// Short delay to control not breaking the interrupt logic in rangingmodule
			}
			if (countMin >= numSamples) {
				PORTB |= (1<<PB0);
				_delay_ms(1000);
				PORTB &= ~(1<<PB0);
			}
		}
	}

	if (countMax == numSamples) {
		maxDistance = sumMax / numSamples;
	}

	if (countMin == numSamples) {
		minDistance = sumMin / numSamples;
	}
	PORTB = 0xFF;
	_delay_ms(1000);
	PORTB = 0;
}


