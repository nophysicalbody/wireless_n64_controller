//#include <SoftwareSerial.h>

// Specify I/O connections
const int N64_SIG_PIN = 2;
const int CLK_PIN = 1;
const int RX_PIN = 3;
const int TX_PIN = 4;
const int LED_PIN = 0;

#define BAUDRATE_EXT 19200
#define NO_BITS_TO_READ_EXT 40 // 5 * 8 
const int serial_half_period_delay_us = 26; //1000000 / (BAUDRATE_EXT * 2);

volatile byte N64_CONTROLLER_STATE[4] = {0,0,0,0};

#define N64_NO_MESSAGE 0
#define N64_STATUS 1
#define N64_POLL 3

volatile byte consoleRequestMessage = N64_NO_MESSAGE;

ISR(INT0_vect)
{
  GIMSK = 0; // turn off all interrupts
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
        "breq reply             \n\t" // If timeout has occurred then jump to the reply section
        "sbic 0x16, 2           \n\t" // proceed only if the bit is cleared
        "rjmp pollForLow        \n\t" // If this bit is set then jump back and try again, otherwise continue
  
      "pollForHigh: inc r19   \n\t"
      // ***** potential lockup loop - use r19 and a branch instruction to skip to the end if it overflows
        "sbis 0x16, 2           \n\t" // If this bit is cleared the jump back and try again
        "rjmp pollForHigh       \n\t"
      "cpi r19, 4             \n\t" // Put our measured bitvalue in the carry flag
      "rol r24                \n\t"
      "rjmp readLoop          \n\t"

    // Change DDR so that pin is an output
    "reply: sbi 0x17, 2      \n\t"
    // Save the current state of the next 4 registers we need
    "push r20 \n\t"
    "push r21 \n\t"
    "push r22 \n\t"
    "push r23 \n\t"
    // Decide on our reply
    "cpi r24, 3    \n\t" // If we recieved "3" then this is a polling request
    "breq respondWithPoll \n\t"
    "cpi r24, 1           \n\t" // If we recieved "1" then this is a status request
    "breq respondWithStatus \n\t"
    "rjmp quit            \n\t" // If it's neither, just ignore.

     // Load r19-25 with appropriate values for the message we wish to send    
     "respondWithStatus:  ldi r19, 24      \n\t" // initialise loop count
     "ldi r20, 0b00000101     \n\t" // initialise byte 0
     "ldi r21, 0b00000000     \n\t" // initialise byte 1
     "ldi r22, 0b00000000     \n\t" // initialise byte 2
     "ldi r23, 0b00000000     \n\t" // initialise byte 3 [Not used for this case!]
     "rjmp start            \n\t"
     
     // Respond with controller state
     "respondWithPoll: ldi r19, 32      \n\t" // initialise loop count
     // Load controller state from supplied memory address
     "ld r20, %a1+     \n\t"
     "ld r21, %a1+     \n\t"
     "ld r22, %a1+     \n\t"
     "ld r23, %a1+      \n\t"

     "start:  cbi 0x18, 2      \n\t" // set IO low
     "rol r23  \n\t"   // "nop \n\t"
     "rol r22   \n\t"  // "nop \n\t"
     "rol r21  \n\t"   // "nop \n\t"
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

     // Check if we need to run again
     "end: dec r19        \n\t"
     "brne start         \n\t"

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

     // Return what request we recieved
     "mov %[output_byte], r24   \n\t"
     
     // Return registers used in ISR to their initial state
     "pop r23 \n\t"
     "pop r22 \n\t"
     "pop r21 \n\t"
     "pop r20 \n\t"
     "pop r24 \n\t"
     "pop r19 \n\t"
     "pop r18 \n\t"
     : [output_byte] "=d" (consoleRequestMessage)
     : "e" (&N64_CONTROLLER_STATE)
     : "r24"
  );
  // Clear INTF0, otherwise ISR will run again!
  GIFR = (1<<INTF0);
  GIMSK = 0b01100000; // turn interrupts back on
}

//SoftwareSerial Ser(0,4); // RX, TX

void setup()
{
  // Set up DDRs
  pinMode(N64_SIG_PIN, INPUT);
  pinMode(RX_PIN, INPUT);
  
  pinMode(CLK_PIN, OUTPUT);
  pinMode(TX_PIN, OUTPUT);
//  pinMode(LED_PIN, OUTPUT);

  // Set outputs to the correct initial state
  digitalWrite(LED_PIN, LOW);
  digitalWrite(CLK_PIN, HIGH);
  digitalWrite(TX_PIN, HIGH);

  asm volatile(
    // Set WatchDog Timer Control Register (0x21) to 0x0E to enable a 1 second watchdog timer that resets.
    "push r16     \n\t"
    "ldi r16, 0x0E \n\t"
    "out 0x21, r16 \n\t"
    "pop r16      \n\t"
  );
  
  // Configure pin change interrupts on pin 2/
  GIMSK |= (1 << INT0);   // external interrupt enable
  MCUCR = 0b10; // configure to trigger interrupt on falling edge
  sei();                  // enable interrupts
  
  //Ser.begin(19200);
}

byte incomingByte0;
byte incomingByte1;
byte incomingByte2;
byte incomingByte3;
byte checksumByte;

bool previous_rx_state = HIGH;
bool update_available = false;

byte synchronous_read_byte() {
  byte buff = 0x00;
  for (int b=0; b<8; b++){
      /* CLK LOW - signal change to transmitter */
      digitalWrite(CLK_PIN, LOW);
      delayMicroseconds(serial_half_period_delay_us);
      buff = buff << 1;
      
      /* CLK HIGH - read bit */
      digitalWrite(CLK_PIN, HIGH);
      buff |= digitalRead(RX_PIN);
      delayMicroseconds(serial_half_period_delay_us);
    }
  return buff;
}

void loop()
{
  // Reset Watchdog
  asm volatile("wdr \n\t");
  
  // Handle external commands:
  bool rx_state = digitalRead(RX_PIN);
  
  if (previous_rx_state && !rx_state) 
  {
    // We have detected a falling edge on the RX line, this indicates a transmit request.
    
    // Read into buffers:
    incomingByte0 = synchronous_read_byte();
    incomingByte1 = synchronous_read_byte();
    incomingByte2 = synchronous_read_byte();
    incomingByte3 = synchronous_read_byte();
    checksumByte  = synchronous_read_byte();
//
//    Ser.print(incomingByte0, HEX); Ser.print(", ");
//    Ser.print(incomingByte1, HEX); Ser.print(", ");
//    Ser.print(incomingByte2, HEX); Ser.print(", ");
//    Ser.print(incomingByte3, HEX); Ser.print(", ");
//    Ser.print(checksumByte, HEX); Ser.println(".");

    // Calculate checksum and set flag if we're ready to update:
    if (((incomingByte0 + incomingByte1 + incomingByte2 + incomingByte3) & 0xFF) == checksumByte) {
      update_available = true;
    } else {
      update_available = false;
//      digitalWrite(LED_PIN, HIGH); // Turn LED on when issue occurs
//      Ser.print(incomingByte0, HEX); Ser.print(F(", "));
//      Ser.print(incomingByte1, HEX); Ser.print(F(", "));
//      Ser.print(incomingByte2, HEX); Ser.print(F(", "));
//      Ser.print(incomingByte3, HEX); Ser.print(F(", "));
//      Ser.print(checksumByte, HEX); Ser.print(F(" == "));
//      Ser.print(((incomingByte0 + incomingByte1 + incomingByte2 + incomingByte3) & 0xFF), HEX); Ser.println("?");
    }
    //update_available = ((incomingByte0 + incomingByte1 + incomingByte2 + incomingByte3) & 0xFF) == checksumByte;
  }

  if (update_available) {
    // Turn off external interrupts (but allow pin change interrupts) - make changes to ISR variables here!
    GIMSK = 0b00100000; 
    N64_CONTROLLER_STATE[0] = incomingByte0;
    N64_CONTROLLER_STATE[1] = incomingByte1;
    N64_CONTROLLER_STATE[2] = incomingByte2;
    N64_CONTROLLER_STATE[3] = incomingByte3;
    GIMSK = 0b01100000; // turn external interrupts back on
    update_available = false;
  }
  previous_rx_state = rx_state;
}
