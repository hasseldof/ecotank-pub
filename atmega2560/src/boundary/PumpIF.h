#ifndef PUMPIF_H
#define PUMPIF_H

#include <avr/io.h>

// No implementation file exists for PumpIF. All details are in header file.

class __attribute__((packed)) PumpIF {
public:
	static void init() { DDRB |= (1<<DDB6); }			// Set PL0/pin D49 as output
	inline static void start() { PORTB |= (1<<PB6); }	// Turn off PL0/pin D49
	inline static void stop() { PORTB &= ~(1<<PB6); }	// Turn off PL0/pin D49
private:
	PumpIF() = delete;
	~PumpIF() = delete;
	PumpIF(const PumpIF &c) = delete;
	PumpIF& operator=(const PumpIF &c) = delete;		
};

#endif //PUMPIF_H
