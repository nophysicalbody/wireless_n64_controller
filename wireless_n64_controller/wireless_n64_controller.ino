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

unsigned long n64_status;
byte sendbuffer[4];

void loop()
{
  // Poll the controller
  n64_status = pollController();  

  // Split the status into 4 bytes:
  sendbuffer[0] = n64_status >> 24;
  sendbuffer[1] = n64_status >> 16;
  sendbuffer[2] = n64_status >> 8;
  sendbuffer[3] = n64_status;

  // Radio controller status back to console
  radio.send(TONODEID, sendbuffer, 4);
  // Wait
  delay(500);
}
  
//  // Set up a "buffer" for characters that we'll send:
//
//  static char sendbuffer[62];
//  static int sendlength = 0;
//
//  // SENDING
//
//  // In this section, we'll gather serial characters and
//  // send them to the other node if we (1) get a carriage return,
//  // or (2) the buffer is full (61 characters).
//
//  // If there is any serial input, add it to the buffer:
//
//  if (Serial.available() > 0)
//  {
//    char input = Serial.read();
//
//    if (input != '\r') // not a carriage return
//    {
//      sendbuffer[sendlength] = input;
//      sendlength++;
//    }
//
//    // If the input is a carriage return, or the buffer is full:
//
//    if ((input == '\r') || (sendlength == 61)) // CR or buffer full
//    {
//      // Send the packet!
//      Serial.print("sending to node ");
//      Serial.print(TONODEID, DEC);
//      Serial.print(", message [");
//      for (byte i = 0; i < sendlength; i++)
//        Serial.print(sendbuffer[i]);
//      Serial.println("]");
//
//      // There are two ways to send packets. If you want
//      // acknowledgements, use sendWithRetry():
//
//      if (USEACK)
//      {
//        if (radio.sendWithRetry(TONODEID, sendbuffer, sendlength))
//          Serial.println("ACK received!");
//        else
//          Serial.println("no ACK received");
//      }
//
//      // If you don't need acknowledgements, just use send():
//
//      else // don't use ACK
//      {
//        radio.send(TONODEID, sendbuffer, sendlength);
//      }
//
//      sendlength = 0; // reset the packet
//    }
//  }
//}
