//
// This program polls for status data from the on-board Arduino.  The status is read, parsed,
// stored and virtual keyboard/mouse functions are triggered.
//
// Dependencies:
//   - WiringPi
//   - Loaded Linux kernel module "uinput".   Run 'sudo modprobe uinput' to load
//
// uinput code based from:
//   https://gist.github.com/toinsson/7e9fdd3c908b3c3d3cd635321d19d44d
// Key codes:
//   https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
//
// Thrown together by:
//   Chris Lathan
//   Cerulean Submergence, LLC
//

#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <string.h>

#define DEBUG 0
#define MOUSE_STEP_SIZE 2
#define PRESS_DOWN 1
#define PRESS_UP 0

// DEFAULT KEY MAPPINGS
// If the config file is not found, these will be used.

#define START_BUTTON		KEY_ESC
#define	SELECT_BUTTON		KEY_SPACE
#define A_BUTTON		KEY_1
#define B_BUTTON		KEY_2
#define X_BUTTON		KEY_3
#define Y_BUTTON		KEY_4
#define LNAV_CENTER		KEY_ENTER
#define LNAV_UP			KEY_UP
#define LNAV_DOWN		KEY_DOWN
#define LNAV_LEFT		KEY_LEFT
#define LNAV_RIGHT		KEY_RIGHT
#define RSHOULDER_CENTER	BTN_MIDDLE
#define RSHOULDER_CW		BTN_RIGHT
#define RSHOULDER_CCW		BTN_LEFT
#define LSHOULDER_CENTER	KEY_MUTE
#define LSHOULDER_CW		KEY_SCROLLUP
#define LSHOULDER_CCW		KEY_SCROLLDOWN
#define LJOYSTICK_LEFT		KEY_A
#define LJOYSTICK_RIGHT		KEY_D
#define LJOYSTICK_UP		KEY_W
#define LJOYSTICK_DOWN		KEY_S




// This is the status container strucktk definition
struct STATUS {
    unsigned char start;
    unsigned char select;
    unsigned char lsh_center;
    unsigned char lsh_cw;
    unsigned char lsh_ccw;
    unsigned char lnav_center;
    unsigned char lnav_right;
    unsigned char lnav_left;
    unsigned char lnav_down;
    unsigned char lnav_up;
    unsigned char rnav_center;
    unsigned char a;
    unsigned char y;
    unsigned char x;
    unsigned char b;
    unsigned char rsh_center;
    unsigned char rsh_cw;
    unsigned char rsh_ccw;

    unsigned char left_joy_x;
    unsigned char left_joy_y;
    unsigned char right_joy_x;
    unsigned char right_joy_y;

    unsigned int raw_battVolts;
    float battVolts;
};

// Make a new empty one
struct STATUS current_status = { 0 };


// Key register function to make the code below look nicer
void register_key(int fd, int code) {
    if(ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
        printf("Error registering key with code %d\n", code);
    if(ioctl(fd, UI_SET_KEYBIT, code) < 0)
        printf("Error registering key with code %d\n", code);
}

// Likewise.  Nicer looking code below thanks to this bad boy.
void emit(int fd, int type, int code, int val) {
    struct input_event ie;

    ie.type = type;
    ie.code = code;
    ie.value = val;
    /* timestamp values below are ignored */
    ie.time.tv_sec = 0;
    ie.time.tv_usec = 0;

    write(fd, &ie, sizeof(ie));
}

// See, isn't that nicer?
void keyPress(int fd, int key, int pressDir) {
    emit(fd, EV_KEY, key, pressDir);
    emit(fd, EV_SYN, SYN_REPORT, 0);
}

int main(void) {
    int                    keys_fd;
    struct uinput_user_dev uidev;
    struct input_event     ev;
    int                    dx, dy;
    int                    i;

    int log_time = time(NULL);
    int status_time = time(NULL);

    // Following directions.
    wiringPiSetupSys();

    int i2c_fd = wiringPiI2CSetup(0x19);

    if (!i2c_fd) {
        printf("ErroR opening i2c port!\n");
        return -1;
    }

    FILE *current_batt_file;
    FILE *current_joystick_file;


    // Try to open the uinput device.  Keep going if it doesn't
    // b/c we still do useful stuff without it.
    // BUT!  Will this code run now without it?  Will it crash?
    // Oh noes!  Find out!
    keys_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if(keys_fd < 0) printf("Can't open /dev/uinput.  Is the uinput module loaded?  sudo modprobe uinput");

    // Request? the use of certain keys for use with Omnijoy.
    // These keys are actually mapped to buttons later in the code

    // Left Nav
    register_key(keys_fd, LNAV_CENTER);
    register_key(keys_fd, LNAV_UP);
    register_key(keys_fd, LNAV_DOWN);
    register_key(keys_fd, LNAV_RIGHT);
    register_key(keys_fd, LNAV_LEFT);

    // Left Joystick
    register_key(keys_fd, LJOYSTICK_UP);
    register_key(keys_fd, LJOYSTICK_DOWN);
    register_key(keys_fd, LJOYSTICK_LEFT);
    register_key(keys_fd, LJOYSTICK_RIGHT);

    // A/B/X/Y/Start/Select
    register_key(keys_fd, A_BUTTON);
    register_key(keys_fd, B_BUTTON);
    register_key(keys_fd, X_BUTTON);
    register_key(keys_fd, Y_BUTTON);
    register_key(keys_fd, START_BUTTON);
    register_key(keys_fd, SELECT_BUTTON);

    // Left Shoulder
    register_key(keys_fd, LSHOULDER_CENTER);
    register_key(keys_fd, LSHOULDER_CW);
    register_key(keys_fd, LSHOULDER_CCW);

    // Right Shoulder/Mouse
    register_key(keys_fd, RSHOULDER_CENTER);
    register_key(keys_fd, RSHOULDER_CW);
    register_key(keys_fd, RSHOULDER_CCW);

    // Right Joystick/Mouse
    if(ioctl(keys_fd, UI_SET_EVBIT, EV_REL) < 0)
        printf("error: ioctl 5");
    if(ioctl(keys_fd, UI_SET_RELBIT, REL_X) < 0)
        printf("error: ioctl 6");
    if(ioctl(keys_fd, UI_SET_RELBIT, REL_Y) < 0)
        printf("error: ioctl 7");


    // Magic that I cut and pasted happens here
    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "OmniJoy Controls");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor  = 0x1;
    uidev.id.product = 0x1;
    uidev.id.version = 1;

    if(write(keys_fd, &uidev, sizeof(uidev)) < 0)
        printf("error: write");

    if(ioctl(keys_fd, UI_DEV_CREATE) < 0)
        printf("error: ioctl create");

    // This is tough work!  Stop and think about what
    // you've done.
    sleep(2);

    // Loop!
    while (1) {
        unsigned char buffer[20];
        int bytes_available;

        // Request Omnijoy data over the i2c port
        if (!read(i2c_fd, buffer, 9)) {
         printf("ErroR reading from i2c device!\n");
        }

        // And parse it into our struct container
        current_status.start = !(buffer[0] & 0x02);
        current_status.select = !(buffer[0] & 0x01);

        current_status.lsh_center = !(buffer[1] & 0x80);
        current_status.lsh_ccw = !(buffer[1] & 0x40);
        current_status.lsh_cw = !(buffer[1] & 0x20);
        current_status.lnav_center = !(buffer[1] & 0x10);
        current_status.lnav_right = !(buffer[1] & 0x08);
        current_status.lnav_left = !(buffer[1] & 0x04);
        current_status.lnav_down = !(buffer[1] & 0x02);
        current_status.lnav_up = !(buffer[1] & 0x01);

        current_status.rnav_center = !(buffer[2] & 0x80);
        current_status.b = !(buffer[2] & 0x40);
        current_status.x = !(buffer[2] & 0x20);
        current_status.y = !(buffer[2] & 0x10);
        current_status.a = !(buffer[2] & 0x08);
        current_status.rsh_center = !(buffer[2] & 0x04);
        current_status.rsh_ccw = !(buffer[2] & 0x02);
        current_status.rsh_cw = !(buffer[2] & 0x01);

        current_status.left_joy_x = 255 - buffer[3];   // Inverted
        current_status.left_joy_y = buffer[4];
        current_status.right_joy_x = buffer[5];
        current_status.right_joy_y = 255 - buffer[6];  // Inverted

        current_status.raw_battVolts = (buffer[8] << 8) + buffer[7];

        // Calculate actual voltage (5v/1024 = 0.00488)
        current_status.battVolts = 0.00488 * current_status.raw_battVolts;

        // Print out the results, if anyone's interested
        if (DEBUG) {
            printf("0x%02X 0x%02X 0x%02X X: %d Y: %d X: %d Y: %d Volts: %0.2fv ", \
                buffer[0], \
                buffer[1], \
                buffer[2], \
            current_status.left_joy_x, \
            current_status.left_joy_y, \
            current_status.right_joy_x, \
            current_status.right_joy_y, \
            current_status.battVolts);
            printf("\r");
        }
        current_batt_file = fopen ("/omnijoy/current_battery_voltage", "w+");
        current_joystick_file = fopen ("/omnijoy/current_joystick", "w+");

        // Update the status files on the (hopefully) ramdisk
        fprintf(current_batt_file, "%0.2f     ", current_status.battVolts);
        fprintf(current_joystick_file, "\r0x%02X 0x%02X 0x%02X X: %02d Y: %02d X: %d Y: %d      ", \
            buffer[0], \
            buffer[1], \
            buffer[2], \
            current_status.left_joy_x, \
            current_status.left_joy_y, \
            current_status.right_joy_x, \
            current_status.right_joy_y);

        fclose(current_batt_file);
        fclose(current_joystick_file);
        // Actual key mapping to buttons is here
        if (current_status.lsh_center) {
            if (DEBUG) printf("Left Shoulder Center");
            keyPress(keys_fd, LSHOULDER_CENTER, PRESS_DOWN);
        } else {
            keyPress(keys_fd, LSHOULDER_CENTER, PRESS_UP);
        }

        if (current_status.lsh_cw) {
            if (DEBUG) printf("Left Shoulder CW");
            keyPress(keys_fd, LSHOULDER_CW, PRESS_DOWN);
        } else {
            keyPress(keys_fd, LSHOULDER_CW, PRESS_UP);
        }

        if (current_status.lsh_ccw) {
            if (DEBUG) printf("Left Shoulder CCW");
            keyPress(keys_fd, LSHOULDER_CCW, PRESS_DOWN);
        } else {
            keyPress(keys_fd, LSHOULDER_CCW, PRESS_UP);
        }

        if (current_status.rsh_center) {
            if (DEBUG) printf("Right Shoulder Center");
            keyPress(keys_fd, RSHOULDER_CENTER, PRESS_DOWN);
        } else {
            keyPress(keys_fd, RSHOULDER_CENTER, PRESS_UP);
        }

        if (current_status.rsh_cw) {
            if (DEBUG) printf("Right Shoulder CW");
            keyPress(keys_fd, RSHOULDER_CW, PRESS_DOWN);
        } else {
            keyPress(keys_fd, RSHOULDER_CW, PRESS_UP);
        }

        if (current_status.rsh_ccw) {
            if (DEBUG) printf("Right Shoulder CCW");
            keyPress(keys_fd, RSHOULDER_CCW, PRESS_DOWN);
        } else {
            keyPress(keys_fd, RSHOULDER_CCW, PRESS_UP);
        }

        if (current_status.lnav_up) {
            if (DEBUG) printf("Nav Up");
            keyPress(keys_fd, LNAV_UP, PRESS_DOWN);
        } else {
            keyPress(keys_fd, LNAV_UP, PRESS_UP);
        }

        if (current_status.lnav_down) {
            if (DEBUG) printf("Nav Down");
            keyPress(keys_fd, LNAV_DOWN, PRESS_DOWN);
        } else {
            keyPress(keys_fd, LNAV_DOWN, PRESS_UP);
        }

        if (current_status.lnav_left) {
            if (DEBUG) printf("Nav Left");
            keyPress(keys_fd, LNAV_LEFT, PRESS_DOWN);
        } else {
            keyPress(keys_fd, LNAV_LEFT, PRESS_UP);
        }

        if (current_status.lnav_right) {
            if (DEBUG) printf("Nav Right");
            keyPress(keys_fd, LNAV_RIGHT, PRESS_DOWN);
        } else {
            keyPress(keys_fd, LNAV_RIGHT, PRESS_UP);
        }

        if (current_status.lnav_center) {
            if (DEBUG) printf("Nav Center");
            keyPress(keys_fd, LNAV_CENTER, PRESS_DOWN);
        } else {
            keyPress(keys_fd, LNAV_CENTER, PRESS_UP);
        }

        if (current_status.a) {
            if (DEBUG) printf("A Button");
            keyPress(keys_fd, A_BUTTON, PRESS_DOWN);
        } else {
            keyPress(keys_fd, A_BUTTON, PRESS_UP);
        }

        if (current_status.b) {
            if (DEBUG) printf("B Button");
            keyPress(keys_fd, B_BUTTON, PRESS_DOWN);
        } else {
            keyPress(keys_fd, B_BUTTON, PRESS_UP);
        }

        if (current_status.x) {
            if (DEBUG) printf("X Button");
            keyPress(keys_fd, X_BUTTON, PRESS_DOWN);
        } else {
            keyPress(keys_fd, X_BUTTON, PRESS_UP);
        }

        if (current_status.y) {
            if (DEBUG) printf("Y Button");
            keyPress(keys_fd, Y_BUTTON, PRESS_DOWN);
        } else {
            keyPress(keys_fd, Y_BUTTON, PRESS_UP);
        }

        if (current_status.start) {
            if (DEBUG) printf("Start Button");
            keyPress(keys_fd, START_BUTTON, PRESS_DOWN);
        } else {
            keyPress(keys_fd, START_BUTTON, PRESS_UP);
        }

        if (current_status.select) {
            if (DEBUG) printf("Select Button");
            keyPress(keys_fd, SELECT_BUTTON, PRESS_DOWN);
        } else {
            keyPress(keys_fd, SELECT_BUTTON, PRESS_UP);
        }

        if (current_status.left_joy_x < 90) {
            keyPress(keys_fd, LJOYSTICK_LEFT, PRESS_DOWN);
        } else {
            keyPress(keys_fd, LJOYSTICK_LEFT, PRESS_UP);
        }

        if (current_status.left_joy_x > 190) {
            keyPress(keys_fd, LJOYSTICK_RIGHT, PRESS_DOWN);
        } else {
            keyPress(keys_fd, LJOYSTICK_RIGHT, PRESS_UP);
        }

        if (current_status.left_joy_y < 90) {
            keyPress(keys_fd, LJOYSTICK_DOWN, PRESS_DOWN);
        } else {
            keyPress(keys_fd, LJOYSTICK_DOWN, PRESS_UP);
        }

        if (current_status.left_joy_y > 190) {
            keyPress(keys_fd, LJOYSTICK_UP, PRESS_DOWN);
        } else {
            keyPress(keys_fd, LJOYSTICK_UP, PRESS_UP);
        }

        // This is where we make the mouse move
        if (current_status.right_joy_x < 90) {
            emit(keys_fd, EV_REL, REL_X, -MOUSE_STEP_SIZE);
            emit(keys_fd, EV_SYN, SYN_REPORT, 0);
        }
        if (current_status.right_joy_x > 190) {
            emit(keys_fd, EV_REL, REL_X, MOUSE_STEP_SIZE);
            emit(keys_fd, EV_SYN, SYN_REPORT, 0);
        }
        // The value in the joystick struct is correct
        // but +y = down in X11
        if (current_status.right_joy_y < 90) {
            emit(keys_fd, EV_REL, REL_Y, MOUSE_STEP_SIZE);
            emit(keys_fd, EV_SYN, SYN_REPORT, 0);
        }
        if (current_status.right_joy_y > 190) {
            emit(keys_fd, EV_REL, REL_Y, -MOUSE_STEP_SIZE);
            emit(keys_fd, EV_SYN, SYN_REPORT, 0);
        }

        // Give some time to the processor
        delay(10);
    }

    if(ioctl(keys_fd, UI_DEV_DESTROY) < 0)
    printf("error: ioctl");

    close(keys_fd);
    close(i2c_fd);
    close(current_batt_file);
    close(current_joystick_file);

    return 1;
}

