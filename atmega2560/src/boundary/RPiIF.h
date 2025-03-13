// Author: Christoffer H. Brix

#ifndef RPIIF_H
#define RPIIF_H

#include <avr/io.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// The class is used to communicate with the Raspberry Pi via UART0.
class __attribute__((packed)) RPiIF {
public:
	// Enum of byte values used in communication
	enum Byte : uint8_t {
		START_ID_BYTE = 0x7E,
		STOP_ID_BYTE = 0x7D,
		SETPOINT_ID_BYTE = 0x5C,
		SYSTEM_POWER_ID_BYTE = 0x88,
		TEMPERATURE_ID_BYTE = 0x3A,
		WATER_LEVEL_ID_BYTE = 0x2F,
		DUMMY_BYTE = 0xFF
	};

	inline static RPiIF& getInstance() { return instance_; }			// Return reference to static instance
	inline uint8_t frameCount() const { return framesInBuffer_; }		// Return number of frames in the buffer
	void writeToRxBuffer(uint8_t byte);									// Write byte to the ring buffer
	size_t getDataFrame(uint8_t* outBuffer);							// Get a complete frame from the ring buffer
	
	// Generic overloads for sending data
	void send(const uint8_t byte);
	void send(const char c);
	void send(const char* s);
	void send(const uint8_t* p, size_t length); 	// This is what we use to send data to the RPi
	void send(const int32_t i);
	void send(const float f);

	static constexpr size_t kBufferSize = 256;		// Size of the ring buffer. Unecessarily large.
	static constexpr size_t kFrameSize = 7;			// Size of the data-frame in bytes excluding start and stop bytes
	
private:
	RPiIF(uint32_t baudRate = 250000, uint8_t dataBits = 8); // Baudrate isn't used here.
	~RPiIF() = default;
	RPiIF(const RPiIF &c) = delete;
	RPiIF& operator=(const RPiIF &c) = delete;

	
	void initUART0(const uint32_t baudRate,
		const uint8_t dataBits,
		const bool rxInt,
		const uint8_t txIntMode);
	inline bool isFull() const { return (bufferHead_ + 1) % kBufferSize == bufferTail_; }
	inline bool isEmpty() const { return bufferHead_ == bufferTail_; }

	static RPiIF instance_;					// Static instance of the class
	uint8_t rxBuffer_[kBufferSize];			// The ring buffer for incoming data
	size_t bufferHead_;  					// Write position in ring buffer
	size_t bufferTail_;  					// Read position in ring buffer
	volatile uint8_t framesInBuffer_;		// Number of frames in the buffer (counted by stop bytes)
};

#endif //RPIIF_H
