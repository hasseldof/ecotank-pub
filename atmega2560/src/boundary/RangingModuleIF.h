// Authors: Christoffer H. Brix
//			Thomas Leffers

#ifndef RANGINGMODULEIF_H
#define RANGINGMODULEIF_H

#include <avr/interrupt.h>
#include <stdbool.h>
#include <util/atomic.h>

// UltraSonic Range HCSR04

class __attribute__((packed)) RangingModuleIF {
public:
	inline static RangingModuleIF& getInstance() { return instance_; }	// Returns the static member instance
	inline bool hasUnreadDistance() const { return unreadDistance_; }	
	void ISRHandler();													// Function called by timer5 input capture ISR
	bool sendTriggerPulse();											// Called by DataManager; sends a 10�s pulse to HCSR04 trigger pin
	inline uint16_t getCurrentDistance() {								// Called by DataManager when updating SystemData
		unreadDistance_ = false;
		return currentDistance_;
	}
	inline uint16_t getLastDistance() const { return lastDistance_; }
	
	static volatile bool readingAllowed;								// Static variable; allows us to use it in our timer ISR directly. 
	
private:
	RangingModuleIF();													// Private since we create the singleton instance upon startup (in implementation file)
	~RangingModuleIF() = default;										// 
	RangingModuleIF(const RangingModuleIF &c) = delete;					// We don't use these, private and delete
	RangingModuleIF& operator=(const RangingModuleIF &c) = delete;		// We don't use these, private and delete
 
	void initRangingModule();											// Calls the below init functions
	void initTimer5();													// Sets the timer5 registers for correct operation
	void calcDistance();												// Converts startTime_ and stopTime_ to distance in millimeters
	void updateMovingAverage(const uint16_t distance);
	
	// This is used to hold the singleton instance of the RangingModuleIF.
	// We do eager initialization (startup, before main) instead of lazy (upon requesting the instance), 
	// because I couldn't get dynamic guards to work - Christoffer
	static RangingModuleIF instance_;
	volatile uint16_t currentDistance_;									// Holds the moving average value based on 5 measurements
	volatile uint16_t lastDistance_;									// lastDistance_ is set to currentDistance_ every 25 calls to updateMovingAverage
	volatile uint16_t startTime_;										// Set by timer5 interrupt logic, holds the TCNT value captured on rising edge of echo pulse
	volatile uint16_t endTime_;											// Set by timer5 interrupt logic, holds the TCNT value captured on falling edge of echo pulse
	volatile bool unreadDistance_;										// Used by DataManager; true when we have a new distance ready
};

#endif //RANGINGMODULEIF_H
