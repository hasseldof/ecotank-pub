// Authors: Christoffer H. Brix
//			Thomas Leffers

#include <avr/io.h>
#include <stddef.h>
#include <stdint.h>
#include <util/atomic.h>
#include <util/delay.h>
#include "RangingModuleIF.h"
#include "RPiIF.h"

RangingModuleIF RangingModuleIF::instance_;
volatile bool RangingModuleIF::readingAllowed = true;

// Default constructor
RangingModuleIF::RangingModuleIF() : currentDistance_(0), lastDistance_(0), startTime_(0), endTime_(0), unreadDistance_(false) {
	initRangingModule();
}

void RangingModuleIF::initRangingModule() {
	initTimer5();
	DDRL |= (1<<DDL3);					// Set PL3 to output; used to send the trigger pulse
}

// The timer is used to measure the time the echo pulse is high using input capture.
void RangingModuleIF::initTimer5() {
	DDRL &= ~(1<<DDL1);					// Set ICP5 pin to input (PL1)
	TCCR5A = 0;							// Set timer 5 to normal mode
	TCCR5B |= (1<<ICES5) | (1<<CS51);	// Input capture on rising edge, prescaler = 8
	TCCR5B |= (1<<ICNC5);				// Input capture noise filter enabled (4 cycles with same level before capturing timer value)
	TIMSK5 |= (1<<ICIE5);				// Enable input capture interrupt
}

// Added rate limit and atomic block
bool RangingModuleIF::sendTriggerPulse() {
	bool wasReadingAllowed;
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		wasReadingAllowed = readingAllowed;
		readingAllowed = false; 		// Will be set to true again after 2 timer5 overflows
	}
	
	if (wasReadingAllowed) {
		// 10 microsecond pulse
		PORTL |= (1<<PL3);
		_delay_us(10);
		PORTL &= ~(1<<PL3);
		
		TCNT5 = 0;						// Reset timer5 counting register
		TIMSK5 |= (1<<TOIE5);			// Enable timer5 overflow interrupt, for managing readingAllowed
		return true;
	}
	return false;
}

// Converts elapsed time to distance in millimeters and updates distance_ + sets unreadDistance_ flag
// Note that it also handles if a timer overflow has occurred between rising and falling edge of the echo pulse
// This will virtually never happen though, since we set TCNT5 = 0 when sending the trigger pulse,
// and the falling edge of the echo pulse should be well within the 32.8ms window
void RangingModuleIF::calcDistance() {
	uint16_t elapsed = 0;
	if (endTime_ >= startTime_) {		
		elapsed = endTime_ - startTime_;
	} else {
		elapsed = (UINT16_MAX - startTime_) + endTime_ + 1;
	}
	
	uint16_t distance = (elapsed * 5) / 58;
	updateMovingAverage(distance);
	unreadDistance_ = true;
}

// Simple moving average to clean our distance
// - Christoffer
void RangingModuleIF::updateMovingAverage(const uint16_t distance) {
	constexpr uint8_t kSMAWindowSize = 5;			// Small window for quicker response - can be adjusted
	static uint16_t smaBuffer[kSMAWindowSize];
	static uint32_t smaTotal = 0;
	static uint8_t smaIndex = 0;
	
	static uint8_t callCounter = 0;					// Counts calls to this function. Every 25 calls, we take a snapshot of currentDistance
	
	// Check if this is the first update
	if (smaTotal == 0 && smaIndex == 0) {
		// Initialize all buffer slots with the first reading
		for (uint8_t i = 0; i < kSMAWindowSize; i++) {
			smaBuffer[i] = distance;
		}
		smaTotal = distance * kSMAWindowSize;
	}

	// Update SMA
	smaTotal -= smaBuffer[smaIndex];				// Remove the oldest entry from the total
	smaBuffer[smaIndex] = distance;					// Add new reading to the buffer
	smaTotal += distance;							// Add new reading to the total
	smaIndex = (smaIndex + 1) % kSMAWindowSize;		// Move to the next buffer position (circular buffer)

	// Calculate current SMA
	currentDistance_ = smaTotal / kSMAWindowSize;
	
	// Every 25 calls, we take a snapshot of currentDistance.
	// Since we are capped to only reading every 63.6 ms, this should equal roughly about 1.5 seconds
	if (callCounter >= 25) {
		lastDistance_ = currentDistance_;
		callCounter = 0;
	} else {
		++callCounter;
	}
}

// ISRHandler function called by Timer5 input capture interrupt
void RangingModuleIF::ISRHandler() {
	static bool isRisingEdge = true;
	PORTB ^= (1<<PB2);
	if (isRisingEdge) {
		startTime_ = ICR5;				// Get the timer count captured
		TCCR5B &= ~(1<<ICES5);			// Switch to capture on falling edge
		isRisingEdge = false;
	} else {
		endTime_ = ICR5;				// Get the timer count captured
		TCCR5B |= (1<<ICES5);			// Switch back to capture on rising edge
		isRisingEdge = true;
		calcDistance();					// This could and should be moved outside the ISR logic, but it should be fine for our application
	}
}

// On echo pulse edge detection, call ISRHandler()
ISR(TIMER5_CAPT_vect) {
	RangingModuleIF::getInstance().ISRHandler(); 
}

// Handles when sendTriggerPulse is allowed to execute its logic - sets readingAllowed after 2 overflows = (32.8 ms * 2)
ISR(TIMER5_OVF_vect) {
	PORTB ^= (1<<PB1);
	static uint8_t overflows = 0;
	if (overflows >= 2) {
		TIMSK5 &= ~(1<<TOIE5);			// Disable timer5 overflow interrupt
		RangingModuleIF::readingAllowed = true;
		overflows = 0;
		return;
	}
	overflows++;
}
