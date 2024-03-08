#include <stdio.h>
#include <malloc.h>
#include <sys/time.h>
#include <string.h>
#include <gpiod.h>

char result[3000] = {'1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1'};
int counter = 20;

void chartobin(char c)
{
    int i;
    for (i = 7; i >= 0; i--) {
        result[counter] = (c & (1 << i)) ? '1' : '0';
        counter++;
    }
}

void int2bin(unsigned integer, int n)
{
    for (int i = 0; i < n; i++) {
        result[counter] = (integer & (int)1 << (n - i - 1)) ? '1' : '0';
        counter++;
    }
    result[36] = '\0';
}

int pos = 0;

// Function to calculate CRC
void calculateCRC(char *data, int len, int *crc) {
    int poly[9] = {1, 0, 0, 1, 0, 1, 1, 1, 1};  // CRC-8 polynomial
    int crc_val = 0;

    for (int i = 0; i < len; i++) {
        crc_val ^= (data[i] == '1') << 7;
        for (int j = 0; j < 8; j++) {
            crc_val = (crc_val << 1) ^ ((crc_val & 0x80) ? 0x09 : 0);
        }
    }

    *crc = crc_val;
}

int main()
{
    struct timeval tval_before, tval_after, tval_result;
    struct gpiod_chip *chip;
    struct gpiod_line *line;

    // Open GPIO chip
    chip = gpiod_chip_open("/dev/gpiochip4");
    if (!chip) {
        perror("Failed to open GPIO chip");
        return 1;
    }

    // Get GPIO line
    line = gpiod_chip_get_line(chip, 4);
    if (!line) {
        perror("Failed to get GPIO line");
        gpiod_chip_close(chip);
        return 1;
    }

    // Configure GPIO line as output
    if (gpiod_line_request_output(line, "example", 0) != 0) {
        perror("Failed to configure GPIO line as output");
        gpiod_line_release(line);
        gpiod_chip_close(chip);
        return 1;
    }

    // Read message
    char msg[3000];
    int len, k, length;
    printf("\n Enter the Message: ");
    scanf("%[^\n]", msg);
    len = strlen(msg);
    int2bin(len * 8, 16); // Multiply by 8 because one byte is 8 bits
    printf("Frame Header (Synchro and Textlength = %s\n", result);

    for (k = 0; k < len; k++) {
        chartobin(msg[k]);
    }

    // Calculate CRC for the message data
    int crc_val;
    calculateCRC(result + 36, counter - 36, &crc_val);

    // Append CRC value to the binary data
    for (int i = 7; i >= 0; i--) {
        result[counter++] = ((crc_val >> i) & 1) ? '1' : '0';
    }

    printf("Frame Header (Synchro and Textlength and CRC) = %s\n", result);

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

    // Clean up
    gpiod_line_release(line);
    gpiod_chip_close(chip);

    return 0;
}