#include <Arduino.h>
#include "n64_console_if.h"

// Pin assignment is hardcoded in the instructions below. Forgive me, I know this is horrible.
// Below sets the N64 signal pin to (Arduino) pin 2 on an ATtiny85
#define SET_IO_LOW			"cbi 0x18, 2	\n\t"
#define SET_IO_HI			"sbi 0x18, 2	\n\t"
#define SET_IO_OUTPUT		"sbi 0x17, 2	\n\t"
#define SET_IO_INPUT		"cbi 0x17, 2	\n\t"
#define SKIP_NEXT_IF_IO_HI	"sbis 0x16, 2	\n\t"
#define SKIP_NEXT_IF_IO_LO	"sbic 0x16, 2	\n\t"


volatile byte n64_read() {
	volatile byte consoleRequestMessage;
	asm volatile(
		// Save the state of the registers we use
		"push r18						\n\t"
		"push r19						\n\t"
		"push r24						\n\t"

		// Start reading command from N64 console into r24
		"ldi r24, 0						\n\t"
		"ldi r19, 0						\n\t"
		// Wait for pin to be high (with some protections to prevent a lockup loop)
		"wait_high: "
			"inc r19					\n\t"
			"breq read_failure			\n\t"
			SKIP_NEXT_IF_IO_HI
			"rjmp wait_high				\n\t"

		// Enter read loop
		"read_loop: "
			"ldi r19, 0					\n\t"
			// initialise r18, if this value reaches zero, we are no longer recieving so we will jump to replying.
			"ldi r18, 6					\n\t"
			"poll_for_low: "
				"dec r18				\n\t"
				"breq read_success		\n\t" // If timeout has occurred then jump to the end section
				SKIP_NEXT_IF_IO_LO	// proceed only if the bit is cleared
				"rjmp poll_for_low		\n\t" // If this bit is set then jump back and try again, otherwise continue

			"poll_for_high: "
				"inc r19				\n\t"
				"breq read_failure		\n\t"
				SKIP_NEXT_IF_IO_HI
				"rjmp poll_for_high		\n\t"
				"cpi r19, 4				\n\t" // Put our measured bitvalue in the carry flag
				"rol r24				\n\t"
				"rjmp read_loop			\n\t"
		
		// Exit in case of failure - return 0xFF
		"read_failure: "
		"ldi r24, 255					\n\t"
		
		 // Return what request we recieved
		 "read_success: "
		 "mov %[output_byte], r24		\n\t"
		 
		 // Return registers used in ISR to their initial state
		 "pop r24						\n\t"
		 "pop r19						\n\t"
		 "pop r18						\n\t"
		 : [output_byte] "=d" (consoleRequestMessage)
		 :
		 : "r24"
	);
	return consoleRequestMessage;
}


void n64_send(volatile byte n64_message, volatile byte length_in_bytes) {
	asm volatile (
	// Change DDR so that pin is an output
		SET_IO_OUTPUT
		// Save the current state of the registers we need
		"push r18 \n\t"
		"push r19 \n\t"
		"push r20 \n\t"
		"push r21 \n\t"
		
		// R21 will contain the bytes remaining to be sent
		"ld r21, %a1								\n\t"
		"byte_loop_start: "
			"ldi r19, 8								\n\t" // number of bits in a byte
			// Load next byte to be sent into R20
			"ld r20, %a0+							\n\t"
			
			// Respond with controller state
			"bit_loop_start: "
				SET_IO_LOW
				"nop								\n\t"
				"nop								\n\t"
				"nop								\n\t"
				"nop								\n\t"
				//rotate left, placing 0th bit into carry flag
				"rol r20							\n\t" 
				//jump to 'send0' if bit was zero, otherwise continue
				"brcc send_0						\n\t" 
				"nop								\n\t"
				// send a "one"
				"send_1: "
					SET_IO_HI
					"ldi r18, 5						\n\t"
					"one_high_counter: "
						"dec r18					\n\t"
						"brne one_high_counter		\n\t"
						"nop						\n\t"
						"nop						\n\t"
						"rjmp bit_loop_end			\n\t"

				// send a "zero"
				"send_0: "
					"ldi r18, 5						\n\t"
					"nop							\n\t"
					"zero_low_counter: "
						"dec r18					\n\t"
						"brne zero_low_counter		\n\t"
						SET_IO_HI
						"nop						\n\t"
						"nop						\n\t"
						"nop						\n\t"

				// Check if we have finished sending the byte
				"bit_loop_end: "
				"dec r19							\n\t"
				"brne bit_loop_start				\n\t"
			 
			//Check if we have finished sending the whole message
			"dec r21								\n\t"
			"brne byte_loop_start					\n\t"

		// Send stop bit - a 2uS low pulse
		SET_IO_LOW
		"ldi r18, 5									\n\t"
		"stopBitLoop: dec r18						\n\t"
		"brne stopBitLoop							\n\t"
		SET_IO_HI
		 
		// Change DDR so that pin is an input again
		"quit: "
		SET_IO_INPUT

		// Return registers used in ISR to their initial state
		"pop r21									\n\t"
		"pop r20									\n\t"
		"pop r19									\n\t"
		"pop r18									\n\t"
		:: "e" (n64_message), "e" (&length_in_bytes)
		: "r24"
	);
}
