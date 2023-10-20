#ifndef MsfAvrDefinitions_h
#define MsfAvrDefinitions_h

// AVR & ESP8266 interrupt pin assignment examples
/*
		Board	int.0	int.1	int.2	int.3	int.4	int.5
Uno, Ethernet	2		3	 	 	 	 
Mega2560		2		3		21		20		19		18
ATmega1284		10		11		3

ESP8266 Interrupt = GPIO number (except for GPIO16 which does not support interrupts)

All digital pin numbers are the Arduino "Dx" pin numbers.

The following definitions are used during the "begin" method for
"interrupt to pin digital pin mapping".

*/
#if defined (__AVR_ATmega8__) || defined(__AVR_ATmega48__) || defined (__AVR_ATmega48P__) || defined (__AVR_ATmega88__) || defined (__AVR_ATmega88P__) || defined (__AVR_ATmega168__) || defined (__AVR_ATmega168P__) || defined (__AVR_ATmega328P__)
	#define MSF_BOARD_TYPE 			"UNO/NANO/PRO MINI ETC."
	#define MSF_AVR_TYPE 			"ATmega8/48/88/168/328(P)"
	#define MSF_INT_PINS 			2
	// these are the Digital IO pin numbers (Dx) of the relevant Arduino board
	#define MSF_INT_DIG_PIN0 		2
	#define MSF_INT_DIG_PIN1 		3
	#define MSF_INT_DIG_PIN2 		0
	#define MSF_INT_DIG_PIN3 		0
	#define MSF_INT_DIG_PIN4 		0
	#define MSF_INT_DIG_PIN5 		0
	
#elif defined (__AVR_ATmega640__) || defined (__AVR_ATmega1280__) || defined (__AVR_ATmega2560__)
	#define MSF_BOARD_TYPE 			"Mega2560"
	#define MSF_AVR_TYPE 			"ATmega640/1280/2560"
	#define MSF_INT_PINS 			6
	// these are the Digital IO pin numbers (Dx) of the relevant Arduino board
	#define MSF_INT_DIG_PIN0 		2
	#define MSF_INT_DIG_PIN1 		3
	#define MSF_INT_DIG_PIN2 		21
	#define MSF_INT_DIG_PIN3 		20
	#define MSF_INT_DIG_PIN4 		19
	#define MSF_INT_DIG_PIN5 		18
	
#elif defined (__AVR_ATmega164P__) || defined (__AVR_ATmega324P__)|| defined (__AVR_ATmega644__) || defined (__AVR_ATmega1284__)
	#define MSF_BOARD_TYPE 			"NA"
	#define MSF_AVR_TYPE 			"ATmega164/324/644/1284"
	#define MSF_INT_PINS 			3
	// these are the Digital IO pin numbers (Dx) of the relevant Arduino board
	#define MSF_INT_DIG_PIN0 		10
	#define MSF_INT_DIG_PIN1 		11
	#define MSF_INT_DIG_PIN2 		3
	#define MSF_INT_DIG_PIN3 		0
	#define MSF_INT_DIG_PIN4 		0
	#define MSF_INT_DIG_PIN5 		0
	
#elif defined ESP_H
	#define MSF_BOARD_TYPE 			"NOT APPLICABLE"
	#define MSF_AVR_TYPE 			"ESP8266"
	#define MSF_INT_PINS 			7	// ESP8266-12
	/*
	// these are the Digital IO pin numbers (Dx) of the relevant Arduino board
	#define MSF_INT_DIG_PIN0 		0
	#define MSF_INT_DIG_PIN1 		0
	#define MSF_INT_DIG_PIN2 		0
	#define MSF_INT_DIG_PIN3 		0
	#define MSF_INT_DIG_PIN4 		0
	#define MSF_INT_DIG_PIN5 		0
	*/
#else
	#define MSF_BOARD_TYPE 			"NOT APPLICABLE"
	#define MSF_AVR_TYPE 			"UNDEFINED"
	#define MSF_INT_PINS 			0	
	// these are the Digital IO pin numbers (Dx) of the relevant Arduino board
	#define MSF_INT_DIG_PIN0 		0
	#define MSF_INT_DIG_PIN1 		0
	#define MSF_INT_DIG_PIN2 		0
	#define MSF_INT_DIG_PIN3 		0
	#define MSF_INT_DIG_PIN4 		0
	#define MSF_INT_DIG_PIN5 		0
	
#endif










#endif