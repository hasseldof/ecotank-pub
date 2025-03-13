// Author: Christoffer H. Brix

#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/atomic.h>
#include "../boundary/PumpIF.h"
#include "../boundary/RPiIF.h"
#include "../control/BrugVarmtVand.h"
#include "../domain/SystemData.h"

volatile uint8_t BrugVarmtVand::overflowCount = 0;

BrugVarmtVand::BrugVarmtVand(const SystemData& sysData) : sysData_(sysData), currentState_(State::CheckDistance), maxDistance_(sysData.getMaxDistance()), minDistance_(sysData.getMinDistance()) {
	initTimer4();
}

void BrugVarmtVand::initTimer4() {
	TCCR4A = 0;								// Normal mode
	TCCR4B |= (1<<CS40) | (1<<CS42);		// Prescaler 1024
}

void BrugVarmtVand::startTimer() {
	TIMSK4 |= (1<<TOIE4);					// Enable timer 4 overflow interrupt
	TCNT4 = 0;
}

void BrugVarmtVand::stopTimer() {
	TIMSK4 &= ~(1 << TOIE4);				// Disable timer 4 overflow interrupt

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		overflowCount = 0;
	}
}

void BrugVarmtVand::checkDistance() {	
	uint16_t currentDistance = sysData_.getCurrentRange();
	uint16_t lastDistance = sysData_.getLastRange();
	uint8_t overflows = 0;

	//char message[50];
	//snprintf(message, sizeof(message), "minDistance: %d\n\rmaxDistance: %d", minDistance_, maxDistance_);
	//RPiIF::getInstance().send(message);
	
	switch (currentState_) {
		case State::CheckDistance:
			stopTimer();
			if (isLevelMax(currentDistance)) {
				return;
			} else if (isLevelMin(currentDistance)) {
				currentState_ = State::ForceRefill;
			} else if (isDistanceDecreasing(currentDistance, lastDistance)) {
				PumpIF::stop();
			} else if (isDistanceStable(currentDistance, lastDistance)) {
				startTimer();
				currentState_ = State::TimerRunning;
			}
			break;
		
		case State::TimerRunning:
			
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				overflows = overflowCount;
			}

			// 86 = 5 Minutes, 3 = 12 sec (12 seconds for testing purposes. This value was used for acceptance testing.)
			if (overflows >= 3 && isDistanceStable(currentDistance, lastDistance)) {
				currentState_ = State::ForceRefill;
			}
				
			if (isDistanceDecreasing(currentDistance, lastDistance)) {
				currentState_ = State::CheckDistance;
			}
			break;
		
		case State::ForceRefill:
			stopTimer();
			// Continue refilling until we are at max water capacity (+- sensor inaccuracy).
			if (isWithinLimits(currentDistance) || isLevelMin(currentDistance)) {
				PumpIF::start();
			} else if (isLevelMax(currentDistance)) {
				PumpIF::stop();
				currentState_ = State::CheckDistance;
			}
			break;
	}
}

bool BrugVarmtVand::isLevelMin(const uint16_t& currentDistance) const {
	return currentDistance >= maxDistance_;
}

bool BrugVarmtVand::isLevelMax(const uint16_t& currentDistance) const {
	return currentDistance <= minDistance_;
}

bool BrugVarmtVand::isWithinLimits(const uint16_t& currentDistance) const {
	return (currentDistance < maxDistance_) && (currentDistance > minDistance_);
}

bool BrugVarmtVand::isDistanceDecreasing(const uint16_t& currentDistance, const uint16_t& lastDistance) const {
	if ((int16_t)(currentDistance - lastDistance) > (int16_t)stabilityThreshold_) {
		//RPiIF::getInstance().send("Distance decreasing: True\n\r");
		return true;
	}
	//RPiIF::getInstance().send("Distance decreasing: False\n\r");
	return false;
}

bool BrugVarmtVand::isDistanceStable(const uint16_t& currentDistance, const uint16_t& lastDistance) const {
	if (abs(currentDistance - lastDistance) < stabilityThreshold_) {
		//RPiIF::getInstance().send("Distance stable: True\n\r");
		return true;
	}
	//RPiIF::getInstance().send("Distance stable: False\n\r");
	return false;
}

// Used for a 5 min delay before restarting pump
// 360 seconds = (((65535 + 1) * 1024)/16e6) * x => x = 85.8; We need 86 overflows for a 5 min delay
ISR(TIMER4_OVF_vect) {
	BrugVarmtVand::overflowCount += 1;
}