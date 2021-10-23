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
#define ENCRYPTKEY    "TOPSECRETPASSWRD" // Use the same 16-byte key on all nodes

// IO configuration
#define LED_PIN 9

// Create a library object for our RFM69HCW module:
RFM69 radio;

void setup()
{
  // Initialize the RFM69HCW:
  radio.initialize(FREQUENCY, MYNODEID, NETWORKID);
  radio.setHighPower(); // Always use this for RFM69HCW
  radio.encrypt(ENCRYPTKEY);

  pinMode(LED_PIN, OUTPUT);
}


#define LOOP_PERIOD_MS 20
#define INACTIVITY_TIME_MS 5000

boolean controller_state_recently_changed(unsigned long new_status){
  static unsigned long prev_status = new_status;
  static unsigned long inactivity_timer = 0;

  if (new_status == prev_status) {
    inactivity_timer += LOOP_PERIOD_MS;
  } else {
    inactivity_timer = 0;
  }

  prev_status = new_status;

  boolean controller_active = (inactivity_timer <= INACTIVITY_TIME_MS);
  if (not controller_active) {
    inactivity_timer = INACTIVITY_TIME_MS; // so we don't keep growing and eventually overflow (though admittedly that would probably take days)
  }

  return controller_active;
}


void send_controller_update(unsigned long new_status){
  byte sendbuffer[4];
  // Split the status into 4 bytes:
  sendbuffer[0] = new_status >> 24;
  sendbuffer[1] = new_status >> 16;
  sendbuffer[2] = new_status >> 8;
  sendbuffer[3] = new_status;

  // Radio controller status back to console
  radio.send(TONODEID, sendbuffer, 4);
}


void loop()
{
  static unsigned long n64_status = 0;
  
  // Poll the controller
  n64_status = pollController();  

  // Check whether the controller is active
  boolean controller_active = controller_state_recently_changed(n64_status);
  digitalWrite(LED_PIN, controller_active);
  
  if (controller_active) 
    send_controller_update(n64_status);

  // Wait
  delay(LOOP_PERIOD_MS);
}
