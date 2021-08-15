
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
#define TX_PIN 9    // Connect to ATtiny85 N64 Controller RX pin
#define CLK_PIN 8   // Connect to ATtiny85 N64 Controller CLK pin
#define TIMEOUT_PERIOD_MS 2

// Create a library object for our RFM69HCW module:
RFM69 radio;

int synchronous_write_byte(byte out){
  for (int b=0; b<8; b++) {
    // Wait for falling edge on CLK_PIN
    long start_time = millis();
    while (digitalRead(CLK_PIN)) {
      if ((millis() - start_time) > TIMEOUT_PERIOD_MS) {
        return 0; // Indicate timeout
      }
    }
    // Rising edge has occurred. Write MSB to TX_PIN
    digitalWrite(TX_PIN, out & 0b10000000);
    out = out << 1;

    // Wait for rising edge:
      while (!digitalRead(CLK_PIN)) {
      if ((millis() - start_time) > TIMEOUT_PERIOD_MS) {
        return 0; // Indicate timeout
      }
    }
  }
  return 1;
}

void write_32_bit_controller_value(uint32_t out){
  // Assemble components
  byte byte_0 = (out >> 24) & 0xFF;
  byte byte_1 = (out >> 16) & 0xFF;
  byte byte_2 = (out >> 8) & 0xFF;
  byte byte_3 = out & 0xFF;
  byte checksum = byte_0 + byte_1 + byte_2 + byte_3;

  // Send bytes to virtual controller
  digitalWrite(TX_PIN, LOW); // Set TX line low to indicate we wish to start sending
  synchronous_write_byte(byte_0);
  synchronous_write_byte(byte_1);
  synchronous_write_byte(byte_2);
  synchronous_write_byte(byte_3);
  synchronous_write_byte(checksum);
  digitalWrite(TX_PIN, HIGH);
}

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
  // set up IO for virtual n64 controller interface
  pinMode(CLK_PIN, INPUT);
  pinMode(TX_PIN, OUTPUT);
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

    // Pass the update synchronously to the virtual n64 controller
    write_32_bit_controller_value(n64_status);
    //Serial.println(n64_status, HEX);
  }
}
