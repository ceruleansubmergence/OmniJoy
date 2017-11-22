
#include <stdio.h>
#include <syslog.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/types.h>
#include <sys/mman.h>

#include <bcm_host.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>


int channel = 0;
int speed = 32000000;

#define RESET_PIN 3
#define DC_PIN 6
#define LED_PIN 1

unsigned char Gamma1[] = { 0xE0, 0x00, 0x03, 0x09, 0x08, 0x16, 0x0A, 0x3F, 0x78, 0x4C, 0x09, 0x0A, 0x08, 0x16, 0x1A, 0x0F };
unsigned char Gamma2[] = { 0xE1, 0x00, 0x16, 0x19, 0x03, 0x0F, 0x05, 0x32, 0x45, 0x46, 0x04, 0x0E, 0x0D, 0x35, 0x37, 0x0F };
unsigned char PwrCtl1[] = { 0xC0, 0x17, 0x15 };
unsigned char PwrCtl2[] = { 0xC1, 0x41 };
unsigned char PwrCtl3[] = { 0xC5, 0x00, 0x12, 0x80 };
unsigned char MemAcc[] = { 0x36, 0x28 };
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
void write_Display(unsigned char *data, unsigned long int length);
void convert_RGB666(__u16 *rgb565, int width, int height, unsigned char *rgb666);


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

void write_Display(unsigned char *data, unsigned long int length) {
  syslog(LOG_INFO,"Updating Display with %d bytes 0x%02X", length, data[10]);
  int i;

  LCD_SetPos(0,479,0,319);

  // WiringPi SPI can't handle the full buffer so break it up into managable chunks
  for (i = 0; i < 320; i++) {
    wiringPiSPIDataRW(channel,data,480*3);
    data+=480*3;
  }

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


int process() {
    DISPMANX_DISPLAY_HANDLE_T display;
    DISPMANX_MODEINFO_T display_info;
    DISPMANX_RESOURCE_HANDLE_T screen_resource;
    VC_IMAGE_TRANSFORM_T transform;
    uint32_t image_prt;
    VC_RECT_T rect1;
    int ret;

    unsigned char dispBuffer565[480*320*2];
    unsigned char dispBuffer666[480*320*3];

    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;


    bcm_host_init();

    display = vc_dispmanx_display_open(0);
    if (!display) {
        syslog(LOG_ERR, "Unable to open primary display");
        return -1;
    }
    ret = vc_dispmanx_display_get_info(display, &display_info);
    if (ret) {
        syslog(LOG_ERR, "Unable to get primary display information");
        return -1;
    }
    syslog(LOG_INFO, "Primary display is %d x %d", display_info.width, display_info.height);

    screen_resource = vc_dispmanx_resource_create(VC_IMAGE_RGB565, 480, 320, &image_prt);
    if (!screen_resource) {
        syslog(LOG_ERR, "Unable to create screen buffer");
        vc_dispmanx_display_close(display);
        return -1;
    }

    vc_dispmanx_rect_set(&rect1, 0, 0, 480, 320);
    syslog(LOG_INFO, "Created memory for display %d x %d %dbpp\n", 480, 320, 6);


    while (1) {
        ret = vc_dispmanx_snapshot(display, screen_resource, 0);
        vc_dispmanx_resource_read_data(screen_resource, &rect1, dispBuffer565, 480 * 16 / 8);

	convert_RGB666((__u16 *)dispBuffer565, 480, 320, dispBuffer666);
        write_Display(dispBuffer666,sizeof(dispBuffer666));

	usleep(5 * 1000);
   }



    ret = vc_dispmanx_resource_delete(screen_resource);
    vc_dispmanx_display_close(display);
}

void convert_RGB666(__u16 *rgb565, int width, int height, unsigned char *rgb666) {
  int buffer_size = width*height;

  int i = 0;
  int count666 = 0;
  for (i = 0; i < buffer_size; i++) {
    rgb666[count666]   = (rgb565[i] & 0b1111100000000000) >> 9;
    rgb666[count666+1] = (rgb565[i] & 0b0000011111100000) >> 3;
    rgb666[count666+2] = (rgb565[i] & 0b0000000000011111) << 3;
    count666+=3;
  }

}

int init_Display(void) {

  pinMode(LED_PIN, OUTPUT);  // BACKLIGHT
  pinMode(RESET_PIN, OUTPUT);  // RESET
  pinMode(DC_PIN, OUTPUT);  // DC

  if (wiringPiSPISetup(channel, speed) == -1) {
        syslog(LOG_ERR, "Unable to connect to wiringPI SPI\n");
        return -1;
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
  send_command(SleepOut, 1);
  delay(120);
  send_command(DispOn, 1);
  send_command(IdleOff, 1);
  send_command(NormMd, 1);

//  send_command(SetEPF, 2);

  return 1;
}

int main(int argc, char **argv) {
    setlogmask(LOG_UPTO(LOG_DEBUG));
    openlog("fb_RGB666", LOG_NDELAY | LOG_PID, LOG_USER);

  unsigned char buffer[50];

  wiringPiSetup();
  if (init_Display()) {
    syslog(LOG_INFO, "Successfully initiated the display");
  } else {
    syslog(LOG_ERR, "Unable to initiate the display");
  }

    return process();
}



