#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include <jm_CPPM.h>

//
//  Sample program used to test functionality.  Is a basis for minimum needed code
//  to make Omnijoy operate.  Collects button, joystick and ADC (battery voltage)
//  status and waits to be polled by the pi over the i2c bus.
//  
//  Used Cort Buffington's MCP23S17 library for guidance
//  His library is awesome but overkill for use here so rewrote bare-bones version.
//
//  To Do:
//    - Make interrupt friendly so polling isn't required
//      - Setup an interrupt triggered by the MCP23S17 interrupt
//      - Check data for changes and use PB6 or PB7 as an interrupt to the pi
//        to signal new/updated data is ready to read.
//    - Write this into a library to make it seem simpler to use
//      - Omnijoy.begin();
//    - Add error checking (checksum or CRC) to EEPROM reading/writing
//
//  6NOV17:  Added CPPM output
//  8NOV17:  Added i2c registers, CPPM configuration, eeprom config load/save
//
//  Written By:
//  Chris Lathan
//  Cerulean Submergence, LLC
//  24OCT2017
//


// Joystick Connections
#define LEFTJOYSTICK_X    A1
#define LEFTJOYSTICK_Y    A0
#define RIGHTJOYSTICK_X   A3
#define RIGHTJOYSTICK_Y   A2

// Battery Voltage
#define BATTERYVOLTAGE    A6
#define AUX_AD            A7

// Connections to MCP23S17 IO expander
#define INT_MCP23S17      8
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

#define PPM_MAX           990
#define PPM_MIN           -PPM_MAX

#define isLNavUp                !(payload.MCP_Status & 0b0000000000000001)
#define isLNavDown              !(payload.MCP_Status & 0b0000000000000010)
#define isLNavLeft              !(payload.MCP_Status & 0b0000000000000100)
#define isLNavRight             !(payload.MCP_Status & 0b0000000000001000)
#define isLNavSelected          !(payload.MCP_Status & 0b0000000000010000)
#define isLShoulderCCW          !(payload.MCP_Status & 0b0000000000100000)
#define isLShoulderCW           !(payload.MCP_Status & 0b0000000001000000)
#define isLShoulderSelected     !(payload.MCP_Status & 0b0000000010000000)
#define isRShoulderCCW          !(payload.MCP_Status & 0b0000000100000000)
#define isRShoulderCW           !(payload.MCP_Status & 0b0000001000000000)
#define isRShoulderSelected     !(payload.MCP_Status & 0b0000010000000000)
#define isA                     !(payload.MCP_Status & 0b0000100000000000)
#define isY                     !(payload.MCP_Status & 0b0001000000000000)
#define isX                     !(payload.MCP_Status & 0b0010000000000000)
#define isB                     !(payload.MCP_Status & 0b0100000000000000)
#define isStart                 !(payload.buttons & 0b00000001)
#define isSelect                !(payload.buttons & 0b00000010)

// Store config settings at the end of the EEPROM to make it
// easier for users to used the beginning section
#define CONFIG_START      (1024-50)
#define CONFIG_VERSION    "OMNIJOY_R0"

// Define register addresses, starting at an offset as defined here
enum REGISTERS {OFFSET = 0x10, I2C_ADDRESS_REGISTER, SERIAL_DEBUG_EN, UINPUT_EN, PPM_EN, LX_TRIM, LY_TRIM, RX_TRIM, RY_TRIM, LX_SCALE, \
                LY_SCALE, RX_SCALE, RY_SCALE, PPM_MAP1, PPM_MAP2, PPM_MAP3, PPM_MAP4, PPM_MAP5, PPM_MAP6, \
                PPM_MAP7, PPM_MAP8, PPM_MAP9, PPM_INVERT, LOAD_FROM_EEPROM, SAVE_TO_EEPROM, LOAD_DEFAULTS};

enum CHANNELS {LJOY_X, LJOY_Y, RJOY_X, RJOY_Y, RSHOULDER, RSHOULDER_SELECT, LSHOULDER, LSHOULDER_SELECT, \
                START_BTN, SELECT_BTN, A_BTN, B_BTN, X_BTN, Y_BTN, LNAV_X, LNAV_Y, LNAV_SELECT};

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

struct CONFIG {
  char config_version[11];
  unsigned char i2c_Address;
  unsigned char uinput_enabled;
  unsigned char ppm_enabled;
  unsigned char serial_debug_enabled;
  unsigned char ljoy_x_trim;
  unsigned char ljoy_y_trim;
  unsigned char rjoy_x_trim;
  unsigned char rjoy_y_trim;
  unsigned char ljoy_x_scale;
  unsigned char ljoy_y_scale;
  unsigned char rjoy_x_scale;
  unsigned char rjoy_y_scale;
  unsigned char ppm_channel[9];
  char ppm_chn_invert[9];
} cnfig = { CONFIG_VERSION, \
            0x19, \
            true, \
            true, \
            false, \
            127, \
            127, \
            127, \
            127, \
            10, \
            10, \
            10, \
            10, \
            {RJOY_X, RJOY_Y, LJOY_X, LJOY_Y, LNAV_X, LNAV_Y, LNAV_SELECT, RSHOULDER, LSHOULDER}, \
            {1, 1, 1, 1, 1, 1, 1, 1, 1}};


void read_config_from_EEPROM(void) {
  // Check CONFIG_VERSION to make sure the settings are ours.
  // If nothing is found it will use the default settings.
  //   https://playground.arduino.cc/Code/EEPROMLoadAndSaveSettings
  if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] && EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] && EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2]) {
    for (unsigned int t=0; t<sizeof(cnfig); t++) {
      *((char*)&cnfig + t) = EEPROM.read(CONFIG_START + t);
    }
  }
}

void save_config_to_EEPROM(void) {
  for (unsigned int t=0; t<sizeof(cnfig); t++)
    EEPROM.write(CONFIG_START + t, *((char*)&cnfig + t));
}

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
  Wire.onRequest(xferStats); // Register write event
  Wire.onReceive(i2cReceive); // Register read event

  // Enable the start/select IO and pullups
  pinMode(START_SW202, INPUT);
  digitalWrite(START_SW202, HIGH);
  pinMode(SELECT_SW201, INPUT);
  digitalWrite(SELECT_SW201, HIGH);

  // Load defaults
  read_config_from_EEPROM();
  
  CPPM.begin();
  
  // Setup the IO expander
  setup_MCP23S17();
  
}

// This function runs when the pi wants to tell us something
void i2cReceive(int howMany) {
  char buffer[4];
  unsigned char count = 0;
  
  while (Wire.available()) { // loop through all but the last
    buffer[count++] = Wire.read(); // receive byte as a character
  }

  if (count >= 2) {
    switch (buffer[0]) {
      case I2C_ADDRESS_REGISTER:                  // Set the i2c address
        cnfig.i2c_Address = buffer[1];
      break;
      case SERIAL_DEBUG_EN:                       // Enable debugging output over the serial port
        cnfig.serial_debug_enabled = buffer[1];
      break;
      case UINPUT_EN:                             // Allow the pi to read the joystick/button status
        cnfig.uinput_enabled = buffer[1];
      break;
      case PPM_EN:                                // Enable the PPM Output
        cnfig.ppm_enabled = buffer[1];
      break;
      case LX_TRIM:                               // Set the joystick trims
        cnfig.ljoy_x_trim = buffer[1];
      break;
      case LY_TRIM:
        cnfig.ljoy_y_trim = buffer[1];
      break;
      case RX_TRIM:
        cnfig.rjoy_x_trim = buffer[1];
      break;
      case RY_TRIM:
        cnfig.rjoy_y_trim = buffer[1];
      break;
      case LX_SCALE:                              // Set the joystick scaling values
        cnfig.ljoy_x_scale = buffer[1];
      break;
      case LY_SCALE:
        cnfig.ljoy_y_scale = buffer[1];
      break;
      case RX_SCALE:
        cnfig.rjoy_x_scale = buffer[1];
      break;
      case RY_SCALE:
        cnfig.rjoy_y_scale = buffer[1];
      break;
      case PPM_MAP1:                              // Set button/ppm channel mapping
        cnfig.ppm_channel[0] = buffer[1];
      break;
      case PPM_MAP2:
        cnfig.ppm_channel[1] = buffer[1];
      break;
      case PPM_MAP3:
        cnfig.ppm_channel[2] = buffer[1];
      break;
      case PPM_MAP4:
        cnfig.ppm_channel[3] = buffer[1];
      break;
      case PPM_MAP5:
        cnfig.ppm_channel[4] = buffer[1];
      break;
      case PPM_MAP6:
        cnfig.ppm_channel[5] = buffer[1];
      break;
      case PPM_MAP7:
        cnfig.ppm_channel[6] = buffer[1];
      break;
      case PPM_MAP8:
        cnfig.ppm_channel[7] = buffer[1];
      break;
      case PPM_MAP9:
        cnfig.ppm_channel[8] = buffer[1];
      break;
      case PPM_INVERT:                            // Define which channels should be inverted
        for (unsigned char i = 0; i < 7; i++) {
          cnfig.ppm_chn_invert[i] = (buffer[1]>>i) & 0x01;
        }
        cnfig.ppm_chn_invert[8] = buffer[2] & 0x01;
      break;
      case LOAD_FROM_EEPROM:                      // Load values from EEPROM
        read_config_from_EEPROM();
      break;
      case SAVE_TO_EEPROM:                        // Save new defaults to EEPROM
        save_config_to_EEPROM();
      break;
      case LOAD_DEFAULTS:
        //cnfig.config_version = CONFIG_VERSION;
        cnfig.i2c_Address = 0x19;
        cnfig.uinput_enabled = true;
        cnfig.ppm_enabled = true;
        cnfig.serial_debug_enabled = false;
        cnfig.ljoy_x_trim = 127;
        cnfig.ljoy_y_trim = 127;
        cnfig.rjoy_x_trim = 127;
        cnfig.rjoy_y_trim = 127;
        cnfig.ljoy_x_scale = 10;
        cnfig.ljoy_y_scale = 10;
        cnfig.rjoy_x_scale = 10;
        cnfig.rjoy_y_scale = 10;
        cnfig.ppm_channel[0] = RJOY_X;
        cnfig.ppm_channel[1] = RJOY_Y;
        cnfig.ppm_channel[2] = LJOY_X;
        cnfig.ppm_channel[3] = LJOY_Y;
        cnfig.ppm_channel[4] = LNAV_X;
        cnfig.ppm_channel[5] = LNAV_Y;
        cnfig.ppm_channel[6] = LNAV_SELECT;
        cnfig.ppm_channel[7] = RSHOULDER;
        cnfig.ppm_channel[8] = LSHOULDER;
        for (int i = 0; i < 9; i++ ) cnfig.ppm_chn_invert[i] = 1;
      break;
      default:
      break;
    }
  }
}

// This runs when the pi wants our data!
void xferStats() {
  // Need to transfer 9 bytes:
  // 2 bits (1byte) for start/stop (pack with batt voltage)
  // 16 bits (2bytes) for MCP23S17
  // 32 bits (4 bytes) - 8 bits for each joystick axis (decimated from 10bit >>2)
  // 10 bits (2 bytes) for battery voltage
  
  if (cnfig.uinput_enabled) {
    Wire.write((byte *)&payload, sizeof(payload));
  } else {
    char buffer[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    buffer[7] = (payload.battVoltage >> 8) & 0xFF;
    buffer[8] = payload.battVoltage & 0xFF;
    Wire.write((byte *)&buffer, sizeof(buffer));
  }
}

// Checks to make sure our ppm value is in the proper range
int ppm_bounds_check(int value) {
    if (value > PPM_MAX) return PPM_MAX; else if (value < PPM_MIN) return PPM_MIN; else return value;
}

// This runs all the time!
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

  // Do ppm output if it's enabled
  if (cnfig.ppm_enabled) {
    for (unsigned char i = 0; i < 9; i++) {
      int temp = 0;
      switch (cnfig.ppm_channel[i]) {
        case LJOY_X:
          temp = ppm_bounds_check((payload.leftJoystick_Y - cnfig.ljoy_y_trim) * cnfig.ljoy_y_scale);
        break;
        case LJOY_Y:
          temp = ppm_bounds_check((payload.leftJoystick_X - cnfig.ljoy_x_trim) * cnfig.ljoy_x_scale);
        break;
        case RJOY_X:
          temp = ppm_bounds_check((payload.leftJoystick_X - cnfig.rjoy_x_trim) * cnfig.rjoy_x_scale);
        break;
        case RJOY_Y:
          temp = ppm_bounds_check((payload.leftJoystick_Y - cnfig.rjoy_y_trim) * cnfig.rjoy_y_scale);
        break;
        case RSHOULDER:
          if (isRShoulderCW) {
            temp = PPM_MAX;
          } else if (isRShoulderCCW) {
            temp = PPM_MIN;
          }
        break;
        case RSHOULDER_SELECT:
          if (isRShoulderSelected) {
            temp = PPM_MAX;
          }
        break;
        case LSHOULDER:
          if (isLShoulderCW) {
            temp = PPM_MAX;
          } else if (isLShoulderCCW) {
            temp = PPM_MIN;
          }
        break;
        case LSHOULDER_SELECT:
          if (isLShoulderSelected) {
            temp = PPM_MAX;
          }
        break;
        case START_BTN:
          if (isStart) {
            temp = PPM_MAX;
          }
        break;
        case SELECT_BTN:
          if (isSelect) {
            temp = PPM_MAX;
          }
        break;
        case A_BTN:
          if (isA) {
            temp = PPM_MAX;
          }
        break;
        case B_BTN:
          if (isB) {
            temp = PPM_MAX;
          }
        break;
        case X_BTN:
          if (isX) {
            temp = PPM_MAX;
          }
        break;
        case Y_BTN:
          if (isY) {
            temp = PPM_MAX;
          }
        break;
        case LNAV_X:
          if (isLNavRight) { 
            temp = PPM_MAX;
          } else if (isLNavLeft) {
            temp = PPM_MIN;
          }
        break;
        case LNAV_Y:
          if (isLNavUp) { 
            temp = PPM_MAX;
          } else if (isLNavDown) {
            temp = PPM_MIN;
          }
        break;
        case LNAV_SELECT:
          if (isLNavSelected) {
            temp = PPM_MAX;
          }
        break;
        default:
          temp = 0;
          break;
      }
      
      CPPM.write1_us(i, CPPM_PULSE_CENTER + (temp * cnfig.ppm_chn_invert[i]));    
    }
  }
    
  // For now (and debug/testing), send them out the serial port
  if (cnfig.serial_debug_enabled) {
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
  delay(10);
}


void setup_MCP23S17(void) {
  // initalize the  interrupt and chip select pins:
  pinMode(INT_MCP23S17, INPUT);
  pinMode(MISO, INPUT);
  digitalWrite(MISO, LOW);
  pinMode(MOSI, OUTPUT);
  pinMode(SCLK, OUTPUT);  
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
