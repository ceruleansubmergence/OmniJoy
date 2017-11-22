
#include <jm_CPPM.h>
#include <jm_LiquidCrystal_I2C.h>

jm_LiquidCrystal_I2C lcd;

void cppm_cycle(void)
{
	if (CPPM.synchronized() && CPPM.received())
	{

#if 0
		int aile = CPPM.read_us(CPPM_AILE) - 1500; // aile
		int elev = CPPM.read_us(CPPM_ELEV) - 1500; // elevator
		int thro = CPPM.read_us(CPPM_THRO) - 1500; // throttle
		int rudd = CPPM.read_us(CPPM_RUDD) - 1500; // rudder
		int gear = CPPM.read_us(CPPM_GEAR) - 1500; // gear
		int aux1 = CPPM.read_us(CPPM_AUX1) - 1500; // flap

		Serial.print(aile); Serial.print(", ");
		Serial.print(elev); Serial.print(", ");
		Serial.print(thro); Serial.print(", ");
		Serial.print(rudd); Serial.print(", ");
		Serial.print(gear); Serial.print(", ");
		Serial.print(aux1); Serial.print("\n");
#else
		for (int i=0; i<(CPPM_MSERVO + 2); i++)
		{
			Serial.print('\t');
			Serial.print(CPPM.sync_us(i)); // [us] width of sync pulses
			if (i<10)
			{
				lcd.set_cursor(0+i*4%20, 0+i/5);
				lcd.print_u16(CPPM.sync_us(i), 4);
			}
		}
		Serial.println();

		for (int i=0; i<(CPPM_MSERVO + 2); i++)
		{
			Serial.print('\t');
			Serial.print(CPPM.read_us(i)); // [us] width of servo pulses
			if (i<8)
			{
				lcd.set_cursor(0+i*5%20, 2+i/4);
				lcd.print_u16(CPPM.read_us(i), 5);
			}
		}
		Serial.println();
#endif

		Serial.flush();

		delay(100);

		CPPM.received();
	}
	else
	{
		// if not synchronized, do something...
	}
}

void setup()
{
	Serial.begin(115200);
//	while(!Serial);

	lcd.begin();

//	CPPM.begin();

//	CPPM.write2_end();

	CPPM.begin( (1<<CPPM_OUT3) | (0<<CPPM_OUT2) | (1<<CPPM_OUT1) | (1<<CPPM_INP) ); //(0b1011);

	CPPM.end( (1<<CPPM_OUT1) );

	CPPM.write1_us(0, 500);
	CPPM.write2_us(1, 500);
	CPPM.write3_us(2, 500);
}

void loop()
{
	cppm_cycle();
}
