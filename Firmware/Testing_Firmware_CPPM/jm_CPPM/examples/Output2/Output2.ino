
#include <CPPM.h>

void setup()
{
	Serial.begin(9600);

	CPPM.begin();
}

int output2 = 1000 + 500;

void loop()
{
	CPPM.cycle();

	CPPM.write_us(CPPM_AILE, CPPM_PULSE_CENTER + (abs(output2 - 1000) - 500));
	CPPM.write2_us(CPPM_AILE, CPPM_PULSE_CENTER - (abs(output2 - 1000) - 500));

	output2++;
	if (output2 == 2000) output2 = 0;

	delay(1);
}