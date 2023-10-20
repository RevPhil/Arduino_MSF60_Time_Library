/************************************************************************************
 MsfTimeLIb Version 2.7.0 SUITABLE FOR THE ESP8266 WIFI MODULE

This library was written for the Arduino stable but will work with ESP8266 and ESP32
devices although I would seriously suggest that NTP time would be a better call.

 A class to decode the MSF Time Signal from Anthorn, Cumbria, UK
 Inspired by Richard Jarkman's original MSFTime library but with a different
 approach.

 You are free to use this library as you see fit as long as this text remains with it!
 Copyright 2014, 2015 & 2016 Phil Morris
**************************************************************************************/

#ifndef MsfTimeLib_h
#define MsfTimeLib_h

#ifndef Arduino_h
#include <Arduino.h>
#endif

//#include "MsfAvrDefinitions.h"

#if !defined(__time_t_defined)
typedef unsigned long time_t;
#endif

// definitions for board/avr definition
#if defined (__AVR_ATmega8__) || defined(__AVR_ATmega48__) || defined (__AVR_ATmega48P__) || defined (__AVR_ATmega88__) || defined (__AVR_ATmega88P__) || defined (__AVR_ATmega168__) || defined (__AVR_ATmega168P__) || defined (__AVR_ATmega328P__)
	#define MSF_BOARD_ID 1
	#define MSF_BOARD_TYPE 			"UNO/NANO/PRO MINI ETC."
	#define MSF_AVR_TYPE 			"ATmega8/48/88/168/328(P)"
	#define MSF_INT_PINS 			2
#elif defined (__AVR_ATmega640__) || defined (__AVR_ATmega1280__) || defined (__AVR_ATmega2560__)
	#define MSF_BOARD_ID 2
	#define MSF_BOARD_TYPE 			"Mega2560"
	#define MSF_AVR_TYPE 			"ATmega640/1280/2560"
	#define MSF_INT_PINS 			6
#elif defined (__AVR_ATmega164P__) || defined (__AVR_ATmega324P__)|| defined (__AVR_ATmega644__) || defined (__AVR_ATmega1284P__)
	#define MSF_BOARD_ID 3
	#define MSF_BOARD_TYPE 			"NA"
	#define MSF_AVR_TYPE 			"ATmega164/324/644/1284"
	#define MSF_INT_PINS 			3
#elif defined ESP_H || defined ESP8266
	#define MSF_BOARD_ID 4
	#define MSF_BOARD_TYPE 			"ESP8266"
	#define MSF_AVR_TYPE 			"ESP8266"
	#define MSF_INT_PINS 			17	// ESP8266-12 (available interrupt pins are device specific)
#else
	#define MSF_BOARD_ID 0
	#define MSF_BOARD_TYPE 			"NOT APPLICABLE"
	#define MSF_AVR_TYPE 			"UNDEFINED"
	#define MSF_INT_PINS 			0
#endif

// configuration constants:
#define MSF_PULSE_LOW LOW			// MSF "off" pulse is LOW
#define MSF_PULSE_HIGH HIGH			// MSF "off" pulse is HIGH
#define MSF_NO_PIN -1				// NO PIN used

// padding in ms added to incomming pulse
#define MSF_PAD_0MS 	0
#define MSF_PAD_5MS 	5
#define MSF_PAD_10MS 	10
#define MSF_PAD_15MS 	15
#define MSF_PAD_20MS 	20
#define MSF_PAD_25MS 	25
#define MSF_PAD_30MS 	30

// internal value for decoding
#define MIN_STREAM_LEN 	58					// minimum number of seconds to receive for
											// a valid decode
#define MSF_MARKER 	0b01111110				// the end marker of the minute

// the bit offsets of the data segments in the "A" & "B" buffers											
#define MSF_YEAR_OFFSET 	42
#define MSF_MONTH_OFFSET 	34
#define MSF_DATE_OFFSET 	29
#define MSF_WEEKDAY_OFFSET 	23
#define MSF_HOUR_OFFSET 	20
#define MSF_MINUTE_OFFSET 	14
#define MSF_MARKER_OFFSET 	7
#define MSF_BST_BIT_POS 	1
#define MSF_BSTSOON_BIT_POS 6

// the number of bits to be gathered for the time/date data output
#define MSF_YEAR_BITS 		8
#define MSF_MONTH_BITS 		5
#define MSF_DATE_BITS 		6
#define MSF_WEEKDAY_BITS 	3
#define MSF_HOUR_BITS 		6
#define MSF_MINUTE_BITS 	7
#define MSF_MARKER_BITS 	8

// the number of bits to be checked in the parity routines
#define MSF_YEAR_PARITY_BITS 	8
#define MSF_MONTH_PARITY_BITS 	11
#define MSF_WEEKDAY_PARITY_BITS 3
#define MSF_HOUR_PARITY_BITS 	13

// the parity bit offsets in the "B" buffer
#define MSF_YEAR_PARITY_BIT_POS 	5
#define MSF_MONTH_PARITY_BIT_POS 	4
#define MSF_WEEKDAY_PARITY_BIT_POS 	3
#define MSF_HOUR_PARITY_BIT_POS 	2

// Byte offsets for the rtcBuffer BCD data
#define MSF_SECOND	0
#define MSF_MINUTE	1
#define MSF_HOUR	2
#define MSF_DAY		3
#define MSF_DATE	4
#define MSF_MONTH	5
#define MSF_YEAR	6

class MsfTimeLib
{
	private:
		uint8_t aBuffer[8];					// buffer for the 'A' bits
		uint8_t bBuffer[8];					// buffer for the 'B' bits
		volatile uint32_t pulseStart;		// milliseconds when start of pulse occurred
		volatile uint32_t pulseEnd;			// milliseconds when pulse ended
		volatile uint32_t lastPulseStart;	// the previous pulse start value
		volatile uint8_t pulseLength;		// length of pulse/100 as an integer
		volatile bool bitBonly;				// set if a 'B' only pulse detected
		volatile uint8_t secondBits;		// bits decoded from seconds
		volatile uint8_t bitPointer;		// pointer for bits within buffer bytes
		volatile uint8_t bitCounter;		// used to count number of "1"s in chunk routines
		volatile uint8_t ledPin;			// pin to flash on pulses, 0 = off
		volatile uint8_t msfPin;			// pin for MSF Rx signal
		volatile bool carrierOff;			// True = Rx output is HIGH when carrier is off
		volatile bool pinState;				// used for interrupt pin sensing
		volatile int8_t padding;			// time to add/subtract to/from pulse length measurement in ms
		volatile bool timeIsSet;			// true when time data has been decoded
		volatile uint8_t ponPin;			// pin used to switch the MSF module on/off. LOW = ON
				
		// Function to return x bits from buffer
		uint8_t getChunk(uint8_t * _buffer, uint8_t _start, uint8_t _numBits);
		// Function to fetch the parity bits
		uint8_t getParity();
		// Function to check the data and parity bits
		bool checkParity(uint8_t start, uint8_t numBits, uint8_t parityBitNum);
		// Function to return a single bit from a buffer
		bool getBit(uint8_t * _buffer, uint8_t _bitPos);
		// make a time_t compatible reading useable by the Time library
		time_t makeTime();
				
	public:
		MsfTimeLib();
		
		// Startup Function (Interrupt Number, Padding Time, MSF Polarity, PON pin, LED pin)
		int8_t begin(uint8_t _intNum, int8_t _padding);
		int8_t begin(uint8_t _intNum, int8_t _padding, uint8_t _carrierOff);
		int8_t begin(uint8_t _intNum, int8_t _padding, uint8_t _carrierOff, int8_t _ponPin);
		int8_t begin(uint8_t _intNum, int8_t _padding, uint8_t _carrierOff, int8_t _ponPin, int8_t _ledPin);
				
		// control		
		void rxOn(uint8_t _rxOn);		// turn ON(LOW) or OFF(HIGH) the MSF Receiver Module
		uint8_t rxIsOn(void);			// return the PON status of the MSF Receiver Module
		// utilities
		uint8_t bcdToDec(uint8_t _bcd);		// convert BCD Byte to Decimal
		uint32_t freeMem(void);			// returns the amount of free SDRAM memory
				
		void msfPulse(void);				// the actual Interrupt routine

		// time available indicators
		volatile int8_t TimeAvailable;		// set to 1 when the time has been decoded and the new minute has started
		volatile uint8_t TimeReceived;		// the final second of the minute has been received and is being processed
		volatile uint8_t ParityResult;		// the last parity result
		// time data
		volatile uint8_t rtcBuffer[7];		// BCD buffer for RTC clock bytes
		volatile bool startOfSecond;		// set at start of second pulse, reset at end of second pulse
		volatile uint8_t RxSecs;			// number of seconds received for decoding
		volatile bool Bst;					// 1 = BST, 0 = GMT
		volatile bool BstSoon;				// 1 = BST imminent
		volatile uint16_t DutPos;			// DUT1 Positive value in ms
		volatile uint16_t DutNeg;			// DUT1 Negative value in ms
		volatile time_t TimeTime;			// time_t compatible for use with Time/RTC library
		volatile int8_t LeapSecond;			// set to either -1 or +1 if a leap second is detected
		volatile uint8_t NumSeconds;		// the number of seconds received so far
};

extern MsfTimeLib msf;

#endif
