/************************************************************************************
 MsfTimeLIb Version 2.7.0 SUITABLE FOR THE ESP8266 WIFI MODULE

 A class to decode the MSF Time Signal from Anthorn, Cumbria, UK
 Inspired by Richard Jarkman's original MSFTime library but with a different
 approach.

 You are free to use this library as you see fit as long as this text remains with it!
 Copyright 2014, 2015 & 2016 Phil Morris <www.lydiard.plus.com>
**************************************************************************************/

#include <MsfTimeLib.h>

MsfTimeLib *MSFs = NULL;

MsfTimeLib::MsfTimeLib() {}

// AVR & ESP8266 interrupt pin assignment examples
/*
		Board	int.0	int.1	int.2	int.3	int.4	int.5
Uno, Ethernet	2		3	 	 	 	 
Mega2560		2		3		21		20		19		18
ATmega1284		10		11		3

ESP8266 Interrupt = GPIO number (except for GPIO16 which does not support interrupts)

All digital pin numbers are the Arduino "Dx" pin numbers.
*/

#if MSF_BOARD_ID == 1	// UNO, NANO etc.
	static int8_t interuptPins[MSF_INT_PINS] = {2,3};
#elif MSF_BOARD_ID == 2	// MEGA2560 etc.
	static int8_t interruptPins[MSF_INT_PINS] = {2,3,21,20,19,18};
#elif MSF_BOARD_ID == 3	// ATmega1284 AVR etc.
	static int8_t interruptPins[MSF_INT_PINS] = {10,11,3};
#elif MSF_BOARD_ID == 4	// ESP8266 (available interrupt pins are device specific)
	static int8_t interruptPins[MSF_INT_PINS] = {0,1,2,3,4,5,-1,-1,-1,-1,-1,-1,12,13,14,15,-1};
#else
	static int8_t interruptPins[MSF_INT_PINS] = {};
#endif

void msfIntChange() 	// static function for the interrupt handler
{
  if (MSFs) MSFs->msfPulse();
}

//     _intNum: 	Arduino Interrupt Number
//    _padding:		In ms. If your MSF receiver gives pulses that are shorter
// _carrierOff:		The actual output level from the receiver when the carrier is OFF (default HIGH)
//     _ponPin:		The Arduino pin used to control the PON input on the MSF Receiver Module
//     _ledPin: 	A Data pin to attach and flash an led on in time with incoming MSF signal(0 = no led)

int8_t MsfTimeLib::begin(uint8_t _intNum, int8_t _padding)
{
	return begin(_intNum, _padding, HIGH, 0, 0);
}

int8_t MsfTimeLib::begin(uint8_t _intNum, int8_t _padding, uint8_t _carrierOff)
{
	return begin(_intNum, _padding, _carrierOff, 0, 0);
}

int8_t MsfTimeLib::begin(uint8_t _intNum, int8_t _padding, uint8_t _carrierOff, int8_t _ponPin)
{
	return begin(_intNum, _padding, _carrierOff, _ponPin, 0);
}

int8_t MsfTimeLib::begin(uint8_t _intNum, int8_t _padding, uint8_t _carrierOff, int8_t _ponPin, int8_t _ledPin)
{
	// is the interrupt pin requested within available range?
	if(_intNum > MSF_INT_PINS) return -1;
	msfPin = interruptPins[_intNum];
	// is the interrupt pin a valid interrupt pin?
	if(msfPin == -1) return msfPin;
	padding = _padding;
	carrierOff = _carrierOff;
	ponPin = _ponPin;
	ledPin = _ledPin;
	if(ledPin)	pinMode(ledPin, OUTPUT);	// set LED pin to OUTPUT if specified
	if(ponPin)
	{
		pinMode(ponPin, OUTPUT);	// if pon_pin is > 0, set as OUTPUT
		digitalWrite(ponPin, LOW);	// set pin LOW (PON ON)
	}
	memset(&aBuffer[0], 0xFF, sizeof(aBuffer));	// clear the "A" buffer to "1"s
	memset(&bBuffer[0], 0, sizeof(bBuffer));	// clear the "B" buffer	to "0"s
	MSFs = this; // singleton pointer
	attachInterrupt(_intNum, msfIntChange, CHANGE);
	return msfPin;
}

// turn the Receiver PON input ON (LOW) or OFF (HIGH)
void MsfTimeLib::rxOn(uint8_t _rxOn)
{
	digitalWrite(ponPin, _rxOn);
}

uint8_t MsfTimeLib::rxIsOn(void)
{
	return digitalRead(ponPin);	// return the state of the ponPin. LOW = ON
}

void MsfTimeLib::msfPulse(void)	// interrupt routine which is called every time the MSF receiver output changes state
{
// This routine is called every time the selected Interrupt pin changes state. If it is the start of
// a pulse, the millis count is stored in "pulseStart". If it is the end of a pulse the millis count
// is stored in "pulseEnd". "pulseLength" is the result in millis/100. The data is processed to produce an
// integer 1 - 5 representing 100 - 500 ms pulses (no "4" is decoded). "secondBits" contains the binary data
// for the "A" and "B" buffer contents. "secondBits" data is written as it is detected so, even a double "B"
// pulse is written as an "A" bit first. When the second "B" bit is detected, the "bitBonly" flag is set
// during the current second, and the pointer to the buffers is decremented one position which overwrites
// the previous data.

  bitBonly = false;					// clear the bitOnly flag
  secondBits = 0;					// clear the secondBits variable
  pinState = digitalRead(msfPin);	// get the state of the interrupt pin

// is this a pulse start?
  if (pinState == carrierOff)				// pulse or sub-pulse has started, carrier going off
	{
		pulseStart = millis();				// pulseStart = current millis everytime the MSFPIN goes low
		if(timeIsSet)						// this is the first second of the new minute
		{
			TimeAvailable = 1;				// set flag for user to sync minute if this is the first second of the minute
			timeIsSet = false;			// clear the flag to prevent false synchronisation
			TimeReceived = 0;				// clear the flag to prevent false synchronisation
			NumSeconds = 0;					// number of seconds counter
		}
		//startOfSecond = true;					// set this flag for later use
		if(ledPin)	digitalWrite(ledPin,HIGH);	// turn on LED if designated ledPin > 0
		return;									// until there's a another interrupt change
	}

// is this a pulse end?
  if(pinState != carrierOff)								// pulse end, carrier going on
	{
		pulseEnd = millis();								// set the pulse end ms
		//startOfSecond = false;								// clear the start of second flag
		TimeAvailable = 0;									// clear the user flag
		TimeReceived = 0;
		// get the pulse length in ms/100 plus padding
		//pulseLength = abs(((pulseEnd - pulseStart)+ padding) / 100);
		pulseLength = ((pulseEnd - pulseStart)+ padding) / 100;
		if (!pulseLength) return;							// if the pulse is too short ("0"), return
		pulseStart = millis();								// set the pulseStart to current millis
		// if the sequence was 100ms off + 100ms on + 100ms off, this is a 'B' stream only bit
		// so, if this start pulse is less than 300ms after the last start pulse it must be
		// a double 100ms pulse second
		if(pulseStart - lastPulseStart < 300) bitBonly = true;	// this is a 'B' bit
	    lastPulseStart = pulseStart;							// keep the last pulse start ms count
		if(ledPin) digitalWrite(ledPin,LOW);					// turn off the LED if designated ledPin > 0
	}

 switch(pulseLength)	// start processing the valid pulse
	{
		case 5:	// start pulse i.e. 500ms/100
			bitPointer = 0;								// clear the buffer bit pointer
			memset(&aBuffer[0], 0xFF, sizeof(aBuffer));	// clear the "A" buffer to all "1"s
			memset(&bBuffer[0], 0, sizeof(bBuffer));	// clear the "B" buffer to all "0"s
			break;
		case 4:	// in the unlikely event we get a "4" quit
			return;
		case 3:	// check for 300ms/100 pulse, this is an 'A' + 'B' bit case
			secondBits = 0b11;	// both "A" and "B" bits are set
			break;
		case 2:	// check for 200ms/100 pulse, this is an 'A' bit case
			secondBits = 0b01;	// only the "A" bit is set
			break;
		case 1:	// check for 100ms/100 pulse
			secondBits = 0b00;
			if(bitBonly) secondBits = 0b10;		// only the "B" bit is set
			break;			
	}

// store the data in the arrays
  if(pulseLength < 5)  // only pass this point if it's not a "Start" pulse e.g. < 500ms
	{
		bitPointer++;						// increment the bit pointer which always starts at 1
		if(bitBonly)
		{
			bitPointer--;			// if this is a "B" bit, decrement the bit pointer as	
		}							// we have already written 0 bits to both the "A" and "B" buffers
		else NumSeconds++;			// increment the NumSeconds counter
	// store the bit in the "A" buffer
		bitWrite(aBuffer[abs(bitPointer/8)], (bitPointer % 8) ^ 0x07, bitRead(secondBits,0));
	// store the bit in the "B" buffer
		bitWrite(bBuffer[abs(bitPointer/8)], (bitPointer % 8) ^ 0x07, bitRead(secondBits,1));

// we detect the last second of the minute by looking for the binary sequence "01111110" in the "A" buffer
// bits 52 thru 59. If we see this sequence it's time to stop decoding and start working on the data received
// However, if this sequence contains +/- leap seconds we need to cater for this so, we start looking for the final
// 0b01111110 bit sequence at bit 51.

  if(bitPointer > 57 && getChunk(aBuffer, bitPointer - MSF_MARKER_OFFSET, MSF_MARKER_BITS) == MSF_MARKER)
	{
		TimeReceived = 1;						// an early indicator that data wil be available for processing
		//TimeAvailable = 0;					// clear the user time available flag
		ParityResult = getParity();				// check the parity of the data, Good = 0

// ParityResult return 0 if the parity was good. If the parity check failed the following values are returned
// 1	The Year data parity check failed
// 2	The Month data parity check failed
// 3	The Day of week data parity check failed
// 4	The Time data parity check failed
			
// if the parity is OK, get the data from the "A" buffer into the variables. The MSF data is in BCD so we convert
// it to decimal here for the Time library. You would leave it as BCD for a RTC such as the DS1307

  if(!ParityResult && bitPointer >= MIN_STREAM_LEN)	// make sure there are enough bits to work on e.g. 58 or more seconds worth
	{
		// The number of bits decoded indicates if there was a Leap Second event
		if(bitPointer == 58) LeapSecond = -1;
		else if(bitPointer == 60) LeapSecond = 1;
		else LeapSecond = 0;
		// copy the BCD date & time date from the "A" buffer to the rtcBuffer
		rtcBuffer[MSF_SECOND] = 0;
		rtcBuffer[MSF_MINUTE] = getChunk(aBuffer, bitPointer - MSF_MINUTE_OFFSET,MSF_MINUTE_BITS);	// minute
		rtcBuffer[MSF_HOUR] = getChunk(aBuffer, bitPointer - MSF_HOUR_OFFSET,MSF_HOUR_BITS);		// hour
		rtcBuffer[MSF_DAY] = getChunk(aBuffer, bitPointer - MSF_WEEKDAY_OFFSET,MSF_WEEKDAY_BITS);	// weekday
		rtcBuffer[MSF_DATE] = getChunk(aBuffer, bitPointer - MSF_DATE_OFFSET,MSF_DATE_BITS);		// date
		rtcBuffer[MSF_MONTH] = getChunk(aBuffer, bitPointer - MSF_MONTH_OFFSET,MSF_MONTH_BITS);		// month
		rtcBuffer[MSF_YEAR] = getChunk(aBuffer, bitPointer - MSF_YEAR_OFFSET,MSF_YEAR_BITS);		// year	(offset, number of bits to read)
		TimeTime = makeTime();											// make a time_t compatible for Time/RTC library use
		RxSecs = bitPointer + 1;										// number of seconds received
		Bst = getBit(bBuffer, bitPointer - MSF_BST_BIT_POS);			//BST = 1, GMT = 0
		BstSoon = getBit(bBuffer, bitPointer - MSF_BSTSOON_BIT_POS);	// BST imminent = 1
		getChunk(bBuffer, 1,8);											// get DUT1 Positive bit count
		DutPos = bitCounter * 100;
		getChunk(bBuffer, 9,8);											// get DUT1 Negative bit count
		DutNeg = bitCounter * 100;
		timeIsSet = true;												// set flag for next start of minute
	}
	}
  }
}// End of "msfPulse" Interrupt Routine

/* Everything beyond this point is for decoding and parity checking */

uint8_t MsfTimeLib::getChunk(uint8_t * _buffer, uint8_t _start, uint8_t _numBits)
{
	// return the byte/time 'chunk' containing the number of bits read at the starting
	// bit position in the Raw Buffer up to 8 bits. returns a Byte
	uint8_t chunk = 0;
	uint8_t bitVal = 0;
	uint8_t counter = _numBits - 1;
	bitCounter = 0;				// a count of the number of "1" bits
  
  for(uint16_t i = _start;i < _start + _numBits;i++)		// loop for numBits
	{
		// return the value of the single bit (bitPos) from the "A" buffer
		bitVal = bitRead(_buffer[abs(i/8)], (i % 8) ^ 0x07);	// get the bit from the buffer
		bitWrite(chunk,counter,bitVal);							// write the bit to "chunk"
		if(bitVal) bitCounter++;								// if it's a "1" increment the bitCounter
		counter--;												// decrement the counter
	}
  return chunk;
}

uint8_t MsfTimeLib::getParity()
{
	// calculate the parity bits and return true if all's well
	// all data and parity bits are relative to the last second count copntained in "bitPointer"
	// this allows for leap seconds which are added or removed at second 16 i.e. before
	// the actual date & time data. There can be 59 or 61 seconds in a leep minute
	// Year data parity check
	if(!checkParity(bitPointer - MSF_YEAR_OFFSET, MSF_YEAR_PARITY_BITS, bitPointer - MSF_YEAR_PARITY_BIT_POS)) return 1;
	// Month data parity check
	if(!checkParity(bitPointer - MSF_MONTH_OFFSET, MSF_MONTH_PARITY_BITS, bitPointer - MSF_MONTH_PARITY_BIT_POS)) return 2;
	// Day of week data parity check
	if(!checkParity(bitPointer - MSF_WEEKDAY_OFFSET, MSF_WEEKDAY_PARITY_BITS, bitPointer - MSF_WEEKDAY_PARITY_BIT_POS)) return 3;
	// Time data parity check
	if(!checkParity(bitPointer - MSF_HOUR_OFFSET, MSF_HOUR_PARITY_BITS, bitPointer - MSF_HOUR_PARITY_BIT_POS)) return 4;
	return 0;
}

bool MsfTimeLib::checkParity(uint8_t _start, uint8_t _numBits, uint8_t _parityBitNum)
{
	// count the number of "1" bits in the specified chunk of the "A" buffer
	// then add the parity bit to the result
	uint8_t parity = 0;
  for(uint16_t i = _start;i < _start + _numBits;i++)	// loop for numBits
	{
		// add the bits to "parity"
		parity += getBit(aBuffer, i);
	}
	// now add the actual parity bit to parity
	parity += getBit(bBuffer, _parityBitNum);	// add the parity bit
	if(parity & 0x01) return true;			// return true if parity is negative e.g. Good
	return false;
}

bool MsfTimeLib::getBit(uint8_t * _buffer, uint8_t _bitPos)
{
	// return the value of the single bit (_bitPos) from the _buffer
	// the bits are numbered from MSB (0) to LSB (7)
	return bitRead(_buffer[abs(_bitPos/8)], (_bitPos % 8) ^ 0x07);
}

#define LEAP_YEAR(Y)     ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24UL)

static  const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31}; // API starts months from 1, this array starts from 0

time_t MsfTimeLib::makeTime()
{   
// this function was borrowed and modified from the Time library by Michael Margolis
// note year argument is offset from 1970 (see macros in Time.h to convert to other formats)
// previous version used full four digit year (or digits since 2000),i.e. 2009 was 2009 or 9
  
	int16_t i;
	uint32_t seconds;
	uint8_t Year = bcdToDec(rtcBuffer[MSF_YEAR]);	//decYear;
	uint8_t Month = bcdToDec(rtcBuffer[MSF_MONTH]);
	uint8_t Date = bcdToDec(rtcBuffer[MSF_DATE]);
	uint8_t Hour = bcdToDec(rtcBuffer[MSF_HOUR]);
	uint8_t Minute = bcdToDec(rtcBuffer[MSF_MINUTE]);
  
	if(Year > 99) Year = Year - 1970;
	else Year += 30;

	// seconds from 1970 till 1 jan 00:00:00 of the given year
	seconds= Year*(SECS_PER_DAY * 365);
	for (i = 0; i < Year; i++)
	{
		if (LEAP_YEAR(i)) seconds +=  SECS_PER_DAY;   // add extra days for leap years
	}

	// add days for this year, months start from 1
	for (i = 1; i < Month; i++)
	{
		if ( (i == 2) && LEAP_YEAR(Year)) seconds += SECS_PER_DAY * 29;
		else seconds += SECS_PER_DAY * monthDays[i-1];  //monthDay array starts from 0
	}
	seconds+= (Date-1) * SECS_PER_DAY;
	seconds+= Hour * SECS_PER_HOUR;
	seconds+= Minute * SECS_PER_MIN;
    return seconds; 
}

uint8_t MsfTimeLib::bcdToDec(uint8_t _bcd)	// Convert binary coded decimal to normal decimal numbers
{
  return ( (_bcd/16*10) + (_bcd%16) );
}

uint32_t MsfTimeLib::freeMem(void)
{
// report the free DRAM available for sketches
#ifdef ESP_H
	return ESP.getFreeSketchSpace();
#else
	char top;
	extern char *__brkval;
	extern char __bss_end;
	return( __brkval ? &top - __brkval : &top - &__bss_end);
#endif
}


MsfTimeLib msf = MsfTimeLib();