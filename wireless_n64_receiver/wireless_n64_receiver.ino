
#define A_IDX       0x80000000
#define B_IDX       0x40000000
#define Z_IDX       0x20000000
#define START_IDX   0x10000000
#define D_UP_IDX    0x08000000
#define D_DOWN_IDX  0x04000000
#define D_LEFT_IDX  0x02000000
#define D_RIGHT_IDX 0x01000000
#define L_IDX       0x00400000
#define R_IDX       0x00200000
#define C_UP_IDX    0x00100000
#define C_DOWN_IDX  0x00080000
#define C_LEFT_IDX  0x00040000
#define C_RIGHT_IDX 0x00020000
#define X_IDX       0x0000FF00
#define Y_IDX       0x000000FF

// Include the RFM69 and SPI libraries:
#include <RFM69.h>
#include <SPI.h>

// Addresses for this node. CHANGE THESE FOR EACH NODE!
#define NETWORKID     0   // Must be the same for all nodes
#define MYNODEID      2   // My node ID
#define TONODEID      1   // Destination node ID

// RFM69 frequency, uncomment the frequency of your module:
#define FREQUENCY     RF69_915MHZ

// AES encryption (or not):
#define ENCRYPT       true // Set to "true" to use encryption
#define ENCRYPTKEY    "TOPSECRETPASSWRD" // Use the same 16-byte key on all nodes

#define GN_LED 5
#define RD_LED 4
#define N64_CONSOLE_PIN 3 // If you need to change this to another pin, you also need to update the assembly code (sorry!)

// Create a library object for our RFM69HCW module:
RFM69 radio;

#define N64_NO_MESSAGE 0
#define N64_STATUS 1
#define N64_POLL 3
volatile byte N64_CONTROLLER_STATE[4] = {0xDE,0xEF,0xFE,0xED};
volatile byte consoleRequestMessage = N64_NO_MESSAGE;

void n64_isr()
{
  // Interrupt that is triggered by a falling edge on the N64 signal line
  //GIMSK = 0; // turn off all interrupts
  asm volatile(
    // Save the state of the first 3 registers we use
    "push r18 \n\t"
    "push r19 \n\t"
    "push r24 \n\t"
    
    // Start reading command from N64 console into r24
    "ldi r24, 0               \n\t"
    // Wait for pin to be high
    //"waitHigh: sbis 0x16, 2   \n\t"
    "waitHigh: sbis 0x09, 3   \n\t"
    "rjmp waitHigh            \n\t"

    // Enter read loop
    "readLoop: ldi r19, 0             \n\t"
      // initialise r18, if this value reaches zero, we are no longer recieving so we will jump to replying.
      "ldi r18, 6              \n\t"
      "pollForLow: dec r18    \n\t"
        "breq reply             \n\t" // If timeout has occurred then jump to the reply section
        //"sbic 0x16, 2           \n\t" // proceed only if the bit is cleared
        "sbic 0x09, 3           \n\t"
        "rjmp pollForLow        \n\t" // If this bit is set then jump back and try again, otherwise continue
  
      "pollForHigh: inc r19   \n\t"
        //"sbis 0x16, 2           \n\t" // If this bit is cleared the jump back and try again
        "sbis 0x09, 3           \n\t"
        "rjmp pollForHigh       \n\t"
      "cpi r19, 4             \n\t" // Put our measured bitvalue in the carry flag
      "rol r24                \n\t"
      "rjmp readLoop          \n\t"

    // Change DDR so that pin is an output
    //"reply: sbi 0x17, 2      \n\t"
    "reply: sbi 0x0A, 3      \n\t"
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

     //"start:  cbi 0x18, 2      \n\t" // set IO low
     "start:  cbi 0x0B, 3      \n\t" // set IO low
     "rol r23  \n\t"   // "nop \n\t"
     "rol r22   \n\t"  // "nop \n\t"
     "rol r21  \n\t"   // "nop \n\t"
     "rol r20          \n\t" //rotate left, placing 0th bit into carry flag
     "brcc send0       \n\t" //jump to 'send0' if bit was zero, otherwise continue
     "nop \n\t"
     // send a "one"
       //"sbi 0x18, 2      \n\t"
       "sbi 0x0B, 3      \n\t"
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
       //"sbi 0x18, 2      \n\t"
       "sbi 0x0B, 3      \n\t"
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
     //"cbi 0x18, 2      \n\t" // set IO low
     "cbi 0x0B, 3      \n\t" // set IO low
     "ldi r18, 5      \n\t"
     "stopBitLoop: dec r18         \n\t"
     "brne stopBitLoop              \n\t"
     //"sbi 0x18, 2      \n\t"
     "sbi 0x0B, 3      \n\t"
     
     // Change DDR so that pin is an input again
     //"quit: cbi 0x17, 2      \n\t"
     "quit: cbi 0x0A, 3      \n\t"

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
  //GIFR = (1<<INTF0);
  //GIMSK = 0b01100000; // turn interrupts back on
}

void turnOnLight(){
  digitalWrite(GN_LED, HIGH);
}

void setup()
{
  pinMode(N64_CONSOLE_PIN, INPUT); //this pin is connected to the console.
  //digitalWrite(N64_CONSOLE_PIN, HIGH);
  attachInterrupt(digitalPinToInterrupt(N64_CONSOLE_PIN), n64_isr, FALLING);
  //attachInterrupt(digitalPinToInterrupt(N64_CONSOLE_PIN), turnOnLight, FALLING);
  // Open a serial port so we can send keystrokes to the module:

  Serial.begin(9600);
  Serial.print("Node ");
  Serial.print(MYNODEID,DEC);
  Serial.println(" ready");  

  // Initialize the RFM69HCW:
  radio.initialize(FREQUENCY, MYNODEID, NETWORKID);
  radio.setHighPower(); // Always use this for RFM69HCW
  // Turn on encryption if desired:
  if (ENCRYPT)
    radio.encrypt(ENCRYPTKEY);

  // set up indicators
  pinMode(GN_LED, OUTPUT);
  pinMode(RD_LED, OUTPUT);
}

volatile byte n64_status_buffer[4];
unsigned long n64_status = 0;

void loop()
{
  //delay(100);
  if (radio.receiveDone() && (radio.DATALEN==4)) // Got a 4 byte n64 status
  {

    // Read out the information:
    for (byte i = 0; i < 4; i++){
      n64_status_buffer[i] = radio.DATA[i];
    }

    // Update values used by ISR
    noInterrupts();
    N64_CONTROLLER_STATE[0] = n64_status_buffer[0];
    N64_CONTROLLER_STATE[1] = n64_status_buffer[1];
    N64_CONTROLLER_STATE[2] = n64_status_buffer[2];
    N64_CONTROLLER_STATE[3] = n64_status_buffer[3];
    interrupts();

    n64_status = 0;
    // Read out the information:
    for (byte i = 0; i < 4; i++){
      n64_status = n64_status << 8;
      n64_status += radio.DATA[i];
    }
    // If the B button is pressed, light the green indicator
    digitalWrite(GN_LED, bool(!(n64_status & A_IDX)));

    // If the Start button is pressed, light the red indicator
    digitalWrite(RD_LED, bool(n64_status & START_IDX));

    // Pass the update synchronously to the virtual n64 controller
    //write_32_bit_controller_value(n64_status);
    //Serial.println(n64_status, HEX);
  }
}
