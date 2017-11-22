/*	jm_CPPM - Combined PPM
	======================

	Material of experimentation: Spektrum DX8 transmitter and Orange R615X receiver (CPPM on Batt/Bind connector)

	Time definition of counter: 16MHz/8 = 2MHz => 0.5us

	The "CPPM servo pulse" starts and ends on 2 falling edges of the CPPM signal.
	A rising edge is generated about 305us after the start of the servo pulse. This is the "CPPM sync pulse".
	The "CPPM synch pulse" starts on a falling edge and ends on a rising edge.
	The width of synch pulse is 305us + an undefined extra time, but no more than 20us as experimented with R615X.
	            ____                            __________________________________________                                _______
	Pulse CPPM: Pn-1\Sn=synch pulse 305us+x=>Pn/Pn...=1520us+/-512us - synch pulse 305us+x\Sn+1=synch pulse 305us+x=>Pn+1/Pn+1...
	Pn-1: previous servo pulse ends on Sn synch pulse
	Sn: Sn synch pulse ends the Pn-1 servo pulse, starts the Pn servo pulse
	Pn: Pn servo pulse continues...
	Sn+1: Sn+1 synch pulse ends the Pn servo pulse, starts the Pn+1 servo pulse
	Pn+1: Pn+1 servo pulse continues...
	            ___    _____    _____    ___    _____    ____    ___
	Frame CPPM: ...\S0/P0-S0\S1/P1-S1\../...\S5/P5-S5\Sg/G-Sg\../... 21980us (43'960 counts +/-10)
	A frame starts and ends on a falling edge (end of Sg)
	A pulse starts on a falling edge (end of Px/Sg) and ends on a falling edge.
	The width of the pulse includes the Sx pulse, starts and ends on the falling edge of Sx negativ pulse.

	S0,S1,S2,S3,S4,S5(,S6,S7,S8),Sg: about 305us (610 counts, varying -0/+40 counts)

	P0,P1,P2,P3,P4,P5(,P6,P7,P8): 1520us (3040 counts, stick at middle position)

	G: synchronization gap (sold of frame), greater than 1520+512us (2032 counts)

	Pulse range: varying +/-100% of a maximum of +/-125% (Spektrum specifications)
	+/-125%: +/-512us (+/-1024 counts)
	+/-100%: +/-409.6us (+/-819.2 counts)

	CPPM conventional channel assignments (Spektrum specifications): aileron - elevator - throttle - rudder - gear - flaps
	aileron, elevator, rudder: stick -100%..0..+100%
	rudder: stick 0..100%
	gear: on/off switch -100%/100%
	flaps (Spektrum DX8): 0/1/2 switch -100%/0/+100%

	2016-04-21: update CPPM_PULSE_SYNC_MIN_FLOOR and CPPM_PULSE_SYNC_MAX_CEIL to accept more various transmitters/receivers, espacially DX5e

	*/

#ifndef jm_CPPM_h
#define jm_CPPM_h

#include <Arduino.h>

//------------------------------------------------------------------------------

// all times are written with a granularity of 1us

#define FRSKY_PULSE_SYNC 300

#define R615X_FRAME_LENGTH 21980 // DX8-R615X 22ms frame length
#define R615X_PULSE_CENTER 1504 // DX8-R615X center of pulse
#define R615X_PULSE_C100PC 413 // DX8-R615X 100% centered stick mouvement
#define R615X_PULSE_C125PC 516 // DX8-R615X 125% centered stick mouvement
#define R615X_PULSE_C150PC 620 // DX8-R615X 150% centered stick mouvement
#define R615X_PULSE_C200PC 826 // DX8-R615X 200% centered stick mouvement

#define R615X_FRAME_NOTSYNC 22765 // R615X not synchronized +/-8us
#define R615X_PULSE_SYNC 304 // R615X neg sync pulse starting PPM pulse
#define R615X_GAP_SYNC 312 // R615X neg sync pulse starting GAP pulse

#define R920X_FRAME_LENGTH 21980 // DX8-R920X 22ms frame length
#define R920X_PULSE_CENTER 1509 // DX8-R920X center of pulse
#define R920X_PULSE_C100PC 455 // DX8-R920X 100% centered stick mouvement
#define R920X_PULSE_C125PC 568 // DX8-R920X 125% centered stick mouvement
#define R920X_PULSE_C150PC 682 // DX8-R920X 150% centered stick mouvement
#define R920X_PULSE_C200PC 909 // DX8-R920X 200% centered stick mouvement

#define R920X_FRAME_NOTSYNC 22121 // R920X not synchronized +/-2us
#define R920X_PULSE_SYNC 138 // R920X neg sync pulse starting PPM pulse
#define R920X_GAP_SYNC 138 // R920X neg sync pulse starting GAP pulse

#define HXT900_DEGREE 11 // HXT900 9gr servo [us/°] (+/-30° standard deviation, +/-60° extended deviation)

//------------------------------------------------------------------------------

#if defined (__AVR_ATmega328P__) // Arduino UNO and compatible (Duemilanove, Diecimila...)
#define CPPM_ICP1 8 // Input Capture Pin - Arduino UNO D8 - ICP1 (ATmega328 PB0)
#define CPPM_OC1A 9 // Output Compare A - Arduino UNO D9 - OC1A (ATmega328 PB1)
//#define CPPM_OC1B 10 // Output Compare B - Arduino UNO D10 - OC1B (ATmega328 PB2) //+2015-08-12
#define CPPM_ICP1_PINB PINB0
#define CPPM_OC1A_PINB PINB1
//#define CPPM_OC1B_PINB PINB2
#elif defined (__AVR_ATmega32U4__) //+2016-06-15,+2016-06-14 Arduino Leonardo and compatible (Yún...)
#define CPPM_ICP1 4 // Input Capture Pin - Arduino Leonardo D4 - ICP1 (ATmega32U4 PD4)
#define CPPM_OC1A 9 // Output Compare A - Arduino Leonardo D9 - OC1A (ATmega32U4 PB5)
//#define CPPM_OC1B 10 // Output Compare B - Arduino Leonardo D10 - OC1B (ATmega32U4 PB6)
//#define CPPM_OC1C 11 // Output Compare C - Arduino Leonardo D11 - OC1C (ATmega32U4 PB7) //+2016-06-15
#define CPPM_ICP1_PIND PIND4
#define CPPM_OC1A_PINB PINB5
//#define CPPM_OC1B_PINB PINB6
#define CPPM_OC1C_PINB PINB7
#else
#error Sorry, your board is not compatible with this library.
#endif

//------------------------------------------------------------------------------

#define CPPM_US_err(us) (2*((uint32_t)us)/100) // +/-2% oscillator error (ATtiny85) of stick error (DX8)

#if 0
#define CPPM_US_mul 2
#define CPPM_US_div 1
#else
#define CPPM_US_mul (clockCyclesPerMicrosecond()/8) // Timer frequency = FCPU/8
#define CPPM_US_div 1
#endif

#define CPPM_US_ceil(us) ((CPPM_US_mul*((uint32_t)us+CPPM_US_err(us))+(CPPM_US_div-1))/CPPM_US_div) // round up
#define CPPM_US_round(us) ((CPPM_US_mul*(uint32_t)us+(CPPM_US_div-1)/2)/CPPM_US_div) // round [us] to [tt]
#define CPPM_US_floor(us) (CPPM_US_mul*((uint32_t)us-CPPM_US_err(us))/CPPM_US_div) // round down

#define CPPM_TT_round(tt) ((CPPM_US_div*(uint32_t)tt+(CPPM_US_mul-1)/2)/CPPM_US_mul) // round [tt] to [us]

//------------------------------------------------------------------------------

#define CPPM_PULSE_SYNC_MIN_FLOOR CPPM_US_floor(100) //(138 - 38) //(FRSKY_PULSE_SYNC - 50) //10)
#define CPPM_PULSE_SYNC_MAX_CEIL CPPM_US_ceil(500) //(312 + 88) //(FRSKY_PULSE_SYNC + 100) //50) // check sync width... (FRSKY_PULSE_SYNC+20) is too short with R615X

//#define CPPM_FRAME_NOTSYNC_MINUS_PULSE_SYNC_CEIL CPPM_US_ceil(R615X_FRAME_NOTSYNC - FRSKY_PULSE_SYNC) // could be a stange frame if wait so long !
//#define CPPM_FRAME_NOTSYNC_CEIL CPPM_US_ceil(R615X_FRAME_NOTSYNC) // 2% max oscillator error
#define CPPM_FRAME_NOTSYNC_CEIL CPPM_US_ceil(22765 + 135) //(R615X_FRAME_NOTSYNC + 100) // 2% max oscillator error

#define CPPM_PULSE_CENTER 1500 // middle stick
#define CPPM_PULSE_CENTER_PLUS_C200PC_CEIL CPPM_US_ceil(1500 + 909 + 81) //(R615X_PULSE_CENTER + R615X_PULSE_C200PC) // middle stick+200%
#define CPPM_PULSE_CENTER_MINUS_C200PC_FLOOR CPPM_US_floor(1500 - 909 - 81) //(1500 (R615X_PULSE_CENTER - R615X_PULSE_C200PC) // too short servo pulse (middle stick-200%) ?

#define CPPM_FRAME_LENGTH_FLOOR CPPM_US_floor(21980 - 980) //(R615X_FRAME_LENGTH) // frame length too short ?

#define CPPM_MSERVO 9 // 9 servos maximum in a 22ms frame

enum {CPPM_AILE, CPPM_ELEV, CPPM_THRO, CPPM_RUDD, CPPM_GEAR, CPPM_AUX1, CPPM_AUX2, CPPM_AUX3, CPPM_AUX4};
// standard definitions of Spektrum receivers (AR6200, R615X, R920X,...) CPPM output (BIND connector)

enum {CPPM_THR, CPPM_AIL, CPPM_ELE, CPPM_RUD, CPPM_RC5, CPPM_RC6, CPPM_RC7, CPPM_RC8, CPPM_RC9};
// standard definitions of Spektrum transmitters (DX5e, DX6i, DX8, DX9,...) Trainer output (Jack 3.5mm connector)

//------------------------------------------------------------------------------

enum CPPM_begin_t {
	CPPM_INP,
	CPPM_OUT1,
	CPPM_OUT2,
#if defined CPPM_OC1C //+2016-06-15
	CPPM_OUT3,
#endif
};

//------------------------------------------------------------------------------

class jm_CPPM_Class
{
	private:

	public:

		uint8_t _state; // state of synchronization: 0=no signal or errored, 1=start frame, 2=synchronized
		uint8_t _errors; // count of frame errors

		uint16_t _time0; // start time of sync pulse (falling edge)
		uint16_t _time1; // end time of sync pulse (rising edge)
		uint16_t _sync2; // width of sync pulse
		uint16_t _puls3; // width of servo pulse (including starting sync pulse)
		uint16_t _cppm4; // length of CPPM frame
		uint16_t _time5; // start time of CPPM frame (start time of 1st sync pulse)

		uint8_t _iservo; // index servo
		uint8_t _nservo; // found servos
		uint8_t _jservo; // next index servo
//		uint8_t _kservo; // next mask servo

		uint16_t _sync2s[CPPM_MSERVO + 2]; // [tt] width of synch pulses
		uint16_t _puls3s[CPPM_MSERVO + 2]; // [tt] width of servo pulses
		// the 2 extra servos are the gap pulse and the total frame length.

		int8_t _puls3i8[CPPM_MSERVO + 2]; // [i8] width of servo pulses

		bool _begun0;
		bool _received; // +2015-02-05


		uint8_t _oservo1; // cppm output servo index
		uint16_t _oservos1[CPPM_MSERVO]; // [tt] cppm output servo pulse width
		bool _begun1;
		bool _sent1;	// +2015-06-23


		uint8_t _oservo2; // cppm output servo index // +2015-08-02
		uint16_t _oservos2[CPPM_MSERVO]; // [tt] cppm output servo pulse width // +2015-08-12
		bool _begun2;
		bool _sent2;	// +2015-08-12


#if defined CPPM_OC1C //+2016-06-15
		uint8_t _oservo3; // cppm output servo index // +2015-08-02
		uint16_t _oservos3[CPPM_MSERVO]; // [tt] cppm output servo pulse width // +2015-08-12
		bool _begun3;
		bool _sent3;	// +2016-06-15
#endif

	public:

		bool synchronized(void);

		bool received(void);

		int nservo(void);

		uint16_t sync_tt(int n);
		uint16_t sync_us(int n);

		uint16_t read_tt(int n);
		uint16_t read_us(int n);

		void read_begin();
		void read_end();


		void write1_tt(int n, uint16_t tt);
		void write1_us(int n, uint16_t us);

		bool write1_sent(void);

		void write1_begin();
		void write1_end();


		void write2_tt(int n, uint16_t tt);
		void write2_us(int n, uint16_t us);

		bool write2_sent(void);

		void write2_begin();
		void write2_end();


#if defined CPPM_OC1C //+2016-06-15

		void write3_tt(int n, uint16_t tt);
		void write3_us(int n, uint16_t us);

		bool write3_sent(void);

		void write3_begin();
		void write3_end();

#endif


		void begin(void);
		void begin(int begin_mask);

		void end(void);
		void end(int begin_mask);

		void cycle(void);

		operator bool();

		jm_CPPM_Class();
};

extern jm_CPPM_Class CPPM;

#endif
