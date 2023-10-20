/* MSF TEST RECEIVER V1
 *
 *  THIS SKETCH IS NOT AN ACCURATE CLOCK DUE TO THE WAY THE TIME IS DECODED FOR
 *  THE PURPOSES OF THIS SKETCH. THE ACTUAL BLOCK OF DIAGNOSTIC INFORMATION IS
 *  ACCURATE HOWEVER! MSF TIME IS CORRECT!
 *
 *  The output of this sketch is shown below and the data presented is
 *  intended to aid diagnosis of MSF reveiving problems. The accompanying
 *  MSF Simulator sketch should be used initially to ensure that your receiving
 *  device is functioning. Once this has been established you just need to
 *  replace the Simulator output with the MSF receiver output.
 *
 *  The number to the left (xx:) is the actual count of the seconds received (msf.NumSeconds)
 *  and decoded by the MsfTimeLib library so you should see a regular increment of the count
 *  once per second. If see rapid changes or very spasmodic changes to the second
 *  number you have connection problems or interference.
 *
 *  A LED can be attached to a Digital pin with a series 1K resistor.
 *  If used, the LED will flash in sync with the MSF pulses.
 *
 *  The sketch uses the Time library which is synchronised each time a valid MSF
 *  minute has been received. The current time provided by the Time library is
 *  displayed after each second increment. THIS IS NOT THE ACCURATE TIME! as it
 *  can be up to 500ms slow. This inaccuracy is a function of this sketch not the
 *  Time library. In proper use, the time is provided by the MsfTimeLib
 *  library within a few milliseconds of the start of the minute.
 *
 *  The Time library drifts with time so, if it is used in your project it is necessary to
 *  synchronize the Time library with the MSF tima/date regularly. REMEMBER, MSF time is not
 *  guranteed to be available 24/7/365 so, it would be better to use a RTC (Real Time Clock)
 *  as your time source and synchronise it with the MSF time/date a few times a day.
 *
 *  57:  11:31:57 Wed 19 Jan 2016
 *  58:  11:31:58 Wed 19 Jan 2016
 *  59:  11:31:59 Wed 19 Jan 2016
 *
 *     MSF Time/Date: 11:32:00 Tue 19 Jan 2016  // this time is accurate
 *               BST: NO
 *      BST Imminent: NO
 *       Leap Second: 0
 *     DUT1 Positive: 100 ms
 *     DUT1 Negative: 0 ms
 *          TimeTime: 1453203120 Seconds since 1/1/1970
 *  Received Seconds: 60
 *     Parity Result: 0(No Error)
 *       Free Memory: 827392 Bytes
 *
 *  01:  11:32:01 Wed 19 Jan 2016 // this time is NOT accurate, can be up to 500ms slow
 *  02:  11:32:02 Wed 19 Jan 2016
 *  03:  11:32:03 Wed 19 Jan 2016
 *
 */

#include <TimeLib.h>       // the Arduino Time library
#include <MsfTimeLib.h> // MsfTimeLib library
#include <Streaming.h>  // the Streaming template/library

// change these entries to suit your system
#define INTERRUPT 13    // example: ESP8266 interrupt 13 on GPIO13
// example: Arduino interrupt 0 on UNO pin 2 = #define INTERRUPT 0
#define PON_PIN 0       // the pin that is used for PON control (0 = no pin)
#define LED_PIN 0       // the pin used for an LED (0 = no pin)

// macros to aid data printing
// Streaming PRINT Decimal number with leading "0"
#define SPRINTD(x) (x<10?"0":"") << x
// Streaming PRINT BCD number with leading "0"
#define SPRINTB(x) (x<10?"0":"") << _HEX(x)
// Serial PRINT Decimal number with leading "0"
#define PRINTD(x) Serial.print(x<10?"0":"");\
  Serial.print(x);
// Serial PRINT BCD number with leading "0"
#define PRINTB(x) Serial.print(x<10?"0":"");\
  Serial.print(x,HEX);

// weekday text
char* weekDay[8] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
};
// month text
char* months[13] = {
  "", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
// parity error text
char* parityErrors[5] = {
  "No Error", "Year Data Parity Error", "Month Data Parity Error", "Weekday Parity Error", "Time Data Parity Error"
};

void setup()
{
  // start the Serial port at 115200 baud (try not to use slower speeds)
  Serial.begin(115200);
  // start the MSF library Interrupt , Padding 10, Pulse HIGH, no ponPin, no LED
  // result will contain the Digital pin number associated with the chosen interrupt
  // or "0" if the start failed
  uint8_t result = msf.begin(INTERRUPT, MSF_PAD_10MS, MSF_PULSE_HIGH, PON_PIN, LED_PIN);
  if (result)
  {
    // print some basic data about the AVR/Board
    Serial << "         Arduino Type: " << MSF_BOARD_TYPE << endl;
    Serial << "       ATmel AVR Type: " << MSF_AVR_TYPE << endl;
    Serial << "No. of Interrupt Pins: " << MSF_INT_PINS << endl;
    Serial << "       This Interrupt: " << INTERRUPT << endl;
    Serial << "   This Interrupt Pin: " << result << endl;
  }
  else
  {
    // if it fails
    Serial << "MSF Library setup failed!";
    // wait forever
    while (1);
  }
}

uint8_t lastNumSeconds = 0;

void loop()
{
  // check to see if the library has valid time data. msfTimeAvailable is only
  // available during the first 500ms of the START second of the MSF stream
  if (msf.TimeAvailable)
  {
    // set the Time library using the time_t provided by the MSF library
    setTime(msf.TimeTime);
    // get the parity result (for demonstration only)
    uint8_t parity = msf.ParityResult;
    // display the MSF time
    displayTime();
    // now print the details of the MSF data
    Serial << "             BST: " << (msf.Bst == 0 ? "NO" : "YES") << endl;
    Serial << "    BST Imminent: " << (msf.BstSoon == 0 ? "NO" : "YES") << endl;
    Serial << "     Leap Second: " << msf.LeapSecond << endl;
    Serial << "   DUT1 Positive: " << msf.DutPos << " ms" << endl;
    Serial << "   DUT1 Negative: " << msf.DutNeg << " ms" << endl;
    Serial << "        TimeTime: " << msf.TimeTime << " Seconds since 1/1/1970" << endl;
    Serial << "Received Seconds: " << msf.RxSecs << endl;
    Serial << "   Parity Result: " << parity << "(" << parityErrors[parity] << ")" << endl;
    // just for luck print the free DRAM available
    Serial << "     Free Memory: " << msf.freeMem() << " Bytes" << endl << endl;
    // always cancel the TimeAvailable flag to prevent repeated action
    // until the next minute start
    msf.TimeAvailable = 0;
  }

  // compare the msf.NumSeconds value to see it it has changed
  if (msf.NumSeconds != lastNumSeconds)
  {
    // we don't want to repeat the "0" second as the MSF time has already been displayed
    if (msf.NumSeconds > 0)
    {
      // print the second number plus a colon
      Serial << SPRINTD(msf.NumSeconds) << ":  ";
      // print the time from the Time library
      Serial << SPRINTD(hour()) << ":" << SPRINTD(minute()) << ":" << SPRINTD(second()) << " ";
      Serial << weekDay[weekday() - 1] << " " << SPRINTD(day()) << " " << months[month()] << " " << year() << endl;
    }
    // match the values to prevent repeating the action
    lastNumSeconds = msf.NumSeconds;
  }
}

void displayTime()		// LCD print the internal clock
{
  // the MSF data is in BCD so we need to convert it to Decimal for diplaying
  // display the time using the SPRINTH() macro where necessary
  Serial << endl << "   MSF Time/Date: " << SPRINTB(msf.rtcBuffer[MSF_HOUR]) << ":" << SPRINTB(msf.rtcBuffer[MSF_MINUTE]) <<
         ":" << SPRINTB(msf.rtcBuffer[MSF_SECOND]) << " ";
  // display the date
  Serial << weekDay[msf.rtcBuffer[MSF_DAY]] << " " << SPRINTB(msf.rtcBuffer[MSF_DATE]) << " " <<
         months[msf.bcdToDec(msf.rtcBuffer[MSF_MONTH])] << " 20" << SPRINTB(msf.rtcBuffer[MSF_YEAR]) << endl;
}


