#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdio.h>
#include <string.h>

int channel = 0;
int speed = 40000000;

#define RESET_PIN 3
#define DC_PIN 6
#define LED_PIN 1

unsigned char Gamma1[] = { 0xE0, 0x00, 0x03, 0x09, 0x08, 0x16, 0x0A, 0x3F, 0x78, 0x4C, 0x09, 0x0A, 0x08, 0x16, 0x1A, 0x0F };
unsigned char Gamma2[] = { 0xE1, 0x00, 0x16, 0x19, 0x03, 0x0F, 0x05, 0x32, 0x45, 0x46, 0x04, 0x0E, 0x0D, 0x35, 0x37, 0x0F };
unsigned char PwrCtl1[] = { 0xC0, 0x17, 0x15 };
unsigned char PwrCtl2[] = { 0xC1, 0x41 };
unsigned char PwrCtl3[] = { 0xC5, 0x00, 0x12, 0x80 };
unsigned char MemAcc[] = { 0x36, 0x48 };
unsigned char PixForm[] = { 0x3A, 0x66 };
unsigned char IntMod[] = { 0xB0, 0x00 };
unsigned char SleepOut[] = { 0x11 };
unsigned char DispOn[] = { 0x29 };
unsigned char ReadID[] = { 0x04, 0x00, 0x00, 0x00, 0x00 };
unsigned char IdleOff[] = { 0x38 };
unsigned char WriteRam[] = { 0x2C };
unsigned char NormMd[] = { 0x13 };
unsigned char TFTReset[] = { 0x01 };
unsigned char SetEPF[] = { 0xB7, 0x06 };


void send_command(unsigned char *cmd, int len);


void LCD_SetPos(unsigned int xs, unsigned int xe, unsigned int ys, unsigned int ye) {
  unsigned char buffer[5];

  buffer[0] = 0x2A;
  buffer[1] = xs >> 8;
  buffer[2] = xs & 0xFF;
  buffer[3] = xe >> 8;
  buffer[4] = xe & 0xFF;

  send_command(buffer, 5);

  buffer[0] = 0x2B;
  buffer[1] = ys >> 8;
  buffer[2] = ys & 0xFF;
  buffer[3] = ye >> 8;
  buffer[4] = ye & 0xFF;

  send_command(buffer, 5);

  buffer[0] = 0x2C;

  send_command(buffer, 1);

}

unsigned char reverse(unsigned char b) {
 b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
 b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
 b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
 return b;
}

void send_command(unsigned char *cmd, int len) {
 digitalWrite(DC_PIN, LOW);
 wiringPiSPIDataRW(channel,cmd,1);
 digitalWrite(DC_PIN, HIGH);
 cmd++;
 if (len > 1) wiringPiSPIDataRW(channel,cmd,len - 1);
}

int main(void) {
  unsigned char buffer[50];

  wiringPiSetup();

  pinMode(LED_PIN, OUTPUT);  // BACKLIGHT
  pinMode(RESET_PIN, OUTPUT);  // RESET
  pinMode(DC_PIN, OUTPUT);  // DC

  if (wiringPiSPISetup(channel, speed) == -1) {
	printf("Error!\n");
  }

  // Send Reset:
  digitalWrite(LED_PIN, HIGH);
  delay(1);

  digitalWrite(RESET_PIN, HIGH);
  delay(5);
  digitalWrite(RESET_PIN, LOW);
  delay(10);
  digitalWrite(RESET_PIN, HIGH);
  delay(120);

  send_command(TFTReset, 1);
  delay(10);

  send_command(Gamma1, 16);
  send_command(Gamma2, 16);
  send_command(PwrCtl1, 3);
  send_command(PwrCtl2, 2);
  send_command(PwrCtl3, 4);
  send_command(MemAcc, 2);
  send_command(PixForm, 2);
  send_command(IdleOff, 1);
  send_command(NormMd, 1);
  send_command(SleepOut, 1);
  delay(120);
  send_command(DispOn, 1);

  //send_command(SetEPF, 2);

  int i, j;
  j = 10;
  int intv=0;
  #define bpp 3
  while (1) {
    unsigned char rgb_color[480 * bpp];
    LCD_SetPos(0,319,0,479);
    for (i = 0; i < 480; i++) {
      j++;
      memset(rgb_color, 0, sizeof(rgb_color));
      for (intv = 2; intv < 320*bpp; intv+=bpp) {
      rgb_color[intv] = j;
      }
      wiringPiSPIDataRW(channel,rgb_color,320*bpp);
    }
//    delay(50);
  }

  return 0;
}
