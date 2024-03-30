#include <stdio.h>
#include <malloc.h>
#include <sys/time.h>
#include <string.h>
#include <gpiod.h>

char result[3000] = {0};
int counter = 20;
int pos = 0;

char *manchester_encode(const char *input, int *encoded_len) {
    int len = strlen(input);
    char *encoded = malloc((len * 2 + 1) * sizeof(char));
    int encoded_pos = 0;

    for (int i = 0; i < len; i++) {
        if (input[i] == '0') {
            encoded[encoded_pos++] = '0';
            encoded[encoded_pos++] = '1';
        } else if (input[i] == '1') {
            encoded[encoded_pos++] = '1';
            encoded[encoded_pos++] = '0';
        }
    }

    encoded[encoded_pos] = '\0';
    *encoded_len = encoded_pos;
    return encoded;
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

    // Append preamble and Manchester encode
    strcpy(result, "10101010101111111111");
    char *encoded_msg = manchester_encode(msg, &len);
    strncat(result, encoded_msg, len);
    free(encoded_msg);

    printf("Frame Header (Synchro and Text) = %s\n", result);
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

    // Cleanup
    gpiod_line_set_value(line, 0);
    gpiod_line_release(line);
    gpiod_chip_close(chip);
    return 0;
}