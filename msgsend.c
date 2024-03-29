#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <gpiod.h>

char result[16]; // Preamble (5 bits) + Message (8 bits) + CRC (3 bits)
int counter = 0;

int pos = 0;

// CRC calculation function (assuming 3-bit CRC)
void calculateCRC(char *data, int length, char *crc) {
    int polynom[4] = {1, 0, 1, 1}; // 3-bit CRC polynomial
    int k = length; // Length of the data frame (data)
    int p = 4;      // Length of the CRC polynomial
    int n = k + p - 1;   // Total length including padding for division
    int frame[n];        // Buffer frame with perfect size for CRC

    // Convert data frame to integer array
    for (int i = 0; i < k; i++) {
        frame[i] = data[i] - '0';
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

    // Read 8-bit binary message
    char msg[9];
    printf("\nEnter the 8-bit Binary Message: ");
    scanf("%8s", msg);

    // Add preamble (5 bits)
    strcpy(result, "10101");
    counter += 5;

    // Add message data (8 bits)
    strcat(result, msg);
    counter += 8;

    // Calculate and add CRC (3 bits)
    char crc[4]; // 3 bits CRC
    calculateCRC(result + 5, 8, crc);
    strcat(result, crc);
    counter += 3;

    gettimeofday(&tval_before, NULL);

    int length = strlen(result);

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

    // Release GPIO line and chip
    gpiod_line_set_value(line, 0);
    gpiod_line_release(line);
    gpiod_chip_close(chip);

    return 0;
}
