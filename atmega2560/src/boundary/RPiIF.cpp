// Author: Christoffer H. Brix

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "RPiIF.h"

#define F_CPU 16000000UL	// Used by setbaud.h
#define BAUD  250000		// Used by setbaud.h
#include <util/setbaud.h>

RPiIF RPiIF::instance_(250000, 8);	// Eager initialization of static instance

RPiIF::RPiIF(uint32_t baudRate, uint8_t dataBits) : bufferHead_(0), bufferTail_(0) {
	initUART0(baudRate, dataBits, true, 0);	// Initialize UART with RX interrupt enabled and TX interrupt disabled
}

/*************************************************************************
	UART 0 initialization:
		Asynchronous mode.
		Baud rate = between 300 and 250000.
		Data bits = between 5 and 8.
		RX and TX enabled.
		Number of Stop Bits = 1.
		No Parity.
	PARAMETERS:
		baudRate: desired baud rate
		dataBits: desired data bits
		rxInterrupt: enable or disable RX interrupt
		txInterruptMode: choose TX interrupt mode (0 = disabled, 1 = interrupt on transmit buffer empty, 2 = interrupt on transmit buffer and shift register empty)
	NOTE:
		baudrate parameter is NOT used in this implementation, since I changed it to use setbaud.h

*************************************************************************/
void RPiIF::initUART0(const uint32_t baudRate, const uint8_t dataBits, const bool rxInterrupt, const uint8_t txInterruptMode) {	
	// Make sure we have valid parameters before we continue
	if ((baudRate >= 300 && baudRate <= 250000) && (dataBits >= 5 && dataBits <= 8)) {
		UBRR0H = UBRRH_VALUE;
		UBRR0L = UBRRL_VALUE;
		
		// If setbaud.h calculation determines double speed is necessary
		#if USE_2X
		UCSR0A |= (1 << U2X0);
		#else
		UCSR0A &= ~(1 << U2X0);
		#endif
		
		UCSR0C &= ~((1<<UCSZ00) | (1<<UCSZ01) | (1<<UCSZ02)); // Clears UCSZ00, UCSZ01 and UCSZ02 to make sure correct data bits are set (below)
		UCSR0C |= ((dataBits - 5)<<UCSZ00);					// Sets	UCSZ00, UCSZ01 and UCSZ02 to desired data bits
		
		UCSR0C &= ~((1<<UMSEL00) | (1<<UMSEL01));			// Sets UMSEL00 and UMSEL01 to 0, to enable asynchronous USART mode
		UCSR0C &= ~((1<<UPM00) | (1<<UPM01));				// Sets UPM00 and UPM01 to 0, to disable parity
		UCSR0C &= ~(1<<USBS0);								// Sets USBS0 to 0 for 1 stop-bit
		
		UCSR0B |= (1<<RXEN0);								// RX enabled
		UCSR0B |= (1<<TXEN0);								// TX enabled
		
		if (rxInterrupt) {
			UCSR0B |= (1<<RXCIE0);							// Set RXCIE0 to 1; enable RXC0 interrupt (RXC0 = 1 triggers an interrupt; we have data in the RX buffer)
		} else {
			UCSR0B &= ~(1<<RXCIE0);							// Set RXCIE0 to 0;  disable RXC0 interrupt
		}
		
		if (txInterruptMode == 1) {
			UCSR0B |= (1<<UDRIE0);							// Set UDRIE0 to 1; enable interrupt when the TX buffer is ready for more data (UDRE0 = 1 triggers an interrupt)
			UCSR0B &= ~(1<<TXCIE0);							// Set TXCIE0 to 0 to disable TXC0 interrupt
		} else if (txInterruptMode == 2) {
			UCSR0B |= (1<<TXCIE0);							// Set TXCIE0 to 1; enables interrupt when the shift register is empty + transmit buffer is empty
			UCSR0B &= ~(1<<UDRIE0);							// Set UDRIE0 to 1; disable transmit buffer empty interrupt
		} else {
			UCSR0B &= ~((1<<UDRIE0) | (1<<TXCIE0));			// Disable both TX interrupt types.
		}
		
		// Set UBRR0 for baud rate configuration
		// Formula: UBRR0 = (F_CPU / (16 * baudRate)) - 1
		// F_CPU is the clock frequency, 16 is the prescaler value for USART
		//UBRR0 = (F_CPU / (16 * baudRate)) - 1;
	}
}


/*************************************************************************
	This function writes a byte to the RX buffer. 
	If the buffer is full, the byte is discarded.
	It utilizes a circular buffer to store the bytes received from the UART.
	
	PARAMETERS:
		byte: Byte to write to the buffer.

	NOTE: 
		This function is called from the RX interrupt handler.
*************************************************************************/
void RPiIF::writeToRxBuffer(const uint8_t byte) {
	if (!isFull()) {
		rxBuffer_[bufferHead_] = byte;
		bufferHead_ = (bufferHead_ + 1) % kBufferSize;

		if (byte == STOP_ID_BYTE) {
			framesInBuffer_ += 1;
		}
	}
}

/*************************************************************************
	This function copies the data from the internal buffer to the outBuffer.
	If the frame is too large, the function discards the frame and resets, looking for a new frame.
	When the stop byte is found, the function returns the number of bytes in the frame.

	PARAMETERS:
		outBuffer: Pointer to the buffer where the data is copied to.

	RETURNS:
		Number of bytes in the frame.

	NOTE: 
		This needs more work. The implementation is funky atm.
*************************************************************************/
size_t RPiIF::getDataFrame(uint8_t* outBuffer) {
	size_t byteCount = 0;								// Number of bytes in the frame/outBuffer index
	bool startByteFound = false;

    // Iterate over the buffer to find a complete frame
    while (!isEmpty()) {
        uint8_t byte = rxBuffer_[bufferTail_];			// Get the byte at the tail of the buffer
        bufferTail_ = (bufferTail_ + 1) % kBufferSize;	// Increment the tail index

        if (startByteFound) {
			// If we find the stop byte, we return the number of bytes in the frame					
			if (byte == STOP_ID_BYTE && byteCount == kFrameSize) {
				framesInBuffer_ -= 1;
				return byteCount;
            }
            if (byteCount < kFrameSize) {
                outBuffer[byteCount++] = byte;  		// Copy byte to outBuffer and increment byte count
            } else {
                // Frame too large, discard and reset
                startByteFound = false;
                byteCount = 0;
            }
		// If we find the start byte, we reset the byte count and set the start byte flag
        } else if (byte == START_ID_BYTE) {
            startByteFound = true;
            byteCount = 0;
        }
    }
    return 0;	// Return 0 if no frame is found
}

// Send a byte
void RPiIF::send(const uint8_t byte) {
	while (!(UCSR0A & (1<<UDRE0)));			// Waits in loop until UDRE0 == 1
	UDR0 = byte;
}

// Send a char
void RPiIF::send(const char c) {
	while (!(UCSR0A & (1<<UDRE0)));			// Waits in loop until UDRE0 == 1
	UDR0 = c;
}

// Send a null terminated string
void RPiIF::send(const char* s) {
	while (*s != '\0') {	// Ensures the string is null-terminated
		send(*s++);
	}
}

// Send an array
void RPiIF::send(const uint8_t* p, const size_t length) {
	for (size_t i = 0; i < length; i++) {
		send(p[i]);
	}
}

// Send a long int
void RPiIF::send(const int32_t i) {
	char buffer[20];
	snprintf(buffer, sizeof(buffer), "%ld", i);
	send(buffer);
}

// Surprise, we can't print floats. Change this to do a scaled int version, print division and modulus as %d.%d 
void RPiIF::send(const float f) {
	char buffer[20];
	int32_t fScaled = (int32_t)(f * 10); 
	snprintf(buffer, sizeof(buffer), "%ld.%ld", fScaled / 10, fScaled % 10);
	send(buffer);
}

ISR(USART0_RX_vect) {
	uint8_t receivedByte = UDR0;							// Grab the byte from the RX buffer
	RPiIF::getInstance().writeToRxBuffer(receivedByte);		// Get the singleton instance and write the byte to ring buffer
}
