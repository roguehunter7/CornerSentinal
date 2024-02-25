#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <gpiod.h>

#define PREAMBLE_LEN 20
#define PAYLOAD_SIZE_LEN 16
#define PAYLOAD_LEN 2500

typedef struct {
    char preamble[PREAMBLE_LEN];
    char payload_size[PAYLOAD_SIZE_LEN];
    char payload[PAYLOAD_LEN];
} wiremsg;

void int2bin(unsigned integer, int n, char *result, int *counter) {
    for (int i = 0; i < n; i++) {
        result[*counter] = (integer & (int)1 << (n - i - 1)) ? '1' : '0';
        (*counter)++;
    }
}

void setHeaderPayloadSizeField(unsigned msgLen, wiremsg *lifiWireMsg_p) {
    for (int i = 0; i < PAYLOAD_SIZE_LEN; i++) {
        lifiWireMsg_p->payload_size[i] = (msgLen & (int)1 << (PAYLOAD_SIZE_LEN - i - 1)) ? '1' : '0';
    }
    lifiWireMsg_p->payload_size[PAYLOAD_SIZE_LEN] = '\0';
}

void setPayload(unsigned msgLen, wiremsg *lifiWireMsg_p, char *input_buffer) {
    int counter = 0;
    for (int i = 0; i < msgLen; i++) {
        for (int j = 7; j >= 0; j--) {
            lifiWireMsg_p->payload[counter++] = (input_buffer[i] & (1 << j)) ? '1' : '0';
        }
    }
}

int main() {
    struct timeval tval_before, tval_after, tval_result;
    char input_buffer[PAYLOAD_LEN];
    wiremsg lifiWireMsg = {.preamble = {'1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1'}};

    struct gpiod_chip *chip;
    struct gpiod_line_bulk lines;
    unsigned int line_offset = 0;
    const char *consumer = "lifi_consumer";

    chip = gpiod_chip_open("/dev/gpiochip0");
    if (!chip) {
        perror("Error opening GPIO chip");
        return -1;
    }

    gpiod_chip_get_lines(chip, line_offset, &lines);
    gpiod_line_request_output_lines_bulk(&lines, consumer, 0);

    while (1) {
        printf("Please input the message to send: ");
        fgets(input_buffer, sizeof(input_buffer), stdin);
        int msg_len = strlen(input_buffer) - 1;  // Exclude newline character
        setHeaderPayloadSizeField(msg_len, &lifiWireMsg);
        setPayload(msg_len, &lifiWireMsg, input_buffer);

        int wireMsgLen = PREAMBLE_LEN + PAYLOAD_SIZE_LEN + msg_len;

        gettimeofday(&tval_before, NULL);

        for (int bitPos = 0; bitPos < wireMsgLen; bitPos++) {
            gettimeofday(&tval_after, NULL);
            timersub(&tval_after, &tval_before, &tval_result);
            double time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0f);

            while (time_elapsed < 0.001) {
                gettimeofday(&tval_after, NULL);
                timersub(&tval_after, &tval_before, &tval_result);
                time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0f);
            }

            gettimeofday(&tval_before, NULL);

            char bitToSend;
            if (bitPos < PREAMBLE_LEN) {
                bitToSend = lifiWireMsg.preamble[bitPos];
            } else if (bitPos < PREAMBLE_LEN + PAYLOAD_SIZE_LEN) {
                bitToSend = lifiWireMsg.payload_size[bitPos - PREAMBLE_LEN];
            } else {
                bitToSend = lifiWireMsg.payload[bitPos - PREAMBLE_LEN - PAYLOAD_SIZE_LEN];
            }

            gpiod_line_set_value_bulk(&lines, bitToSend == '1');
        }
    }

    gpiod_chip_close(chip);
    return 0;
}
