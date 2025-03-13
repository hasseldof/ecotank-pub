#ifndef HEATINGUNITIF_H
#define HEATINGUNITIF_H

#include <avr/io.h>
#include <stdbool.h>

class __attribute__((packed)) HeatingUnitIF {
public:
	inline static void init() { DDRB |= (1<<DDB7) | (1<<DDB5); }
	inline static void start() { PORTB |= (1<<PB7) | (1<<PB5); }
	inline static void stop() { PORTB &= ~((1<<PB7) | (1<<PB5)); }
	
private:
	HeatingUnitIF() = delete;
	~HeatingUnitIF() = delete;
	HeatingUnitIF(const HeatingUnitIF &c) = delete;
	HeatingUnitIF& operator=(const HeatingUnitIF &c) = delete;
};

#endif //HEATINGUNITIF_H
