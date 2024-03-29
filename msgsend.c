#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <gpiod.h>

char result[3000] = {'1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1'};
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
        counter++;
    }
}

int pos = 0;

// CRC calculation function
void calculateCRC(char *data, int length, char *crc) {
    int polynom[9] = {1, 0, 0, 1, 0, 1, 1, 1, 1};
    int k = length + 18; // Length of the data frame (data + sync sequence + length)
    int p = 9;           // Length of the CRC polynomial
    int n = k + p - 1;   // Total length including padding for division
    int frame[n];        // Buffer frame with perfect size for CRC

    // Convert data frame to integer array
    for (int i = 0; i < k; i++) {
        if (i < 18) {
            frame[i] = result[i] - '0';
        } else {
            frame[i] = data[i - 18] - '0';
        }
    }
    for (int i = k; i < n; i++) {
        frame[i] = 0;
    }

    // Perform polynomial division
    int i = 0;
    while (i < k) {
        for (int j = 0; j < p; j++) {
            if (frame[i + j] == polynom[j]) {
                frame[i + j] = 0;
            } else {
                frame[i + j] = 1;
            }
        }
        while (i < n && frame[i] != 1)
            i++;
    }

    // Copy CRC to the output array
    for (int j = k; j - k < p - 1; j++) {
        crc[j - k] = frame[j] + '0';
    }
    crc[p - 1] = '\0';
}

int main() {
    struct timeval tval_before, tval_after, tval_result;

    // Open GPIO chip
    struct gpiod_chip *chip;
    chip = gpiod_chip_open("/dev/gpiochip4");
    if (!chip) {
        perror("Error opening GPIO chip");
        return 1;
    }

    // Request and configure GPIO line
    struct gpiod_line *line;
    line = gpiod_chip_get_line(chip, 4); // Assuming GPIO line 0
    if (!line) {
        perror("Error getting GPIO line");
        gpiod_chip_close(chip);
        return 1;
    }

    if (gpiod_line_request_output(line, "gpio-program", 0) < 0) {
        perror("Error requesting GPIO output");
        gpiod_chip_close(chip);
        return 1;
    }

    // Read message
    char msg[3000];
    int len, k, length;
    printf("\n Enter the Message: ");
    scanf("%[^\n]", msg);
    len = strlen(msg);

    // Add frame header (sync sequence and length)
    strcpy(result + counter, "1010101111111111");
    counter += 16;
    int2bin(len * 8, 16);

    printf("Frame Header (Synchro and Textlength) = %s\n", result);

    // Add message data
    for (k = 0; k < len; k++) {
        chartobin(msg[k]);
    }

    // Calculate and add CRC
    char crc[9];
    calculateCRC(result + 20, counter - 20, crc);
    strcat(result, crc);
    length = strlen(result);

    gettimeofday(&tval_before, NULL);

    while (pos != length) {
        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);
        double time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0f);
        while (time_elapsed < 0.002) {
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

    // Release GPIO line and chip
    gpiod_line_set_value(line, 0);
    gpiod_line_release(line);
    gpiod_chip_close(chip);

    return 0;
}
