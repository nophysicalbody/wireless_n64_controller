#include <Arduino.h>
#include "n64_console_if.h"

volatile byte n64_read() {
	volatile byte consoleRequestMessage;
	asm volatile(
		// Save the state of the first 3 registers we use
		"push r18 \n\t"
		"push r19 \n\t"
		"push r24 \n\t"
		
		// Start reading command from N64 console into r24
		"ldi r24, 0               \n\t"
		// Wait for pin to be high
		// ***** potential lockup loop
		"waitHigh: sbis 0x16, 2   \n\t"
		"rjmp waitHigh            \n\t"

		// Enter read loop
		"readLoop: ldi r19, 0             \n\t"
		  // initialise r18, if this value reaches zero, we are no longer recieving so we will jump to replying.
		  "ldi r18, 6              \n\t"
		  "pollForLow: dec r18    \n\t"
			"breq end1             \n\t" // If timeout has occurred then jump to the end section
			"sbic 0x16, 2           \n\t" // proceed only if the bit is cleared
			"rjmp pollForLow        \n\t" // If this bit is set then jump back and try again, otherwise continue
	  
		  "pollForHigh: inc r19   \n\t"
		  // ***** potential lockup loop - use r19 and a branch instruction to skip to the end if it overflows
			"sbis 0x16, 2           \n\t" // If this bit is cleared the jump back and try again
			"rjmp pollForHigh       \n\t"
		  "cpi r19, 4             \n\t" // Put our measured bitvalue in the carry flag
		  "rol r24                \n\t"
		  "rjmp readLoop          \n\t"
		 // Return what request we recieved
		 "end1: mov %[output_byte], r24   \n\t"
		 
		 // Return registers used in ISR to their initial state
		 "pop r24 \n\t"
		 "pop r19 \n\t"
		 "pop r18 \n\t"
		 : [output_byte] "=d" (consoleRequestMessage)
		 :
		 : "r24"
  );
  return consoleRequestMessage;
}


//void respond_to_poll_command(volatile byte N64_CONTROLLER_STATE) {
	
//}

void n64_send(volatile byte n64_message, volatile byte length_in_bytes) {
	asm volatile (
	// Change DDR so that pin is an output
		"sbi 0x17, 2      \n\t"
		// Save the current state of the next 4 registers we need
		"push r18 \n\t"
		"push r19 \n\t"
		"push r20 \n\t"
		"push r21 \n\t"
		"push r22 \n\t"
		"push r23 \n\t"
		
		// R21 will contain the bytes remaining to be sent
		"ld r21, %a1		\n\t"
		"byte_loop_start: ldi r19, 8		\n\t" // number of bits in a byte
		// Load next byte to be sent into R20
		"ld r20, %a0+     \n\t"
		
		//// Respond with controller state
		 //"respondWithPoll: ldi r19, 24      \n\t" // initialise loop count - 24 bits to send
		 //// Load controller state from supplied memory address
		 //"ld r20, %a0+     \n\t"
		 //"ld r21, %a0+     \n\t"
		 //"ld r22, %a0+     \n\t"
		 //"ld r23, %a0+      \n\t"

		 "bit_loop_start:  cbi 0x18, 2      \n\t" // set IO low
		 "nop \n\t" // "rol r23  \n\t"   // "nop \n\t"
		 "nop \n\t"
		 "nop \n\t" //"rol r22   \n\t"  // "nop \n\t"
		 "nop \n\t"
		 "rol r20          \n\t" //rotate left, placing 0th bit into carry flag
		 "brcc send0       \n\t" //jump to 'send0' if bit was zero, otherwise continue
		 "nop \n\t"
		 // send a "one"
		   "sbi 0x18, 2      \n\t"
		   "ldi r18, 5       \n\t"
		   "zhloop: dec r18   \n\t"
		   "brne zhloop       \n\t"
		 "nop  \n\t"
		 "nop  \n\t"
		 "rjmp end  \n\t"

		 // send a "zero"
		 "send0: ldi r18, 5       \n\t"
		   "nop              \n\t"
		   "olloop: dec r18    \n\t"
		   "brne olloop        \n\t"
		   "sbi 0x18, 2      \n\t"
		   "nop \n\t"
		   "nop \n\t"
		   "nop \n\t"

		 // Check if we have finished sending the byte
		 "end: dec r19        \n\t"
		 "brne bit_loop_start         \n\t"
		 
		 //Check if we have finished sending the whole message
		 "dec r21	\n\t"
		 "brne byte_loop_start		\n\t"

		 // Send stop bit - a 2uS low pulse
		 "nop \n\t"
		 "nop \n\t"
		 "nop \n\t"
		 "cbi 0x18, 2      \n\t" // set IO low
		 "ldi r18, 5      \n\t"
		 "stopBitLoop: dec r18         \n\t"
		 "brne stopBitLoop              \n\t"
		 "sbi 0x18, 2      \n\t"
		 
		 // Change DDR so that pin is an input again
		 "quit: cbi 0x17, 2      \n\t"

		 // Return registers used in ISR to their initial state
		 "pop r23 \n\t"
		 "pop r22 \n\t"
		 "pop r21 \n\t"
		 "pop r20 \n\t"
		 "pop r19 \n\t"
		 "pop r18 \n\t"
		 :: "e" (n64_message), "e" (&length_in_bytes)
		 : "r24"
  );
}

//void n64_send(volatile byte n64_message, volatile byte length_in_bytes) {
	//asm volatile (
	//// Change DDR so that pin is an output
		//"sbi 0x17, 2      \n\t"
		//// Save the current state of the next 4 registers we need
		//"push r18 \n\t"
		//"push r19 \n\t"
		//"push r20 \n\t"
		//"push r21 \n\t"
		//"push r22 \n\t"
		//"push r23 \n\t"
		
		//// Respond with controller state
		 //"respondWithPoll: ldi r19, 24      \n\t" // initialise loop count - 24 bits to send
		 //// Load controller state from supplied memory address
		 //"ld r20, %a0+     \n\t"
		 //"ld r21, %a0+     \n\t"
		 //"ld r22, %a0+     \n\t"
		 //"ld r23, %a0+      \n\t"

		 //"start:  cbi 0x18, 2      \n\t" // set IO low
		 //"rol r23  \n\t"   // "nop \n\t"
		 //"nop \n\t"
		 //"rol r22   \n\t"  // "nop \n\t"
		 //"rol r21  \n\t"   // "nop \n\t"
		 //"rol r20          \n\t" //rotate left, placing 0th bit into carry flag
		 //"brcc send0       \n\t" //jump to 'send0' if bit was zero, otherwise continue
		 //"nop \n\t"
		 //// send a "one"
		   //"sbi 0x18, 2      \n\t"
		   //"ldi r18, 5       \n\t"
		   //"zhloop: dec r18   \n\t"
		   //"brne zhloop       \n\t"
		 //"nop  \n\t"
		 //"nop  \n\t"
		 //"rjmp end  \n\t"

		 //// send a "zero"
		 //"send0: ldi r18, 5       \n\t"
		   //"nop              \n\t"
		   //"olloop: dec r18    \n\t"
		   //"brne olloop        \n\t"
		   //"sbi 0x18, 2      \n\t"
		   //"nop \n\t"
		   //"nop \n\t"
		   //"nop \n\t"

		 //// Check if we need to run again
		 //"end: dec r19        \n\t"
		 //"brne start         \n\t"

		 //// Send stop bit - a 2uS low pulse
		 //"nop \n\t"
		 //"nop \n\t"
		 //"nop \n\t"
		 //"cbi 0x18, 2      \n\t" // set IO low
		 //"ldi r18, 5      \n\t"
		 //"stopBitLoop: dec r18         \n\t"
		 //"brne stopBitLoop              \n\t"
		 //"sbi 0x18, 2      \n\t"
		 
		 //// Change DDR so that pin is an input again
		 //"quit: cbi 0x17, 2      \n\t"

		 //// Return registers used in ISR to their initial state
		 //"pop r23 \n\t"
		 //"pop r22 \n\t"
		 //"pop r21 \n\t"
		 //"pop r20 \n\t"
		 //"pop r19 \n\t"
		 //"pop r18 \n\t"
		 //:: "e" (n64_message), "r" (length_in_bytes)
		 //: "r24"
  //);
//}

//void handle_command_from_console(volatile byte *N64_CONTROLLER_STATE) {
	//volatile byte consoleRequestMessage;
	//asm volatile(
		//// Save the state of the first 3 registers we use
		//"push r18 \n\t"
		//"push r19 \n\t"
		//"push r24 \n\t"
		
		//// Start reading command from N64 console into r24
		//"ldi r24, 0               \n\t"
		//// Wait for pin to be high
		//// ***** potential lockup loop
		//"waitHigh: sbis 0x16, 2   \n\t"
		//"rjmp waitHigh            \n\t"

		//// Enter read loop
		//"readLoop: ldi r19, 0             \n\t"
		  //// initialise r18, if this value reaches zero, we are no longer recieving so we will jump to replying.
		  //"ldi r18, 6              \n\t"
		  //"pollForLow: dec r18    \n\t"
			//"breq reply             \n\t" // If timeout has occurred then jump to the reply section
			//"sbic 0x16, 2           \n\t" // proceed only if the bit is cleared
			//"rjmp pollForLow        \n\t" // If this bit is set then jump back and try again, otherwise continue
	  
		  //"pollForHigh: inc r19   \n\t"
		  //// ***** potential lockup loop - use r19 and a branch instruction to skip to the end if it overflows
			//"sbis 0x16, 2           \n\t" // If this bit is cleared the jump back and try again
			//"rjmp pollForHigh       \n\t"
		  //"cpi r19, 4             \n\t" // Put our measured bitvalue in the carry flag
		  //"rol r24                \n\t"
		  //"rjmp readLoop          \n\t"

		//// Change DDR so that pin is an output
		//"reply: sbi 0x17, 2      \n\t"
		//// Save the current state of the next 4 registers we need
		//"push r20 \n\t"
		//"push r21 \n\t"
		//"push r22 \n\t"
		//"push r23 \n\t"
		//// Decide on our reply
		//"cpi r24, 3    \n\t" // If we recieved "3" then this is a polling request
		//"breq respondWithPoll \n\t"
		//"cpi r24, 1           \n\t" // If we recieved "1" then this is a status request
		//"breq respondWithStatus \n\t"
		//"rjmp quit            \n\t" // If it's neither, just ignore.

		 //// Load r19-25 with appropriate values for the message we wish to send    
		 //"respondWithStatus:  ldi r19, 24      \n\t" // initialise loop count
		 //"ldi r20, 0b00000101     \n\t" // initialise byte 0
		 //"ldi r21, 0b00000000     \n\t" // initialise byte 1
		 //"ldi r22, 0b00000000     \n\t" // initialise byte 2
		 //"ldi r23, 0b00000000     \n\t" // initialise byte 3 [Not used for this case!]
		 //"rjmp start            \n\t"
		 
		 //// Respond with controller state
		 //"respondWithPoll: ldi r19, 32      \n\t" // initialise loop count
		 //// Load controller state from supplied memory address
		 //"ld r20, %a1+     \n\t"
		 //"ld r21, %a1+     \n\t"
		 //"ld r22, %a1+     \n\t"
		 //"ld r23, %a1+      \n\t"

		 //"start:  cbi 0x18, 2      \n\t" // set IO low
		 //"rol r23  \n\t"   // "nop \n\t"
		 //"rol r22   \n\t"  // "nop \n\t"
		 //"rol r21  \n\t"   // "nop \n\t"
		 //"rol r20          \n\t" //rotate left, placing 0th bit into carry flag
		 //"brcc send0       \n\t" //jump to 'send0' if bit was zero, otherwise continue
		 //"nop \n\t"
		 //// send a "one"
		   //"sbi 0x18, 2      \n\t"
		   //"ldi r18, 5       \n\t"
		   //"zhloop: dec r18   \n\t"
		   //"brne zhloop       \n\t"
		 //"nop  \n\t"
		 //"nop  \n\t"
		 //"rjmp end  \n\t"

		 //// send a "zero"
		 //"send0: ldi r18, 5       \n\t"
		   //"nop              \n\t"
		   //"olloop: dec r18    \n\t"
		   //"brne olloop        \n\t"
		   //"sbi 0x18, 2      \n\t"
		   //"nop \n\t"
		   //"nop \n\t"
		   //"nop \n\t"

		 //// Check if we need to run again
		 //"end: dec r19        \n\t"
		 //"brne start         \n\t"

		 //// Send stop bit - a 2uS low pulse
		 //"nop \n\t"
		 //"nop \n\t"
		 //"nop \n\t"
		 //"cbi 0x18, 2      \n\t" // set IO low
		 //"ldi r18, 5      \n\t"
		 //"stopBitLoop: dec r18         \n\t"
		 //"brne stopBitLoop              \n\t"
		 //"sbi 0x18, 2      \n\t"
		 
		 //// Change DDR so that pin is an input again
		 //"quit: cbi 0x17, 2      \n\t"

		 //// Return what request we recieved
		 //"mov %[output_byte], r24   \n\t"
		 
		 //// Return registers used in ISR to their initial state
		 //"pop r23 \n\t"
		 //"pop r22 \n\t"
		 //"pop r21 \n\t"
		 //"pop r20 \n\t"
		 //"pop r24 \n\t"
		 //"pop r19 \n\t"
		 //"pop r18 \n\t"
		 //: [output_byte] "=d" (consoleRequestMessage)
		 //: "e" (&N64_CONTROLLER_STATE)
		 //: "r24"
  //);
//}
