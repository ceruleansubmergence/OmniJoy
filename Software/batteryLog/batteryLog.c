#include <stdio.h>
#include <time.h>

int main(void) {
    FILE* log_fd = NULL;
    FILE* battv_fd = NULL;

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    log_fd = fopen("battLog.csv", "a+");

    fprintf(log_fd, "Battery Log: %d-%d-%d %d:%d:%d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    while (1) {
        char buffer[5];

        // Open the battery voltage file for read only
        battv_fd = fopen("/omnijoy/current_battery_voltage", "r");

        // Read 4 bytes from the battery voltage file
        fread(buffer, 1, 4, battv_fd);

        // Close the file
        fclose(battv_fd);

        // Update the time
        tm = *localtime(&t);

        // Log the time and voltage
        fprintf(log_fd, "%d-%d-%d, %d:%d:%d, %s\n", tm.tm_year + 1900, tm.tm_mon + 1, \
                tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, buffer);

        // Sleep for 60sec
        usleep(60*1000*1000);
    }


}

