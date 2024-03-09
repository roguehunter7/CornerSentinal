#include <stdio.h>
#include <gpiod.h>
#include <sys/time.h>

#define PREAMBLE 0x15 // Binary: 10101
#define MSG_LEN 8
#define CRC_LEN 3
#define FRAME_LEN (MSG_LEN + CRC_LEN)
#define TOTAL_LEN (FRAME_LEN + 5)

unsigned char frame[FRAME_LEN];

void calculate_crc(unsigned char *data, int len) {
    unsigned char crc = 0;
    unsigned char polynomial = 0xB; // x^3 + x + 1

    for (int i = 0; i < len; i++) {
        crc ^= (data[i] << (CRC_LEN - 1));
        for (int j = 0; j < CRC_LEN; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc = (crc << 1);
            }
        }
    }

    for (int i = 0; i < CRC_LEN; i++) {
        frame[len + i] = (crc & (1 << (CRC_LEN - 1 - i))) ? 1 : 0;
    }
}

void delay_ms(double ms) {
    struct timeval tval_before, tval_after, tval_result;
    gettimeofday(&tval_before, NULL);

    while (1) {
        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);
        double time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0f);

        if (time_elapsed >= ms / 1000.0)
            break;
    }
}

int main() {
    struct gpiod_chip *chip;
    struct gpiod_line *line;

    chip = gpiod_chip_open("/dev/gpiochip4");
    if (!chip) {
        perror("Failed to open GPIO chip");
        return 1;
    }

    line = gpiod_chip_get_line(chip, 4);
    if (!line) {
        perror("Failed to get GPIO line");
        gpiod_chip_close(chip);
        return 1;
    }

    if (gpiod_line_request_output(line, "example", 0) < 0) {
        perror("Failed to configure GPIO line as output");
        gpiod_line_release(line);
        gpiod_chip_close(chip);
        return 1;
    }

    unsigned char msg[MSG_LEN];
    printf("Enter an 8-bit binary code: ");
    scanf("%8hx", &msg);

    for (int i = 0; i < MSG_LEN; i++) {
        frame[i] = ((msg[MSG_LEN - 1 - i / 8] >> (i % 8)) & 1);
    }

    calculate_crc(frame, MSG_LEN);

    for (int i = 0; i < 5; i++) {
        gpiod_line_set_value(line, (PREAMBLE >> (4 - i)) & 1);
        delay_ms(1);
    }

    for (int i = 0; i < TOTAL_LEN; i++) {
        gpiod_line_set_value(line, frame[i % FRAME_LEN]);
        delay_ms(1);
    }

    gpiod_line_release(line);
    gpiod_chip_close(chip);

    return 0;
}