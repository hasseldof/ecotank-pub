#include <avr/io.h>
#include "switch.h"

// Set PORTA to input (switches on shield are connected to PORTA)
void initSwitchPort() {
	DDRA = 0;
}

// Return logic level of all switches - inverted reading on PINA is used since switches are logic low when pressed
unsigned char switchStatus() {
	return ~(PINA);
}

// Returns TRUE, if switch_nr is pressed, else false
unsigned char switchOn(unsigned char switch_nr) {
	return (~PINA & (1<<switch_nr));
}