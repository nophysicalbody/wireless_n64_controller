
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

// Create a library object for our RFM69HCW module:
RFM69 radio;

void setup()
{
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


unsigned long n64_status;

void loop()
{
  if (radio.receiveDone() && (radio.DATALEN==4)) // Got a 4 byte n64 status
  {
    n64_status = 0;
    // Read out the information:
    for (byte i = 0; i < 4; i++){
      n64_status = n64_status << 8;
      n64_status += radio.DATA[i];
    }

    // If the B button is pressed, light the green indicator
    digitalWrite(GN_LED, bool(n64_status & A_IDX));

    // If the Start button is pressed, light the red indicator
    digitalWrite(RD_LED, bool(n64_status & START_IDX));

    //Serial.println(n64_status, HEX);
  }
}
