#include <stdio.h>
#include <malloc.h>
#include <sys/time.h>
#include <string.h>
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
        result[36] = '\0';
        counter++;
    }
}

int pos = 0;

// Calculate CRC for the given data frame
void calculateCRC(char *dataFrame) {
    int polynom[9] = {1, 0, 0, 1, 0, 1, 1, 1, 1};
    int k = strlen(dataFrame);
    int p = 9;
    int n = k + p - 1;
    int frame[n];

    // Convert data frame to integer array
    for (int i = 0; i < n; i++) {
        if (i < k) {
            frame[i] = dataFrame[i] - '0';
        } else {
            frame[i] = 0;
        }
    }

    // Perform CRC calculation
    int i = 0;
    while (i < k) {
        for (int j = 0; j < p; j++) {
            if (frame[i + j] == polynom[j]) {
                frame[i + j] = 0;
            } else {
                frame[i + j] = 1;
            }
        }
        while (i < n && frame[i] != 1) {
            i++;
        }
    }

    // Append CRC bits to the data frame
    for (int j = k; j - k < p - 1; j++) {
        dataFrame[j] = frame[j];
    }
}

int main() {
    struct timeval tval_before, tval_after, tval_result;
    char msg[3000];
    int len, k, length;

    // Initialize libgpiod
    struct gpiod_chip *chip = gpiod_chip_open_by_name("gpiochip4");
    if (!chip) {
        perror("Failed to open GPIO chip");
        return 0;
    }

    struct gpiod_line *line = gpiod_chip_get_line(chip, 4);
    if (!line) {
        perror("Failed to get GPIO line");
        gpiod_chip_close(chip);
        return 0;
    }

    // Configure the GPIO line as output
    int ret = gpiod_line_request_output(line, "led", GPIOD_LINE_ACTIVE_STATE_LOW);
    if (ret < 0) {
        perror("Failed to request GPIO line");
        gpiod_line_release(line);
        gpiod_chip_close(chip);
        return 0;
    }
    gpiod_line_set_value(line, 0);
    printf("\nEnter the Message: ");
    scanf("%[^\n]", msg);
    len = strlen(msg);

    // Convert message length to binary and append to result
    int2bin(len * 8, 16);
    printf("Frame Header (Synchro and Textlength): %s\n", result);

    // Append message bits to result
    for (k = 0; k < len; k++) {
        chartobin(msg[k]);
    }

    // Calculate CRC and append to the message
    calculateCRC(result + 20); // Start from position after header
    printf("Frame with CRC: %s\n", result);

    length = strlen(result);
    gettimeofday(&tval_before, NULL);

    // Transmit the message with CRC
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

    // Clean up
    gpiod_line_set_value(line, 0);
    gpiod_line_release(line);
    gpiod_chip_close(chip);

    return 0;
}
