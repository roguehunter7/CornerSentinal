#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <gpiod.h>

#define preambleSize 5
#define binaryCodeSize 8
#define crcCodeSize 3

char result[16] = {'1', '0', '1', '0', '1'};
int counter = preambleSize;
int pos = 0;

void CalculateCRC(char dataFrame[]) {
    int polynom[4] = {1, 0, 1, 1};
    int k = preambleSize;
    int p = 4; // length of polynom
    int frame[8] = {0}; // n=k+p-1 buffer frame with perfect size for CRC

    // Copy directly from the binary input string
    for (int i = 0; i < k; i++) {
        frame[i] = dataFrame[i] - '0'; // converts a char number to the corresponding int number
    }

    // make the division
    int i = 0;
    while (i < k) {
        for (int j = 0; j < p; j++) {
            if (frame[i + j] == polynom[j]) {
                frame[i + j] = 0;
            } else {
                frame[i + j] = 1;
            }
        }
        while (i < 16 && frame[i] != 1)
            i++;
    }

    // CRC
    for (int j = k; j - k < p - 1; j++) {
        result[j] = (frame[j] == 1) ? '1' : '0';
    }
}

int main() {
    struct timeval tval_before, tval_after, tval_result;

    // Initialize GPIO using libgpiod
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    int ret;

    chip = gpiod_chip_open("/dev/gpiochip4");
    if (!chip) {
        perror("Error opening GPIO chip");
        return 1;
    }

    line = gpiod_chip_get_line(chip, 4); // Replace 4 with the actual GPIO pin number
    if (!line) {
        perror("Error getting GPIO line");
        gpiod_chip_close(chip);
        return 1;
    }

    // Read binary code
    char binaryCode[9]; // 8 bits + '\0'
    printf("\nEnter the 8-bit Binary Code: ");
    scanf("%8s", binaryCode);

    if (strlen(binaryCode) != binaryCodeSize) {
        fprintf(stderr, "Invalid binary code length. Please enter exactly 8 bits.\n");
        return 1;
    }

    // Copy directly to the result array
    for (int i = 0; i < binaryCodeSize; i++) {
        result[counter] = binaryCode[i];
        counter++;
    }

    // CRC Calculation
    CalculateCRC(result);

    // Display the message to be transmitted
    printf("Message to be transmitted: %s\n", result);

    gettimeofday(&tval_before, NULL);

    while (pos < 16) {
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
            ret = gpiod_line_set_value(line, 1);
            if (ret < 0) {
                perror("Error setting GPIO value");
                break;
            }
            pos++;
        } else if (result[pos] == '0') {
            ret = gpiod_line_set_value(line, 0);
            if (ret < 0) {
                perror("Error setting GPIO value");
                break;
            }
            pos++;
        }
    }

    // Close GPIO
    gpiod_line_release(line);
    gpiod_chip_close(chip);

    return 0;
}
