/*
 * Wireless n64 controller sketch. 
 * 
 * Read n64 controller connected on pin 4 (Arduino Pro Mini 3.3 V, 8 MHz).
 * 
 * Send n64 controller state to reciever, which is connected to the console.
 * 
 */

#include <RFM69.h>
#include <SPI.h>
#include "src/n64_controller_if.h"

// Radio configuration
#define NETWORKID     0   // Must be the same for all nodes
#define MYNODEID      1   // My node ID
#define TONODEID      2   // Destination node ID
#define FREQUENCY     RF69_915MHZ
#define ENCRYPT       true // Set to "true" to use encryption
#define ENCRYPTKEY    "TOPSECRETPASSWRD" // Use the same 16-byte key on all nodes

// Create a library object for our RFM69HCW module:
RFM69 radio;

void setup()
{
  // Open a serial port so we can peek into what's going on
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
}

unsigned long n64_status = 0;
unsigned long previous_n64_status = 0;
unsigned long update_timer_ms = 0;
byte sendbuffer[4];


void loop()
{
  // Poll the controller
  n64_status = pollController();  

  if ((n64_status != previous_n64_status) || (update_timer_ms >= 500)) {
    // If there has been a change since last time, or it's been too long since
    // the last update was sent, send a n64_status update to the controller
    // Split the status into 4 bytes:
    sendbuffer[0] = n64_status >> 24;
    sendbuffer[1] = n64_status >> 16;
    sendbuffer[2] = n64_status >> 8;
    sendbuffer[3] = n64_status;
  
    // Radio controller status back to console
    radio.send(TONODEID, sendbuffer, 4);

    // Reset the timer
    update_timer_ms = 0;

  } else {
    update_timer_ms += 20;
  }
  previous_n64_status = n64_status;
  // Wait
  delay(20);
}
