
#include <CPPM.h>

void cppm_cycle(void)
{
	if (CPPM.synchronized() && CPPM.received())
	{
//		// good for DX8-R615X
//		int aile = (CPPM.read(CPPM_AILE) - 1500*2) / 8 * 125 / 128; // aile -100% .. +100%
//		int elev = (CPPM.read(CPPM_ELEV) - 1500*2) / 8 * 125 / 128; // elevator -100% .. +100%
//		int thro = (CPPM.read(CPPM_THRO) - 1500*2) / 8 * 125 / 128; // throttle -100% .. +100%
//		int rudd = (CPPM.read(CPPM_RUDD) - 1500*2) / 8 * 125 / 128; // rudder -100% .. +100%
//		int gear = (CPPM.read(CPPM_GEAR) - 1500*2) / 8 * 125 / 128; // gear -100% .. +100%
//		int aux1 = (CPPM.read(CPPM_AUX1) - 1500*2) / 8 * 125 / 128; // flap -100% .. +100%

		int aile = CPPM.read_us(CPPM_AILE) - 1500; // aile
		int elev = CPPM.read_us(CPPM_ELEV) - 1500; // elevator
		int thro = CPPM.read_us(CPPM_THRO) - 1500; // throttle
		int rudd = CPPM.read_us(CPPM_RUDD) - 1500; // rudder
		int gear = CPPM.read_us(CPPM_GEAR) - 1500; // gear
		int aux1 = CPPM.read_us(CPPM_AUX1) - 1500; // flap

#if 0
		Serial.print(aile); Serial.print(", ");
		Serial.print(elev); Serial.print(", ");
		Serial.print(thro); Serial.print(", ");
		Serial.print(rudd); Serial.print(", ");
		Serial.print(gear); Serial.print(", ");
		Serial.print(aux1); Serial.print("\n");
#else
		for (int i=0; i<(CPPM_MSERVO + 2); i++) {
			Serial.print('\t');
			Serial.print(CPPM._sync2[i]/2); // width of synch pulses
		}
		Serial.println();
		for (int i=0; i<(CPPM_MSERVO + 2); i++) {
			Serial.print('\t');
			Serial.print(CPPM._puls3[i]/2); // width of servo pulses
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

	CPPM.begin();
}

void loop()
{
	cppm_cycle();
}
