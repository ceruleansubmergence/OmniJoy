
#include <jm_CPPM.h>

//#include <avr/io.h>
//#include <Arduino.h>

//------------------------------------------------------------------------------

void iservos_ICP1_reset(bool errored);

//------------------------------------------------------------------------------

volatile uint16_t CPPM_T_W; // extended TCNT0 timer maintained by cycle function
volatile uint16_t CPPM_T_X; // extended TCNT0 timer maintained by cycle function
volatile uint16_t CPPM_T_T; // extended TCNT0 timer timeout

volatile bool CPPM_T_cycling; // GPIOR0.0 = CPPM_T_cycle() in action, could be stopped by CPPM_T_interrupt()
volatile bool CPPM_T_syncing; // GPIOR0.1 = CPPM sync falling edge detected
volatile bool CPPM_T_checking; // GPIOR0.2 = CPPM check timeout

//------------------------------------------------------------------------------

uint16_t CPPM_T_read()
{
	uint16_t tcnt_x;
	cli();
	tcnt_x = TCNT1;
	sei();
	return tcnt_x;
}

uint16_t CPPM_T_get()
{
	uint16_t tcnt_x;
	cli();
	tcnt_x = CPPM_T_X;
	sei();
	return tcnt_x;
}

void CPPM_T_set(uint16_t tcnt_x)
{
	cli();
	CPPM_T_X = tcnt_x;
	sei();
}

void CPPM_T_cycle()
{
	CPPM_T_cycling = true;
	cli();
	CPPM_T_W = TCNT1;
	sei();
	cli();
	if (CPPM_T_cycling) CPPM_T_X = CPPM_T_W;
	sei();
	CPPM_T_cycling = false;
}

uint16_t CPPM_T_interrupt()
{
	uint16_t tcnt_x;
	tcnt_x = ICR1;
	CPPM_T_X = tcnt_x;
	CPPM_T_W = tcnt_x;
	CPPM_T_cycling = false;
	return tcnt_x;
}

uint16_t CPPM_T_timeout()
{
	uint16_t tcnt_x;
	cli();
	tcnt_x = CPPM_T_X - CPPM_T_T;
	sei();
	return tcnt_x;
}

void CPPM_T_check()
{
	if (CPPM_T_checking && (uint8_t) (CPPM_T_timeout()>>8)==0) iservos_ICP1_reset(true);
}

bool CPPM_T_inited = false;

void CPPM_T_setup()
{
	if (CPPM_T_inited) return;

	uint16_t tcnt_x = CPPM_T_read();

	CPPM_T_W = CPPM_T_X = CPPM_T_T = tcnt_x;

	ICR1 = tcnt_x; // init Input Capture Register

	OCR1A = tcnt_x; // init Output Compare Register A

//	OCR1B = tcnt_x; // init Output Compare Register B

#if defined CPPM_OC1C //+2016-06-15
	OCR1C = tcnt_x; // init Output Compare Register C
#endif

	// Configure Timer/Counter1: disable PWM, set prescaler /8 (0.5[us]@16MHz)
#if defined CPPM_OC1C //+2016-06-15
	TCCR1A = (1<<COM1A0) | (1<<COM1B0) | (1<<COM1C0);	// COM1A=Toggle | COM1B=Toggle | COM1C=Toggle | WGM=Normal%4
#else
	TCCR1A = (1<<COM1A0);// | (1<<COM1B0);	// COM1A=Toggle | COM1B=Toggle | WGM=Normal%4
#endif
	TCCR1B = (1<<ICNC1) | (0<<ICES1) | (0<<WGM12) | (1<<CS11); // ICNC=Enabled | ICES=Falling | WGM=Normal/4 | clkIO=8
	TCCR1C = 0; // no action.

	CPPM_T_inited = true;
}

//------------------------------------------------------------------------------

void iservos_ICP1_reset(bool errored)
{
	// disable "pin change" interrupt from CPPM input frame...
//	cbi(PCMSK,PCINT4);
	bitClear(TIMSK1, ICIE1); // disable interrupt

//	cbi(GPIOR0,CPPM_T_syncing);
//	cbi(GPIOR0,CPPM_T_checking);
	CPPM_T_syncing = false;
	CPPM_T_checking = false;

//	PORTB = (PORTB & ~CPPM_PBMASK); // clear PWM channels

	CPPM._state = 0;
	if (errored) CPPM._errors++; else CPPM._errors = 0;
	CPPM._iservo = 0;
	CPPM._nservo = CPPM_MSERVO;
	CPPM._jservo = 0;
//	CPPM._kservo = 0;

	// enable "pin change" interrupt from CPPM input frame...
//	sbi(PCMSK,PCINT4);
	// Enable Timer1 input capture interrupt...
//	TCCR1B = (1<<ICNC1) | (0<<ICES1) | (1<<CS11); // falling edge
	TCCR1B = (1<<ICNC1) | (0<<ICES1) | (0<<WGM12) | (1<<CS11); // ICNC=Enabled | ICES=Falling | WGM=Normal/4 | clkIO=8
	bitSet(TIFR1, ICF1); // clr pending interrupt
	bitSet(TIMSK1, ICIE1); // enable interrupt
}

//------------------------------------------------------------------------------

//ISR(PCINT0_vect)
ISR(TIMER1_CAPT_vect)
{
	uint16_t tcnt0 = CPPM_T_interrupt(); // get time from extended TCNT0 timer

//	if (PB_tst(PB4)) // ? rising edge => end 300us synchro pulse
	if (TCCR1B & (1<<ICES1)) // rising edge => end 300us synchro pulse ?
	{
		TCCR1B = (1<<ICNC1) | (0<<ICES1) | (1<<CS11); // next falling edge

//		if (tbi(GPIOR0,CPPM_T_syncing)) // ? follow a start edge
		if (CPPM_T_syncing) // ? follow a start edge
		{
//			cbi(GPIOR0,CPPM_T_syncing);
			CPPM_T_syncing = false;

			CPPM._sync2 = tcnt0 - CPPM._time0; // compute width of synch pulse
			CPPM._time1 = tcnt0;

			CPPM._sync2s[CPPM._iservo] = CPPM._sync2; // store sync width of current PWM servo pulse

//			if(	CPPM._sync2 < CPPM_US_floor(FRSKY_PULSE_SYNC-10) ||
//				CPPM._sync2 > CPPM_US_ceil(FRSKY_PULSE_SYNC+50) ) // check sync width... (FRSKY_PULSE_SYNC+20) is too short with R615X
			if(	CPPM._sync2 < CPPM_PULSE_SYNC_MIN_FLOOR ||
				CPPM._sync2 > CPPM_PULSE_SYNC_MAX_CEIL )
			{
				iservos_ICP1_reset(true);
			}
			else
			{
				if (CPPM._state==0) CPPM._state = 1; // 1st well formed sync pulse found.

				if (CPPM._state==1) // ? get pulse until gap pulse found
				{
//					CPPM_T_T = tcnt0 + CPPM_US_ceil(R615X_FRAME_NOTSYNC-FRSKY_PULSE_SYNC); // could be a stange frame if wait so long !
					CPPM_T_T = CPPM._time5 + CPPM_FRAME_NOTSYNC_CEIL; // could be a stange frame if wait so long !
				}
				else // : gap pulse found => start time of frame known.
				if (CPPM._state==2) // ? get pulses and compute _nservo.
				{
//					CPPM_T_T = CPPM._time5 + CPPM_US_ceil(R615X_FRAME_NOTSYNC); // 2% max oscillator error
					CPPM_T_T = CPPM._time5 + CPPM_FRAME_NOTSYNC_CEIL; // set frame timeout
				}
				else // : check pulses and gap.
				{
					if (CPPM._iservo<CPPM._nservo)
//						CPPM_T_T = CPPM._time0 + CPPM_US_ceil(R615X_PULSE_CENTER+R615X_PULSE_C150PC); // middle stick+150%
//						CPPM_T_T = CPPM._time0 + CPPM_US_ceil(R615X_PULSE_CENTER+R615X_PULSE_C200PC); // middle stick+200%
						CPPM_T_T = CPPM._time0 + CPPM_PULSE_CENTER_PLUS_C200PC_CEIL; // middle stick+200%
					else
//						CPPM_T_T = CPPM._time5 + CPPM_US_ceil(R615X_FRAME_NOTSYNC);
						CPPM_T_T = CPPM._time5 + CPPM_FRAME_NOTSYNC_CEIL;
				}
//				sbi(GPIOR0,CPPM_T_checking);
				CPPM_T_syncing = true;
			}
		}
	}
	else // : falling edge => start 300us sync pulse and start PWM servo pulse.
	{
		TCCR1B = (1<<ICNC1) | (1<<ICES1) | (1<<CS11); // next rising edge

//		if (_state==3) PORTB = (PORTB & ~CPPM_PBMASK) | (_kservo & CPPM_PBMASK); // update PWM channels, stop current pulse, start next pulse

		CPPM._received = false;

//		sbi(GPIOR0,CPPM_T_syncing);
		CPPM_T_syncing = true;

//		CPPM_T_T = tcnt0 + CPPM_US_ceil(FRSKY_PULSE_SYNC+50);
		CPPM_T_T = tcnt0 + CPPM_PULSE_SYNC_MAX_CEIL;
//		sbi(GPIOR0,CPPM_T_checking);
		CPPM_T_checking = true;

		if (CPPM._state==0)
		{
			CPPM._time5 = CPPM._time0 = tcnt0; // set start time of next PWM servo pulse (and sync pulse)
		}
		else // : _state>=1
		{
			CPPM._puls3 = tcnt0 - CPPM._time0; // compute width of elapsed pulse
			CPPM._time0 = tcnt0; // set start time of next PWM servo pulse (and sync pulse)

			CPPM._puls3s[CPPM._iservo] = CPPM._puls3; // store width of servo pulse

			int puls3i = ((signed) CPPM._puls3 - R615X_PULSE_CENTER) / 4; // middle centered servo pulse
//			int puls3i = (signed) CPPM._puls3 / 4 - (CPPM._sync2 + CPPM._sync2/4); // middle centered servo pulse
			if (puls3i > 127) puls3i = 127; else if (puls3i < -128) puls3i= -128;
			CPPM._puls3i8[CPPM._iservo] = puls3i;

//			if (CPPM._puls3 < CPPM_US_floor(R615X_PULSE_CENTER-R615X_PULSE_C150PC)) // too short servo pulse (middle stick-150%) ?
//			if (CPPM._puls3 < CPPM_US_floor(R615X_PULSE_CENTER-R615X_PULSE_C200PC)) // too short servo pulse (middle stick-200%) ?
			if (CPPM._puls3 < CPPM_PULSE_CENTER_MINUS_C200PC_FLOOR) // too short servo pulse (middle stick-200%) ?
			{
				iservos_ICP1_reset(true);
			}
			else
//			if (CPPM._puls3 > CPPM_US_ceil(R615X_PULSE_CENTER+R615X_PULSE_C150PC)) // is a gap pulse (middle stick+150%) ?
//			if (CPPM._puls3 > CPPM_US_ceil(R615X_PULSE_CENTER+R615X_PULSE_C200PC)) // is a gap pulse (middle stick+200%) ?
			if (CPPM._puls3 > CPPM_PULSE_CENTER_PLUS_C200PC_CEIL) // is a gap pulse (middle stick+200%) ?
			{
				CPPM._cppm4 = tcnt0 - CPPM._time5; // compute length of elapsed CPPM frame
				CPPM._time5 = tcnt0; // set start time of next CPPM frame

				CPPM._puls3s[CPPM._iservo+1] = CPPM._cppm4; // store CPPM frame length
				CPPM._sync2s[CPPM._iservo+1] = 0;

//				if(	(CPPM._state==3 && CPPM._cppm4 < CPPM_US_floor(R615X_FRAME_LENGTH)) || // frame length too short ?
				if(	(CPPM._state==3 && CPPM._cppm4 < CPPM_FRAME_LENGTH_FLOOR) || // frame length too short ?
//					CPPM._cppm4 > CPPM_US_ceil(R615X_FRAME_NOTSYNC) ) // frame length too long ?
					CPPM._cppm4 > CPPM_FRAME_NOTSYNC_CEIL ) // frame length too long ?
				{
					iservos_ICP1_reset(true);
				}
				else
				{
					if (CPPM._state==2) CPPM._nservo = CPPM._iservo;

					CPPM._iservo = 0; // set crnt servo (1st)
					CPPM._jservo = 1; // set next servo (2nd)
//					CPPM._kservo = CPPM.lservo[1]; // set mask of next servo

					if (CPPM._state < 3) CPPM._state++;
				}
			}
			else // valid servo pulse.
			{
				CPPM._iservo = CPPM._jservo; // set index of current servo pulse

				if (CPPM._jservo > CPPM_MSERVO) // servos overflow ?
				{
					iservos_ICP1_reset(true);
				}
				else
				if (CPPM._jservo > CPPM._nservo) // servos overflow ?
				{
					iservos_ICP1_reset(true);
				}
				else
				if (CPPM._jservo == CPPM._nservo)
				{
					CPPM._jservo = 0;
//					_kservo = lservo[0]; // set next mask of 1st servo

					CPPM._received = true;
				}
				else
				{
					CPPM._jservo++;
//					_kservo = lservo[_jservo]; // set next mask of next servo
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

bool jm_CPPM_Class::synchronized()
{
	return _state == 3;
}

bool jm_CPPM_Class::received(void)
{
	bool received = _received;
	_received = false;
	return received;
}

int jm_CPPM_Class::nservo(void)
{
	return _nservo;
}

//------------------------------------------------------------------------------

uint16_t jm_CPPM_Class::sync_tt(int n)
{
	uint16_t *sync_p = &_sync2s[n];
	cli();
	uint16_t tt = *sync_p;
	sei();
	return tt;
}

uint16_t jm_CPPM_Class::sync_us(int n)
{
	return CPPM_TT_round(sync_tt(n));
}

//------------------------------------------------------------------------------

uint16_t jm_CPPM_Class::read_tt(int n)
{
	uint16_t *servo2_p = &_puls3s[n];
	cli();
	uint16_t tt = *servo2_p;
	sei();
	return tt;
}

uint16_t jm_CPPM_Class::read_us(int n)
{
	return CPPM_TT_round(read_tt(n));
}

//------------------------------------------------------------------------------

void jm_CPPM_Class::read_begin()
{
	if (_begun0) return;

	// Configure the input capture pin
	pinMode(CPPM_ICP1, INPUT_PULLUP);

	iservos_ICP1_reset(false);
}

void jm_CPPM_Class::read_end()
{
	if (!_begun0) return;

	bitClear(TIMSK1, ICIE1); // disable interrupt
}

//------------------------------------------------------------------------------

ISR(TIMER1_COMPA_vect) // *2015-06-05,+2015-02-05
{
	static uint16_t _time5 = 0;

	if (CPPM._oservo1 < CPPM_MSERVO) // PPM pulse ?
	{
		if ((PINB & _BV(CPPM_OC1A_PINB))) // rising edge ?
		{
			OCR1A += CPPM._oservos1[CPPM._oservo1] - CPPM_US_round(R615X_PULSE_SYNC);

			CPPM._oservo1++; // next PPM pulse (or gap)
		}
		else // falling edge.
		{
			if (CPPM._oservo1 == 0) _time5 = OCR1A;

			OCR1A += CPPM_US_round(R615X_PULSE_SYNC);

//			CPPM._sent1 = false;
		}
	}
	else // gap pulse.
	{
		if ((PINB & _BV(CPPM_OC1A_PINB))) // rising edge ?
		{
			OCR1A = _time5 + CPPM_US_round( R615X_FRAME_LENGTH );

			CPPM._oservo1 = 0; // next PPM pulse
		}
		else // falling edge.
		{
			OCR1A += CPPM_US_round(R615X_GAP_SYNC);

			CPPM._sent1 = true;
		}
	}
}

void jm_CPPM_Class::write1_tt(int n, uint16_t tt)
{
	uint16_t *oservo_p = &_oservos1[n];
	cli();
	*oservo_p = tt;
	sei();
}

void jm_CPPM_Class::write1_us(int n, uint16_t us)
{
	write1_tt(n, CPPM_US_round(us));
}

bool jm_CPPM_Class::write1_sent(void)
{
	bool write1_sent = _sent1;
	_sent1 = false;
	return write1_sent;
}

void jm_CPPM_Class::write1_begin()
{
	if (_begun1) return;

	CPPM_T_setup();

	// Configure the output compare pin
	digitalWrite(CPPM_OC1A, HIGH);
	pinMode(CPPM_OC1A, OUTPUT);

	CPPM._oservo1 = 0;
	for (int i=0; i<CPPM_MSERVO; i++) CPPM._oservos1[i] = CPPM_US_round(R615X_PULSE_CENTER);

	// start CPPM frame after 22ms...
//	OCR1A += CPPM_US_round(R615X_FRAME_LENGTH);
	OCR1A = CPPM_T_read() + CPPM_US_round(R615X_FRAME_LENGTH);

	// Enable Timer1 output compare interrupt...
	bitSet(TIFR1, OCF1A); // clr pending interrupt
	bitSet(TIMSK1, OCIE1A); // enable interrupt

	_begun1 = true;
}

void jm_CPPM_Class::write1_end()
{
	if (!_begun1) return;

	TCCR1A = TCCR1A & ~(0b11<<COM1A0); // disconnect OC

	bitClear(TIMSK1, OCIE1A); // disable interrupt

	digitalWrite(CPPM_OC1A, HIGH); // set output

	_begun1 = false;
}

//------------------------------------------------------------------------------

ISR(TIMER1_COMPB_vect) // +2015-08-12
{
/*
	static uint16_t _time5 = 0;

	if (CPPM._oservo2 < CPPM_MSERVO) // PPM pulse ?
	{
		if ((PINB & _BV(CPPM_OC1B_PINB))) // rising edge ?
		{
			OCR1B += CPPM._oservos2[CPPM._oservo2] - CPPM_US_round(R615X_PULSE_SYNC);

			CPPM._oservo2++; // next PPM pulse (or gap)
		}
		else // falling edge.
		{
			if (CPPM._oservo2 == 0) _time5 = OCR1B;

			OCR1B += CPPM_US_round(R615X_PULSE_SYNC);

//			CPPM._sent2 = false;
		}
	}
	else // gap pulse.
	{
		if ((PINB & _BV(CPPM_OC1B_PINB))) // rising edge ?
		{
			OCR1B = _time5 + CPPM_US_round( R615X_FRAME_LENGTH );

			CPPM._oservo2 = 0; // next PPM pulse
		}
		else // falling edge.
		{
			OCR1B += CPPM_US_round(R615X_GAP_SYNC);

			CPPM._sent2 = true;
		}
	}
*/
}

void jm_CPPM_Class::write2_tt(int n, uint16_t tt)
{
	uint16_t *oservo2_p = &_oservos2[n];
	cli();
	*oservo2_p = tt;
	sei();
}

void jm_CPPM_Class::write2_us(int n, uint16_t us)
{
	write2_tt(n, CPPM_US_round(us));
}

bool jm_CPPM_Class::write2_sent(void)
{
	bool write2_sent = _sent2;
	_sent2 = false;
	return write2_sent;
}

void jm_CPPM_Class::write2_begin() // +2015-08-02
{
/*
	if (_begun2) return;

	CPPM_T_setup();

	// Configure the output compare pin
	digitalWrite(CPPM_OC1B, HIGH);
	pinMode(CPPM_OC1B, OUTPUT);

	CPPM._oservo2 = 0;
	for (int i=0; i<CPPM_MSERVO; i++) CPPM._oservos2[i] = CPPM_US_round(R615X_PULSE_CENTER);

	// start CPPM frame after 22ms...
//	OCR1B += CPPM_US_round(R615X_FRAME_LENGTH);
	OCR1B = CPPM_T_read() + CPPM_US_round(R615X_FRAME_LENGTH);

	// Enable Timer1 output compare interrupt...
	bitSet(TIFR1, OCF1B); // clr pending interrupt
	bitSet(TIMSK1, OCIE1B); // enable interrupt

	_begun2 = true;
*/
}

void jm_CPPM_Class::write2_end()
{
/*
	if (!_begun2) return;

	TCCR1A = TCCR1A & ~(0b11<<COM1B0); // disconnect OC

	bitClear(TIMSK1, OCIE1B); // disable interrupt

	digitalWrite(CPPM_OC1B, HIGH); // set output

	_begun2 = false;
*/
}

//------------------------------------------------------------------------------

#if defined CPPM_OC1C //+2016-06-15

ISR(TIMER1_COMPC_vect) // +2016-06-15
{
	static uint16_t _time5 = 0;

	if (CPPM._oservo3 < CPPM_MSERVO) // PPM pulse ?
	{
		if ((PINB & _BV(CPPM_OC1C_PINB))) // rising edge ?
		{
			OCR1C += CPPM._oservos3[CPPM._oservo3] - CPPM_US_round(R615X_PULSE_SYNC);

			CPPM._oservo3++; // next PPM pulse (or gap)
		}
		else // falling edge.
		{
			if (CPPM._oservo3 == 0) _time5 = OCR1C;

			OCR1C += CPPM_US_round(R615X_PULSE_SYNC);

//			CPPM._sent3 = false;
		}
	}
	else // gap pulse.
	{
		if ((PINB & _BV(CPPM_OC1C_PINB))) // rising edge ?
		{
			OCR1C = _time5 + CPPM_US_round( R615X_FRAME_LENGTH );

			CPPM._oservo3 = 0; // next PPM pulse
		}
		else // falling edge.
		{
			OCR1C += CPPM_US_round(R615X_GAP_SYNC);

			CPPM._sent3 = true;
		}
	}
}

void jm_CPPM_Class::write3_tt(int n, uint16_t tt)
{
	uint16_t *oservo3_p = &_oservos3[n];
	cli();
	*oservo3_p = tt;
	sei();
}

void jm_CPPM_Class::write3_us(int n, uint16_t us)
{
	write3_tt(n, CPPM_US_round(us));
}

bool jm_CPPM_Class::write3_sent(void)
{
	bool write3_sent = _sent3;
	_sent3 = false;
	return write3_sent;
}

void jm_CPPM_Class::write3_begin()
{
	if (_begun3) return;

	CPPM_T_setup();

	// Configure the output compare pin
	digitalWrite(CPPM_OC1C, HIGH);
	pinMode(CPPM_OC1C, OUTPUT);

	CPPM._oservo3 = 0;
	for (int i=0; i<CPPM_MSERVO; i++) CPPM._oservos3[i] = CPPM_US_round(R615X_PULSE_CENTER);

	// start CPPM frame after 22ms...
//	OCR1C += CPPM_US_round(R615X_FRAME_LENGTH);
	OCR1C = CPPM_T_read() + CPPM_US_round(R615X_FRAME_LENGTH);

	// Enable Timer1 output compare interrupt...
	bitSet(TIFR1, OCF1C); // clr pending interrupt
	bitSet(TIMSK1, OCIE1C); // enable interrupt

	_begun3 = true;
}

void jm_CPPM_Class::write3_end()
{
	if (!_begun3) return;

	TCCR1A = TCCR1A & ~(0b11<<COM1C0); // disconnect OC

	bitClear(TIMSK1, OCIE1C); // disable interrupt

	digitalWrite(CPPM_OC1C, HIGH); // set output

	_begun3 = false;
}

#endif

//------------------------------------------------------------------------------

void jm_CPPM_Class::begin()
{
	begin(-1);
}

void jm_CPPM_Class::begin(int begin_mask)
{
	CPPM_T_setup();

//	if (begin_mask & (1<<CPPM_INP)) read_begin();

	if (begin_mask & (1<<CPPM_OUT1)) write1_begin();

//	if (begin_mask & (1<<CPPM_OUT2)) write2_begin();

#if defined CPPM_OC1C //+2016-06-15
//	if (begin_mask & (1<<CPPM_OUT3)) write3_begin();
#endif
}

void jm_CPPM_Class::end()
{
	end(-1);
}

void jm_CPPM_Class::end(int begin_mask)
{
#if defined CPPM_OC1C //+2016-06-15
//	if (begin_mask & (1<<CPPM_OUT3)) write3_end();
#endif

//	if (begin_mask & (1<<CPPM_OUT2)) write2_end();

	if (begin_mask & (1<<CPPM_OUT1)) write1_end();

//	if (begin_mask & (1<<CPPM_INP)) bitClear(TIMSK1, ICIE1); // disable interrupt
	if (begin_mask & (1<<CPPM_INP)) read_end();
}

void jm_CPPM_Class::cycle()
{
	CPPM_T_cycle();
	CPPM_T_check();
}

jm_CPPM_Class::operator bool()
{
	return true;
}

jm_CPPM_Class::jm_CPPM_Class()
{
	_begun0 = false;
	_begun1 = false;
	_begun2 = false;
#if defined CPPM_OC1C //+2016-06-15
	_begun3 = false;
#endif
}

//------------------------------------------------------------------------------

jm_CPPM_Class CPPM;
