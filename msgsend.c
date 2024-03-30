#include <stdio.h>
#include <malloc.h>
#include <sys/time.h>
#include <string.h>
#include <gpiod.h>

char result[3000] = {0};
int counter = 20;
int pos = 0;

void custom_delay(double milliseconds) {
    struct timeval tval_before, tval_after, tval_result;
    gettimeofday(&tval_before, NULL);
    double time_elapsed = 0;
    while (time_elapsed < milliseconds / 1000.0) {
        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);
        time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0);
    }
}

int main() {
    struct timeval tval_before, tval_after, tval_result;
    // GPIO Initialization
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    chip = gpiod_chip_open("/dev/gpiochip4");
    if (!chip) {
        perror("Open chip failed\n");
        return 1;
    }
    line = gpiod_chip_get_line(chip, 4);
    if (!line) {
        perror("Get line failed\n");
        gpiod_chip_close(chip);
        return 1;
    }
    int ret = gpiod_line_request_output(line, "example", 0);
    if (ret < 0) {
        perror("Request line as output failed\n");
        gpiod_chip_close(chip);
        return 1;
    }

    // Read message
    char msg[3000];
    int len, k, length;
    gpiod_line_set_value(line, 0);
    printf("\n Enter the Message: ");
    scanf("%[^\n]", msg);
    // Append preamble
    strcpy(result, "10101");

    // Append user's input
    length = strlen(msg);
    strncat(result, msg, length);

    printf("Frame Header (Synchro and Text) = %s\n", result);

    length = strlen(result);
    gettimeofday(&tval_before, NULL);
    while(pos != length) {
        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);
        double time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0f);
        while(time_elapsed < 0.001) {
            gettimeofday(&tval_after, NULL);
            timersub(&tval_after, &tval_before, &tval_result);
            time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0f);
        }
        gettimeofday(&tval_before, NULL);
        if (result[pos] == '1') {
            gpiod_line_set_value(line, 1);
            pos++;
        } else if(result[pos] == '0') {
            gpiod_line_set_value(line, 0);
            pos++;
        }
    }

    // Cleanup
    custom_delay(1);
    gpiod_line_set_value(line, 0);
    gpiod_line_release(line);
    gpiod_chip_close(chip);
    return 0;
}