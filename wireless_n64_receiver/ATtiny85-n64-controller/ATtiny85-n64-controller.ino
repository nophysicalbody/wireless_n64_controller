//#include <SoftwareSerial.h>
#include "src/n64_console_if.h"

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
volatile byte N64_CONTROLLER_STATUS[3] = {0x05,0x00,0x00};
volatile byte console_command_id;

ISR(INT0_vect)
{
  GIMSK = 0; // turn off all interrupts

  console_command_id = get_incoming_command_from_console();
  if (console_command_id == N64_STATUS) {
    respond_to_status_command(N64_CONTROLLER_STATUS);
  }
  
  //handle_command_from_console(N64_CONTROLLER_STATE);
  
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

  //Ser.println(console_command_id, BIN);
  delay(100);
  // Reset Watchdog
  asm volatile("wdr \n\t");
//  
//  // Handle external commands:
//  bool rx_state = digitalRead(RX_PIN);
//  
//  if (previous_rx_state && !rx_state) 
//  {
//    // We have detected a falling edge on the RX line, this indicates a transmit request.
//    
//    // Read into buffers:
//    incomingByte0 = synchronous_read_byte();
//    incomingByte1 = synchronous_read_byte();
//    incomingByte2 = synchronous_read_byte();
//    incomingByte3 = synchronous_read_byte();
//    checksumByte  = synchronous_read_byte();
//
//    // Calculate checksum and set flag if we're ready to update:
//    if (((incomingByte0 + incomingByte1 + incomingByte2 + incomingByte3) & 0xFF) == checksumByte) {
//      update_available = true;
//    } else {
//      update_available = false;
//    }
//    //update_available = ((incomingByte0 + incomingByte1 + incomingByte2 + incomingByte3) & 0xFF) == checksumByte;
//  }
//
//  if (update_available) {
//    // Turn off external interrupts (but allow pin change interrupts) - make changes to ISR variables here!
//    GIMSK = 0b00100000; 
//    N64_CONTROLLER_STATE[0] = incomingByte0;
//    N64_CONTROLLER_STATE[1] = incomingByte1;
//    N64_CONTROLLER_STATE[2] = incomingByte2;
//    N64_CONTROLLER_STATE[3] = incomingByte3;
//    GIMSK = 0b01100000; // turn external interrupts back on
//    update_available = false;
//  }
//  previous_rx_state = rx_state;
}
