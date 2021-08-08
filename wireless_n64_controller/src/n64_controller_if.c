#include <Arduino.h>
#include "n64_controller_if.h"

// IO is hardcoded in the instructions below. Forgive me, this is horrible.
// Below sets the N64 signal pin to Arduino pin 4 on an Arduino Pro Mini
#define SET_IO_LOW		"cbi 0x0B, 4			\n\t"
#define SET_IO_HI		"sbi 0x0B, 4			\n\t"
#define SET_IO_OUTPUT	"sbi 0x0A, 4			\n\t"
#define SET_IO_INPUT	"cbi 0x0A, 4			\n\t"
#define TEST_IO_INPUT	"in r20, 0x09 \n\t andi r20, 0b00010000 \n\t"

#define ONE_US_DELAY	"nop \n\t nop \n\t nop \n\t nop \n\t nop \n\t nop \n\t nop \n\t nop \n\t"

unsigned long getControllerStatus(){
  byte b0; byte b1; byte b2;
    
  asm volatile(
    // Disable interrupts!
    "cli              \n\t" 
    
    // Ensure pin is configured as an output
    SET_IO_OUTPUT
    
    // Send 8 zeros
    "ldi r19, 8								\n\t"	// r19 is our bit counter
    "zero_bit_loop: "
		SET_IO_LOW
		"ldi r18, 7							\n\t" // r18 counts how long to stay low
		"nop								\n\t" // small extra delay
		"z_low_counter:"
			"dec r18						\n\t" // decrement counter
			"brne z_low_counter 			\n\t" // continue when counter is 0
		SET_IO_HI
		"ldi r18, 1							\n\t" // r18 counts how long to stay high
		"z_high_counter:"
			"dec r18						\n\t"
			"brne z_high_counter			\n\t"
		"dec r19							\n\t"
		"brne zero_bit_loop					\n\t"

    // Send 1 short length (3uS period) one
    "ldi r19, 1								\n\t"
	"one_bit_loop:"
		SET_IO_LOW
		"ldi r18, 2							\n\t"
		"one_low_counter: "
			"dec r18						\n\t"
			"brne one_low_counter			\n\t"
		SET_IO_HI
		"ldi r18, 2							\n\t"
		"one_high_counter: "
			"dec r18						\n\t"
			"brne one_high_counter			\n\t"
		"dec r19							\n\t"
		"nop								\n\t"
		"brne one_bit_loop					\n\t"

    // Now change DDR so the pin is an input (logic 0)
    SET_IO_INPUT

    // Read first 8 bits into R23
    //  < Measure duration of hi pulse to determine 0 or 1 >
    "ldi r23, 0								\n\t" // r23 will contain our first byte
    "ldi r18, 8								\n\t" // As usual, r18 is our bit counter
	"byte_1_read_loop: "
		"ldi r22, 0							\n\t" // Initialise loop count to 0
		"byte_1_lo_waitloop: "
			TEST_IO_INPUT // Poll for low
			"brne byte_1_lo_waitloop 		\n\t"
		"byte_1_hi_count_loop: "
			"inc r22						\n\t"
			TEST_IO_INPUT // Now, poll for high
			"breq byte_1_hi_count_loop		\n\t"
		// Read bit by storing it in the carry flag
		"cpi r22, 4							\n\t" // Sets carry flag if 4 > r22
		"rol r23							\n\t" // Shift register left & put carry flag in bit 0
		"dec r18							\n\t"
		"brne byte_1_read_loop				\n\t"
		
	// Read second 8 bits into R24
    //  < Measure duration of hi pulse to determine 0 or 1 >
    "ldi r24, 0								\n\t"
    "ldi r18, 8								\n\t"
	"byte_2_read_loop: "
		"ldi r22, 0							\n\t"
		"byte_2_lo_waitloop: "
			TEST_IO_INPUT // Poll for low
			"brne byte_2_lo_waitloop 		\n\t"
		"byte_2_hi_count_loop: "
			"inc r22						\n\t"
			TEST_IO_INPUT // Now, poll for high
			"breq byte_2_hi_count_loop		\n\t"
		// Read bit by storing it in the carry flag
		"cpi r22, 4							\n\t"
		"rol r24							\n\t"
		"dec r18							\n\t"
		"brne byte_2_read_loop				\n\t"

    //// Read third 8 bits into R25
    "ldi r25, 0								\n\t"
    "ldi r18, 8								\n\t"
	"byte_3_read_loop: "
		"ldi r22, 0							\n\t"
		"byte_3_lo_waitloop: "
			TEST_IO_INPUT // Poll for low
			"brne byte_3_lo_waitloop 		\n\t"
		"byte_3_hi_count_loop: "
			"inc r22						\n\t"
			TEST_IO_INPUT // Now, poll for high
			"breq byte_3_hi_count_loop		\n\t"
		// Read bit by storing it in the carry flag
		"cpi r22, 4							\n\t"
		"rol r25							\n\t"
		"dec r18							\n\t"
		"brne byte_3_read_loop				\n\t"
      
    //// Wait for final bit to complete
    ONE_US_DELAY ONE_US_DELAY ONE_US_DELAY ONE_US_DELAY

    // Change DDR so that pin is an output again
    SET_IO_OUTPUT

	"sei									\n\t" // enable interrupts!

    // Take the byte values and transfer them to the c variables
    "mov %[output_byte_0], r23    \n\t"
    "mov %[output_byte_1], r24    \n\t"
    "mov %[output_byte_2], r25    \n\t"
    
    : [output_byte_0] "=r" (b0) , 
      [output_byte_1] "=r" (b1) ,
      [output_byte_2] "=r" (b2)
  );

  // Now combine the bytes into a long to return.
  unsigned long controllerConfig = 0;
  controllerConfig |= b0;
  controllerConfig <<= 8;
  controllerConfig |= b1;
  controllerConfig <<= 8;
  controllerConfig |= b2;
  
  return controllerConfig;
}

unsigned long pollController(){
  byte b0; byte b1; byte b2; byte b3;
  asm volatile(
	// Disable interrupts!
    "cli              \n\t" 
    
    // Ensure pin is configured as an output
    SET_IO_OUTPUT
    
    // Send 7 zeros
    "ldi r19, 7								\n\t"	// r19 is our bit counter
    "zero_bit_loop: "
		SET_IO_LOW
		"ldi r18, 7							\n\t" // r18 counts how long to stay low
		"nop								\n\t" // small extra delay
		"z_low_counter:"
			"dec r18						\n\t" // decrement counter
			"brne z_low_counter 			\n\t" // continue when counter is 0
		SET_IO_HI
		"ldi r18, 1							\n\t" // r18 counts how long to stay high
		"z_high_counter:"
			"dec r18						\n\t"
			"brne z_high_counter			\n\t"
		"dec r19							\n\t"
		"brne zero_bit_loop					\n\t"

    // Send 1 normal length (4uS period) one
    "ldi r19, 1								\n\t"
	"norm_one_bit_loop: "
		SET_IO_LOW
		"ldi r18, 2							\n\t"
		"norm_one_low_counter:"
			"dec r18						\n\t"
			"brne norm_one_low_counter		\n\t"
		SET_IO_HI
		"ldi r18, 6							\n\t"
		"norm_one_hi_counter:"
			"dec r18						\n\t"
			"brne norm_one_hi_counter       \n\t"
		"dec r19							\n\t"
		"nop								\n\t"
		"brne norm_one_bit_loop				\n\t"

    // Send 1 short length (3uS period) one
    "ldi r19, 1								\n\t"
	"short_one_bit_loop:"
		SET_IO_LOW
		"ldi r18, 2							\n\t"
		"short_one_low_counter: "
			"dec r18						\n\t"
			"brne short_one_low_counter		\n\t"
		SET_IO_HI
		"ldi r18, 2							\n\t"
		"short_one_high_counter: "
			"dec r18						\n\t"
			"brne short_one_high_counter	\n\t"
		"dec r19							\n\t"
		"nop								\n\t"
		"brne short_one_bit_loop			\n\t"

    // Now change DDR so the pin is an input (logic 0)
    SET_IO_INPUT

    // Read first 8 bits into R23
    //  < Measure duration of hi pulse to determine 0 or 1 >
    "ldi r23, 0								\n\t" // r23 will contain our first byte
    "ldi r18, 8								\n\t" // As usual, r18 is our bit counter
	"byte_1_read_loop: "
		"ldi r22, 0							\n\t" // Initialise loop count to 0
		"byte_1_lo_waitloop: "
			TEST_IO_INPUT // Poll for low
			"brne byte_1_lo_waitloop 		\n\t"
		"byte_1_hi_count_loop: "
			"inc r22						\n\t"
			TEST_IO_INPUT // Now, poll for high
			"breq byte_1_hi_count_loop		\n\t"
		// Read bit by storing it in the carry flag
		"cpi r22, 4							\n\t" // Sets carry flag if 4 > r22
		"rol r23							\n\t" // Shift register left & put carry flag in bit 0
		"dec r18							\n\t"
		"brne byte_1_read_loop				\n\t"
		
	// Read second 8 bits into R24
    //  < Measure duration of hi pulse to determine 0 or 1 >
    "ldi r24, 0								\n\t"
    "ldi r18, 8								\n\t"
	"byte_2_read_loop: "
		"ldi r22, 0							\n\t"
		"byte_2_lo_waitloop: "
			TEST_IO_INPUT // Poll for low
			"brne byte_2_lo_waitloop 		\n\t"
		"byte_2_hi_count_loop: "
			"inc r22						\n\t"
			TEST_IO_INPUT // Now, poll for high
			"breq byte_2_hi_count_loop		\n\t"
		// Read bit by storing it in the carry flag
		"cpi r22, 4							\n\t"
		"rol r24							\n\t"
		"dec r18							\n\t"
		"brne byte_2_read_loop				\n\t"

    //// Read third 8 bits into R25
    "ldi r25, 0								\n\t"
    "ldi r18, 8								\n\t"
	"byte_3_read_loop: "
		"ldi r22, 0							\n\t"
		"byte_3_lo_waitloop: "
			TEST_IO_INPUT // Poll for low
			"brne byte_3_lo_waitloop 		\n\t"
		"byte_3_hi_count_loop: "
			"inc r22						\n\t"
			TEST_IO_INPUT // Now, poll for high
			"breq byte_3_hi_count_loop		\n\t"
		// Read bit by storing it in the carry flag
		"cpi r22, 4							\n\t"
		"rol r25							\n\t"
		"dec r18							\n\t"
		"brne byte_3_read_loop				\n\t"

    //// Read fourth 8 bits into R26
    "ldi r26, 0								\n\t"
    "ldi r18, 8								\n\t"
	"byte_4_read_loop: "
		"ldi r22, 0							\n\t"
		"byte_4_lo_waitloop: "
			TEST_IO_INPUT // Poll for low
			"brne byte_4_lo_waitloop 		\n\t"
		"byte_4_hi_count_loop: "
			"inc r22						\n\t"
			TEST_IO_INPUT // Now, poll for high
			"breq byte_4_hi_count_loop		\n\t"
		// Read bit by storing it in the carry flag
		"cpi r22, 4							\n\t"
		"rol r26							\n\t"
		"dec r18							\n\t"
		"brne byte_4_read_loop				\n\t"
      
    // Wait for final bit to complete
    ONE_US_DELAY ONE_US_DELAY ONE_US_DELAY ONE_US_DELAY

    // Change DDR so that pin is an output again
    SET_IO_OUTPUT

	"sei									\n\t" // enable interrupts!

    // Take the byte values and transfer them to the c variables
    "mov %[output_byte_0], r23				\n\t"
    "mov %[output_byte_1], r24				\n\t"
    "mov %[output_byte_2], r25				\n\t"
    "mov %[output_byte_3], r26				\n\t"
    
    //"sei              \n\t" // enable interrupts!
    : [output_byte_0] "=d" (b0) , 
      [output_byte_1] "=d" (b1) ,
      [output_byte_2] "=d" (b2) ,
      [output_byte_3] "=d" (b3) :
  );

  // Now combine the bytes into a long to return.
  unsigned long N64state = 0;
  N64state |= b0;
  N64state <<= 8;
  N64state |= b1;
  N64state <<= 8;
  N64state |= b2;
  N64state <<= 8;
  N64state |= b3;
  
  return N64state;
}
