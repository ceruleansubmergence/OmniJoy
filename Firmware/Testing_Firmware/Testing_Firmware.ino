#include <SPI.h>
#include <Wire.h>

//
//  Sample program used to test functionality.  Is a basis for minimum needed code
//  to make Omnijoy operate.  Collects button, joystick and ADC (battery voltage)
//  status and waits to be polled by the pi over the i2c bus.
//  
//  Used Cort Buffington's MCP23S17 library for guidance
//  His library is awesome but overkill for use here so rewrote bare-bones code.
//
//  To Do:
//    - Make struct that holds data
//    - Make bit masks and macros to make getting at button values easier
//    - Make interrupt friendly so polling isn't required
//      - Setup an interrupt triggered by the MCP23S17 interrupt
//      - Check data for changes and use PB6 or PB7 as an interrupt to the pi
//        to signal new/updated data is ready to read.
//    - Write this into a library to make it seem simpler to use
//      - Omnijoy.begin();
//
//  Written By:
//  Chris Lathan
//  Cerulean Submergence, LLC
//  24OCT2017
//


// Joystick Connections
#define LEFTJOYSTICK_X    A0
#define LEFTJOYSTICK_Y    A1
#define RIGHTJOYSTICK_X   A2
#define RIGHTJOYSTICK_Y   A3

// Battery Voltage
#define BATTERYVOLTAGE A6
#define AUX_AD            A7

// Connections to MCP23S17 IO expander
#define INT_MCP23S17      9
#define CS_MCP23S17       10
#define MOSI              11
#define MISO              12
#define SCLK              13

#define WRITE_MCP23S17    0x40
#define READ_MCP23S17     0x41

// Register addresses (assumes IOCON BANK=0)
#define GPIOA_MCP23S17    0x12
#define GPIOB_MCP23S17    0x13
#define GPINTENA_MCP23S17 0x04
#define GPINTENB_MCP23S17 0x05
#define IOCON_MCP23S17    0x0A
#define GPPUA_MCP23S17    0x0C
#define GPPUB_MCP23S17    0x0D

// Start/Select Buttons
#define START_SW202       21
#define SELECT_SW201      20

#define I2C_ADDRESS       0x19
#define I2C_SDA_PIN       18
#define I2C_SCL_PIN       19
#define SERIAL_BAUDRATE   115200
#define SERIAL_DEBUG      0

struct PAYLOAD {
  unsigned char buttons;
  unsigned int MCP_Status;
  unsigned char leftJoystick_X;
  unsigned char leftJoystick_Y;
  unsigned char rightJoystick_X;
  unsigned char rightJoystick_Y;
  unsigned int battVoltage;
};

volatile struct PAYLOAD payload;

void setup() {
  // Setup comms
  Serial.begin(SERIAL_BAUDRATE);


  // Enable the pullup on the SDA/SCL pins
  pinMode(I2C_SDA_PIN, INPUT);
  digitalWrite(I2C_SDA_PIN, HIGH);
  pinMode(I2C_SCL_PIN, INPUT);
  digitalWrite(I2C_SCL_PIN, HIGH);
  
  // Start i2c using address
  Wire.begin(I2C_ADDRESS);
  Wire.onRequest(xferStats); // register event


  // Enable the start/select IO and pullups
  pinMode(START_SW202, INPUT);
  digitalWrite(START_SW202, HIGH);
  pinMode(SELECT_SW201, INPUT);
  digitalWrite(SELECT_SW201, HIGH);

  // Needed here?
  /*
  pinMode(LEFTJOYSTICK_X, INPUT);
  pinMode(LEFTJOYSTICK_Y, INPUT);
  pinMode(RIGHTJOYSTICK_X, INPUT);
  pinMode(RIGHTJOYSTICK_Y, INPUT);
  */
  
  // Setup the IO expander
  setup_MCP23S17();
}

void xferStats() {
  // Need to transfer 8 bytes:
  // 2 bits for start/stop (pack with batt voltage)
  // 10 bits for battery voltage
  // 16 bits for MCP23S17
  // 32 bits - 8 bits for each joystick axis (decimated from 10bit >>2)
  Wire.write((byte *)&payload, sizeof(payload));
}

void loop() {
  // Read the values of all the things and store them.
  payload.buttons = (payload.buttons & 0x01) + (digitalRead(START_SW202) << 1);
  payload.buttons = (payload.buttons & 0x02) + digitalRead(SELECT_SW201);
  payload.MCP_Status = read_MCP23S17();
  payload.leftJoystick_X = (analogRead(LEFTJOYSTICK_X) >> 2) & 0xFF;
  payload.leftJoystick_Y = (analogRead(LEFTJOYSTICK_Y) >> 2) & 0xFF;
  payload.rightJoystick_X = (analogRead(RIGHTJOYSTICK_X) >> 2) & 0xFF;
  payload.rightJoystick_Y = (analogRead(RIGHTJOYSTICK_Y) >> 2) & 0xFF;
  payload.battVoltage = analogRead(BATTERYVOLTAGE);

  // For now (and debug/testing), send them out the serial port
  if (SERIAL_DEBUG) {
    Serial.print(payload.buttons, BIN);
    Serial.print(" ");
    Serial.print(payload.MCP_Status, BIN);
    Serial.print(" ");
    Serial.print(payload.leftJoystick_X);
    Serial.print(" ");
    Serial.print(payload.leftJoystick_Y);
    Serial.print(" ");
    Serial.print(payload.rightJoystick_X);
    Serial.print(" ");
    Serial.print(payload.rightJoystick_Y);
    Serial.print(" ");  
    Serial.print(payload.battVoltage);
    Serial.write("\n");
    delay(10);
  } else {
    while(Serial.available() > 0) {
      if (Serial.read() == 'G') {
        Serial.write((byte *)&payload,sizeof(payload));
      }
    }
  }
  //delay(10);
}


void setup_MCP23S17(void) {
  // initalize the  interrupt and chip select pins:
  pinMode(INT_MCP23S17, INPUT);

  pinMode(CS_MCP23S17, OUTPUT);
  digitalWrite(CS_MCP23S17, HIGH);
  
  SPI.begin();

  // MCP23S17 starts up with all GPIO as inputs
  // No need to set address as there's only one MCP23S17 on OmniJoy Mini

  // Need to enable interupt mirroring (write 0x40 into IOCON)
  // Address pointer auto-increments
  writeByte_MCP23S17(IOCON_MCP23S17, 0x40);
  
  // Need to enable interrupt on change (write ones to GPINTEN registers)
  writeWord_MCP23S17(GPINTENA_MCP23S17, 0xFFFF);

  // Need to enable pullups (write ones into GPPU registers)
  writeWord_MCP23S17(GPPUA_MCP23S17, 0xFFFF);

}

unsigned int read_MCP23S17(void) {
  unsigned int value = 0;
  digitalWrite(CS_MCP23S17, LOW);
  SPI.transfer(READ_MCP23S17);
  SPI.transfer(GPIOA_MCP23S17);
  value = SPI.transfer(0x00);
  value |= (SPI.transfer(0x00) << 8);
  digitalWrite(CS_MCP23S17, HIGH);
  return value;
}

void writeByte_MCP23S17(uint8_t reg, uint8_t value) {
  digitalWrite(CS_MCP23S17, LOW);
  SPI.transfer(WRITE_MCP23S17);
  SPI.transfer(reg);
  SPI.transfer(value);
  digitalWrite(CS_MCP23S17, HIGH);
}

void writeWord_MCP23S17(uint8_t reg, unsigned int word) {
  digitalWrite(CS_MCP23S17, LOW);
  SPI.transfer(WRITE_MCP23S17); 
  SPI.transfer(reg);
  SPI.transfer((uint8_t) (word));
  SPI.transfer((uint8_t) (word >> 8));
  digitalWrite(CS_MCP23S17, HIGH);
}

uint8_t readByte_MCP23S17(uint8_t reg) {
  uint8_t value = 0;
  digitalWrite(CS_MCP23S17, LOW);
  SPI.transfer(READ_MCP23S17);
  SPI.transfer(reg);
  value = SPI.transfer(0x00);
  digitalWrite(CS_MCP23S17, HIGH);
  return value;
}
