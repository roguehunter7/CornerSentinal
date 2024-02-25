#include <stdio.h>
#include <string.h>
#include <gpiod.h>
#include <sys/time.h>

int main() {
    // Initialize libgpiod
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    
    chip = gpiod_chip_open("/dev/gpiochip4");
    if (!chip) {
        perror("Error opening GPIO chip");
        return -1;
    }

    line = gpiod_chip_get_line(chip, 4);
    if (!line) {
        perror("Error getting GPIO line");
        gpiod_chip_close(chip);
        return -1;
    }

    // Read message
    char msg[3000];
    printf("\nEnter the Message: ");
    if (scanf("%2999[^\n]", msg) != 1) {
        perror("Error reading input");
        gpiod_chip_close(chip);
        return -1;
    }

    // Initialization
    char result[3000] = {'1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1'};
    int counter = 20;
    int pos = 0;

    // Print frame header
    int len = strlen(msg);
    int2bin(len * 8, 16);
    printf("Frame Header (Synchro and Textlength = %s)\n", result);

    // Transmit data
    struct timeval tval_before, tval_after, tval_result;
    gettimeofday(&tval_before, NULL);
    
    while (pos != strlen(result)) {
        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);
        double time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0f);

        // Wait for the required time (1 ms)
        while (time_elapsed < 0.001) {
            gettimeofday(&tval_after, NULL);
            timersub(&tval_after, &tval_before, &tval_result);
            time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0f);
        }
        
        // Update the timestamp for the next iteration
        gettimeofday(&tval_before, NULL);

        // Set GPIO line based on the current bit
        gpiod_line_set_value(line, result[pos] == '1' ? 1 : 0);
        pos++;
    }

    // Cleanup libgpiod
    gpiod_line_set_value(line, 0); // Set GPIO line to LOW
    gpiod_line_release(line);
    gpiod_chip_close(chip);

    return 0;
}
