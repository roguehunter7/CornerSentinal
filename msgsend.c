#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <gpiod.h>

#define GPIO_CHIP_NAME "gpiochip4"  // Change this if your GPIO chip name is different
#define GPIO_PIN_NUMBER 4           // Change this to your desired GPIO pin number
#define BUFFER_SIZE 3000

char result[BUFFER_SIZE] = {'1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1'};
int counter = 20;

void chartobin(char c) {
    int i;
    for (i = 7; i >= 0; i--) {
        result[counter] = (c & (1 << i)) ? '1' : '0';
        counter++;
    }
}

void int2bin(unsigned integer, int n) {
    for (int i = 0; i < n; i++) {
        result[counter] = (integer & (int)1 << (n - i - 1)) ? '1' : '0';
        result[36] = '\0';
        counter++;
    }
}

int pos = 0;

int main() {
    struct timeval tval_before, tval_after, tval_result;

    // Open GPIO chip
    struct gpiod_chip *chip = gpiod_chip_open(GPIO_CHIP_NAME);
    if (!chip) {
        perror("Error opening GPIO chip");
        return 1;
    }

    // Get GPIO line
    struct gpiod_line *line = gpiod_chip_get_line(chip, GPIO_PIN_NUMBER);
    if (!line) {
        perror("Error getting GPIO line");
        gpiod_chip_close(chip);
        return 1;
    }

    // Request output direction for GPIO pin
    if (gpiod_line_request_output(line, "output", 0) < 0) {
        perror("Error setting GPIO pin as output");
        gpiod_line_release(line);
        gpiod_chip_close(chip);
        return 1;
    }

    // Read message
    char msg[BUFFER_SIZE];
    int len, k, length;

    printf("\nEnter the Message: ");
    scanf("%[^\n]", msg);

    len = strlen(msg);

    int2bin(len * 8, 16); // Multiply by 8 because one byte is 8 bits
    printf("Frame Header (Synchro and Textlength): %s\n", result);

    for (k = 0; k < len; k++) {
        chartobin(msg[k]);
    }

    length = strlen(result);
    gettimeofday(&tval_before, NULL);
    while (pos != length) {
        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);
        double time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0f);

        while (time_elapsed < 0.001) {
            gettimeofday(&tval_after, NULL);
            timersub(&tval_after, &tval_before, &tval_result);
            time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0f);
        }
        gettimeofday(&tval_before, NULL);

        if (result[pos] == '1') {
            gpiod_line_set_value(line, 1);
            pos++;
        } else if (result[pos] == '0') {
            gpiod_line_set_value(line, 0);
            pos++;
        }
    }

    // Release resources
    gpiod_line_release(line);
    gpiod_chip_close(chip);

    return 0;
}
