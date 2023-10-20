/* A SIMPLE MSF SIGNAL SIMULATOR FOR THE ARDUINO
 This sketch simulates the MSF Radio Time Signal from the
 National Physical Laboratory. DUT1, BST and LEAP Seconds
 can be configured by the user.
 
 (C) 2016 by Phil Morris
 
 Truth table for A & B channels
 B      A
 -      -
 0      0  100mS off
 0      1  200mS off
 1      0  100ms off + 100mS on + 100mS off
 1      1  300mS off
 0      0  500mS off (START)
 
 The DUT1 values are added or subtracted upon receipt of the time data by the receiver.
 Only one DUT segment can be set, either DUT_POSITIVE or DUT_NEGATIVE; NOT BOTH!
 Starting with bit "7", the DUT values are calculated like this:
 
 76543210    Value(milli seconds)
 00000000    0
 10000000    100
 11000000    200
 11100000    300
 11110000    400
 11111000    500
 11111100    600
 11111110    700
 11111111    800
 
 LEAP_SECOND can be "-1", "0" or "1" which will add or subtract
 a second from the stream. NO OTHER VALUE IS ALLOWED! for MOST applications
 there is no practical use for LEAP seconds!
 
 C_ON is the required output level when the Carrier is ON. This can be
 inverted e.g. C_ON = HIGH to invert the output signal. The default is
 Carrier ON = LOW, Carrier OFF = HIGH.
 */

/* USER CONFIGURATION */
#define DUT_POSITIVE  0    // the DUT1 Positive value (see above)
#define DUT_NEGATIVE 0     // the DUT1 Negative value (see above)
#define BST_IMMINENT 0     // BST is about to start "0" or "1"
#define BST 0              // BST is active "0" or "1"
#define LEAP_SECOND 0      // "-1", "0" or "1" only (see above)
#define PIN_CARRIER 13     // the output pin for the MSF signal
const uint8_t C_ON = LOW;  // the logic level when the Carrier is ON

// START DATE & TIME
#define START_YEAR 2016
#define START_MONTH 1
#define START_DATE 5
#define START_HOUR 12
#define START_MINUTE 30
#define START_SECOND 0

/* BEYOND HERE BE TIGRES */
#include <TimeLib.h>
#include <Streaming.h>

// segement Byte offsets for buffers
#define DUT_POS 0
#define DUT_NEG 1
#define YEAR 2
#define MONTH 3
#define DATE 4
#define DAY 5
#define HOUR 6
#define MINUTE 7
#define PARITY 8

// Parity bits and BST in Parity segment
#define BIT_BST_IMMINENT 6  // bit which indicates BST is imminent
#define BIT_54B 5           // Parity bit for YEAR
#define BIT_55B 4           // Parity bit for MONTH + DATE
#define BIT_56B 3           // Parity bit for DAY
#define BIT_57B 2           // Parity bit for HOUR + MINUTE
#define BIT_BST 1           // bit which indicates BST is active 

// buffers and data arrays
uint8_t aBuffer[9];
uint8_t bBuffer[9];
uint8_t segmentLength[9] = {
  8,8,8,5,6,3,6,7,8};
char * segmentNames[9] ={
  "DUT+","DUT-","YEAR","MONTH","DATE","DAY","HOUR","MINUTE","PARITY"};

const uint8_t C_OFF = !C_ON;  // DO NOT ALTER!
uint8_t x;

void setup() 
{
  Serial.begin(115200);
  // set the Time library to 00:00:00 1/1/2016
  // you may change this to whatever you choose
  setTime(START_HOUR,START_MINUTE,START_SECOND,START_DATE,START_MONTH,START_YEAR);
  // set the carrier pin to OUTPUT
  pinMode(PIN_CARRIER,OUTPUT);
  // turn the carrier ON
  digitalWrite(PIN_CARRIER,C_ON);
}// END OF SETUP

uint32_t startMillis;
uint32_t totalMillis;

void loop() 
{
  uint8_t parity;
  // All the work can be done while the pulses are being sent
  // so turn the carrier off and build the A & B buffers
  // during the START pulse which is 500 mS
  digitalWrite(PIN_CARRIER,C_OFF);
  // set the timer
  startMillis = millis();
  totalMillis = startMillis;
  // output to Serial
  Serial.println("START");
  // clear the buffers
  memset(&aBuffer,0,sizeof(aBuffer));
  memset(&bBuffer,0,sizeof(bBuffer));
  // fill the buffers with data
  bBuffer[DUT_POS] = DUT_POSITIVE;
  bBuffer[DUT_NEG] = DUT_NEGATIVE;
  aBuffer[YEAR] = decToBcd(year()-2000);
  aBuffer[MONTH] = decToBcd(month());
  aBuffer[DATE] = decToBcd(day());
  aBuffer[DAY] = decToBcd(weekday());
  aBuffer[HOUR] = decToBcd(hour());
  aBuffer[MINUTE] = decToBcd(minute());
  aBuffer[PARITY] = 0b01111110;
  // update the BST & BST Imminent bits
  bitWrite(bBuffer[PARITY],BIT_BST,BST);
  bitWrite(bBuffer[PARITY],BIT_BST_IMMINENT,BST_IMMINENT);
  // get Parity results and set the Parity bits
  generateParity(YEAR,1, BIT_54B);
  generateParity(MONTH,2, BIT_55B);
  generateParity(DAY,1, BIT_56B);
  generateParity(HOUR,2, BIT_57B);
  // wait for the 500 mS START pulse to finish and turn the carrier back on
  while(millis() - startMillis < 500);
  digitalWrite(PIN_CARRIER,C_ON);
  // now wait until the total time = 1000 mS has elapsed before proceding
  // THIS ENDS THE FIRST SECOND
  while(millis() - startMillis <1000);
  // cycle through the 9 data segments in the aBuffer and bBuffer
  for(x = 0;x<9;x++)
  {
    // output debug data
    Serial << "Segment(" << (x+1) << ") " << segmentNames[x] <<\ 
    "   B segment = 0x" << _HEX(bBuffer[x]) << "   A segment = 0x" <<\  
    _HEX(aBuffer[x]) << endl;
    // send the segments 1 to 9, in order, MSB first
    sendSegment(segmentLength[x],aBuffer[x],bBuffer[x]);
  }
  // report how long the minute took to process and send
  Serial << "Total elapsed time (mS): " << (millis() - totalMillis) << endl;
}// END OF LOOP

void sendSegment(uint8_t numBits, uint8_t aSegment, uint8_t bSegment)
{
  uint8_t bitVal = 0;
  // shift the bits in the data segments left so that only the wanted bits
  // are lined up. Lose the unwanted upper bits if necessary
  aSegment = aSegment << 8 - numBits;
  bSegment = bSegment << 8 - numBits;
  // adjust for a LEAP Second by adding/subtracting a bit, x = the current segment
  // the last bit of DUT1- is either not sent or an additional bit is sent
  if(x == DUT_NEG) numBits += LEAP_SECOND;
  // cycle through the data segments and send the bits, one per second
  for(uint8_t y = 0;y<numBits;y++)
  {
    // turn the carrier OFF
    digitalWrite(PIN_CARRIER,C_OFF);
    // set the timer
    startMillis = millis();
    // calculate a value from the A and B buffer segment bits
    bitVal = bitRead(bSegment,7);
    bitVal = bitVal << 1;
    bitVal += bitRead(aSegment,7);
    aSegment = aSegment << 1;
    bSegment = bSegment << 1;
    Serial << ">> " << (bitVal<2?"0":"") << _BIN(bitVal) << endl;
    // use the calculated value to decide the necessary output pulse length
    switch(bitVal)
    {
    case 0:
      // a 100 mS pulse
      while(millis() - startMillis < 100);
      break;
    case 1:
      // a 200 mS pulse
      while(millis() - startMillis < 200);
      break;
    case 2:
      // a 100 mS pulse + 100 mS of Carrier + 100 mS pulse
      while(millis() - startMillis < 100);
      digitalWrite(PIN_CARRIER,C_ON);
      while(millis() - startMillis < 200);
      digitalWrite(PIN_CARRIER,C_OFF);
      while(millis() - startMillis < 300);
      break;
    case 3:
      // a 300 mS pulse
      while(millis() - startMillis < 300);
      break;
    }
    // turn the Carrier back ON
    digitalWrite(PIN_CARRIER,C_ON);
    // wait until the second has finished
    while(millis() - startMillis < 1000);
    // turn the Carrier OFF for the start of the next second    
  }
  //digitalWrite(PIN_CARRIER,C_ON);
}

void generateParity(uint8_t startSegment, uint8_t numSegments, uint8_t targetBit)
{
  //bitWrite(bBuffer[PARITY],BIT_54B,!bitRead(getParity(YEAR,1),0));
  // cycles through the buffer segments specified to count "1" bits
  uint8_t result = 0;
  // loop for the number of segments to analise
  for(uint8_t w = 0;w<numSegments;w++)
  {
    // loop for the bits in the segment
    for(uint8_t z = 0;z<8;z++)
    {
      // add the bits to the result
      result += bitRead(aBuffer[startSegment+w],z); 
    }
  }
  // result contains the nomber of "1" bits found
  // if the segment parity is even, the Parity bit = "1"
  // if the segment parity is odd, the Parity bit = "0"
  bitWrite(bBuffer[PARITY],targetBit,!bitRead(result,0));
}

// Convert normal decimal numbers to binary coded decimal
uint8_t decToBcd(uint8_t val)
{
  return ( (val/10*16) + (val%10) );
}

// END OF MSF SIGNAL SIMULATOR SKETCH (C) 2016 by Phil Morris
