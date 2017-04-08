//
// Bridge CAN-ASCII (produced by JMRI-OpenLCB) from a serial input (from ESP8266) to a physical CAN adapter
//
/*
   From: https://gridconnect.com/media/documentation/grid_connect/CAN_USB232_UM.pdf

   Normal CAN Message Syntax

   : <S | X > <IDENTIFIER> <N> <Data0> <Data1> ... <Data7> ;

   RTR CAN Message Syntax

   : <S | X > <IDENTIFIER> <R> <Length> ;


*/
/* Example CAN-ASCII Messagesfrom the JMRI OpenLCB tool

  Bytes Available: 12
  :X170002B7N;
  Bytes Available: 60
  :X160002B7N;:X150002B7N;:X140002B7N;:X107002B7N;:X194902B7N;
  Bytes Available: 18
  :X00000123N120456;
  Bytes Available: 18
  :X00000123N120456;
  Bytes Available: 18
  :X00000123N120456;
  Bytes Available: 26
  :X1A2B72B7N20610000000008;
  Bytes Available: 16
  :X199702B7N02B7;
  Bytes Available: 20
  :X00000123N12345678;
*/

// handling of the CAN adapter format is performed by the MCP2515 library (mcp_can)

// https://github.com/coryjfowler/MCP_CAN_lib.git
#include <mcp_can.h>

#include <SPI.h>

//#define SHOWLCD
//#define SHOWSERIAL

// Arduino SPI Pins
//
// CLK  D13
// MISO D12
// MOSI D11
// CS   D10
//
// CAN Interrupt D2
//

#define CANINTPIN   2
//#define GPIO12 12
//#define GPIO13 13
//#define GPIO14 14
//#define GPIO15 15

#ifdef SHOWLCD
#include <LiquidCrystal_I2C.h>
/*
  Pins: SDA   A4
        SCK   A5
*/

#define I2C_ADDR    0x3f
#define BACKLIGHT_PIN     3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7


LiquidCrystal_I2C  lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin);
#endif

bool initialised = false;
enum WiFiDecodeStatus {expect_colon, expect_msg_type, process_identifier, process_data};
enum CAN_message_type {Standard = 0, Extended = 1};
WiFiDecodeStatus decodeStatus = expect_colon;
CAN_message_type  WiFiMessageType = Standard;
CAN_message_type  CANMessageType = Standard;

MCP_CAN CAN(10); // Set CS to pin 10

//volatile boolean can_msg_received = false;

// send and receive buffers for the can packets
// CAN => received from the CAN interface; WiFi => received over WiFi
long unsigned int CANIdentifier = 0;
unsigned char CANDataLen = 0;
unsigned char CANData[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

char CANAscii[21]; // array to store the ASCII-encoded message

long unsigned int WiFiIdentifier = 0;
byte WiFiDataLen = 0;
byte WiFiData[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

char hexDigits[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
int i = 0;
boolean firstNybble = true;

char msgString[128];                        // Array to store serial string


//void MCP2515_ISR()
//{
//  can_msg_received = true;
//}

//void CAN_Send(unsigned char cantxValue1, unsigned char cantxValue2, unsigned char cantxValue3, unsigned char cantxValue4, unsigned char cantxValue5, unsigned char cantxValue6, unsigned char cantxValue7, unsigned char cantxValue8) {
//  unsigned char canMsg[8] = {cantxValue1, cantxValue2, cantxValue3, cantxValue4, cantxValue5, cantxValue6, cantxValue7, cantxValue8};
//  CAN.sendMsgBuf(I2C_ADDR, 0, 8, canMsg);
//}


int Hex2Int(char ch) {
  // convert the input character from a Hex value to Int
  // if the input is not a valid hex character (0..9A..F), 16 will be returned and is an error value

  int i = 0;
  for (i = 0; i <= 15; i++) {
    if (ch == hexDigits[i]) break;
  }
  return i;
}

char Int2Hex(int nybble) {
  if (nybble >= 0 && nybble <= 15)
    return hexDigits[nybble];
  else
    return '\0';
}

void setup() {
  Serial.begin(115200);
#ifdef SHOWSERIAL
  Serial.println("Starting...");
#endif  

#ifdef SHOWLCD
  lcd.begin (16, 2); //  <<----- My LCD was 16x2
  lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.clear();
#endif

  pinMode(CANINTPIN, INPUT); 
  
  //start UART and the server
  //Serial.begin(115200);

  initialised = (CAN.begin(MCP_ANY, CAN_125KBPS, MCP_8MHZ) == CAN_OK);
  if (initialised) {
#ifdef SHOWLCD
    lcd.print("can init ok");
#endif

    CAN.setMode(MCP_NORMAL);

    //attachInterrupt(CANINTPIN, MCP2515_ISR, FALLING); // start interrupt
    
//    Serial.println("can init ok");
  }
#ifdef SHOWLCD
  else
    lcd.print("Can init fail");
#endif

//for (int x = 0; x < 16; x++){
//  Serial.print(hexDigits[x]); Serial.print(": "); Serial.println(Hex2Int(hexDigits[x])); 
//}
}

void loop() {
	char buf[50];

  // Send incoming message from CAN to WiFi (if available)
  if (!digitalRead(CANINTPIN)) {
//  if (can_msg_received) {
#ifdef SHOWSERIAL
	Serial.println("Message received.");
#endif	  
    //can_msg_received = false;

    CAN.readMsgBuf(&CANIdentifier, &CANDataLen, CANData);      // Read data: len = data length, buf = data byte(s)
    
#ifdef SHOWSERIAL
    Serial.print("CANIdentifier: "); Serial.println(CANIdentifier, HEX);
    Serial.print("CANDataLen: "); Serial.println(CANDataLen, HEX);
    if (CANDataLen > 0){
        Serial.print("CANData: "); 
        for (uint8_t i = 0; i < CANDataLen; i++){
            sprintf(buf, "%02hhX", CANData[i]);
            Serial.print( buf );
			}
        Serial.println("");
    }
#endif    

    if ((CANIdentifier & 0x40000000) == 0x40000000) {  // Determine if message is a remote request frame.
#ifdef SHOWLCD
      sprintf(msgString, " REMOTE REQUEST FRAME");
      lcd.clear();
      lcd.print(msgString);
#endif
    }
    else {
      if ((CANIdentifier & 0x80000000) == 0x80000000)  {  // Determine if ID is standard (11 bits) or extended (29 bits)
        CANMessageType = Extended;
        CANIdentifier = CANIdentifier & 0x1FFFFFFF;
      }
      else {
        CANMessageType = Standard;
        CANIdentifier = CANIdentifier & 0x7FF;
      }

      //for (int i = 0; i < 21; i ++) {
        //CANAscii[i] = '\0';  // clear the output string
      //}
      //int pos = 2 + ((CANMessageType == Standard) ? 3 : 8);
      //unsigned long id = CANIdentifier;

      //CANAscii[0] = ':';
      //CANAscii[1] = ((CANMessageType == Standard) ? 'S' : 'X');
      //for (int i = pos - 1; i >= 2; i--) {                // write the identifier
        //CANAscii[2 + i] = hexDigits[id & 0x0F];
        //id = id << 4;
      //}
      //CANAscii[pos++] = 'N';                              // write the data (if any)
      //for (int i = 0; i < CANDataLen; i++) {
        //CANAscii[pos++] = hexDigits[(CANData[i] >> 4) & 0x0F];
        //CANAscii[pos++] = hexDigits[CANData[i] & 0x0F];
      //}
      //CANAscii[pos] = ';';                                // write the terminator
      
      if (CANMessageType == Extended){
		  sprintf(CANAscii, ":X%08lXN", CANIdentifier);
	  } else {
		  sprintf(CANAscii, ":X%03lXN", CANIdentifier);
	  }
	  for (uint8_t i = 0; i < CANDataLen; i++){     // write any data bytes
		  sprintf(buf, "%02hhX", CANData[i]);
		  strcat(CANAscii, buf);
		  }
	  strcat(CANAscii, ";");                        // add the terminator
      Serial.print((String)CANAscii);               // Transmit the CAN-Ascii message to the client
      
#ifdef SHOWLCD
      lcd.clear();
      lcd.print("Sending to WiFi client: ");
      lcd.println((String)CANAscii);
#endif
#ifdef SHOWSERIAL
      //Serial.print("Sending to WiFi client: ");
      //Serial.println((String)CANAscii);
      Serial.println();
#endif
    }
  }

  // send incoming message from WiFi to CAN  (if available)
  if (Serial.available()) {
    //get data from the telnet client
    while (Serial.available()) {
      char ch = Serial.read();
#ifdef SHOWLCD
      lcd.print(ch); // push it to the UART (=> Serial Monitor)
#endif
      //Serial.write(ch);
      switch (decodeStatus) {
        case expect_colon:
          // if we receive a : change the status, initialise the data stores and discard the character
          // otherwise ignore the character - it's not what we were expecting
          if (ch == ':') {
            decodeStatus = expect_msg_type;
            WiFiMessageType = Standard;
            WiFiIdentifier = 0;
            WiFiDataLen = 0;
            firstNybble = true;
            //Serial.println("Got colon");
          }
          break;

        case (expect_msg_type):
          // the message Type indicators are S (Standard) and X (Extended)
          // if we receive one of these, set the message type accordingly and change the status
          // otherwise something is wrong therefore we reset to expecting_colon
          decodeStatus = process_identifier;  // assume a valid message type
          switch (ch) {
            case 'S':
              WiFiMessageType = Standard;
              break;

            case 'X':
              WiFiMessageType = Extended;
              break;

            default:
              // we shouldn't be here - reset the processor
              decodeStatus = expect_colon;
              break;
          }
          //Serial.println("Got Message type");
          break;

        case (process_identifier):
          // read hex chars until we hit an N
          if (ch == 'N') {
            decodeStatus = process_data;
            //Serial.print("Got Identifier "); Serial.println(WiFiIdentifier);
            break;
          }

          // convert the identifier from Hex to Int
          i = Hex2Int(ch);
          //Serial.print(ch); Serial.print(": ");Serial.println(i);
          if (i > 15) {
            // not a hex character => reset the processor
            decodeStatus = expect_colon;
            break;
          }
          WiFiIdentifier = (WiFiIdentifier << 4) + i;
          //Serial.println(WiFiIdentifier);
          break;

        case (process_data):
          // process data bytes (Hex encoded) until we hit a ;
          if (ch == ';') {
            // received a ; - send the message and reset the processor for the next
            byte sndStat = CAN.sendMsgBuf(WiFiIdentifier, (int) WiFiMessageType, WiFiDataLen, WiFiData);
#ifdef SHOWLCD
            if (sndStat == CAN_OK) {
              lcd.println("Message Sent Successfully!");
            } else {
              lcd.println("Error Sending Message...");
            }
#endif
            //if (sndStat == CAN_OK) {
            //  Serial.println("Message Sent Successfully!");
            //} else {
            //  Serial.println("Error Sending Message...");
            //}
            
            decodeStatus = expect_colon;
            break;
          }

          if (firstNybble) {
            WiFiData[WiFiDataLen] = 0x0;
            WiFiDataLen++;
          }

          firstNybble = !firstNybble;
          i = Hex2Int(ch);
          if (i > 15) {
            // not a hex character => reset the processor
            decodeStatus = expect_colon;
            break;
          }

          // WiFiDataLen contains the actual length, which is 1 greater than the index value
          WiFiData[WiFiDataLen - 1] = (WiFiData[WiFiDataLen - 1] << 4) + i;
          break;

        default:
          // should not be here - just reset the processor
          decodeStatus = expect_colon;
          break;
      }
    }
  }

}

