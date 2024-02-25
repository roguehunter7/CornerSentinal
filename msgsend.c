#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <gpiod.h>

typedef struct {
    char preamble[20];
    char payload_size[16];
    char payload[2500];
} wiremsg;

void int2bin(unsigned integer, int n, char *result, int *counter) {
    for (int i = 0; i < n; i++) {
        result[*counter] = (integer & (int)1 << (n - i - 1)) ? '1' : '0';
        (*counter)++;
    }
}

void setHeaderPayloadSizeField(unsigned msgLen, wiremsg *lifiWireMsg_p) {
    int payload_size_filed_len = sizeof(lifiWireMsg_p->payload_size);
    int counter = 0;
    for (int i = 0; i < payload_size_filed_len; i++) {
        lifiWireMsg_p->payload_size[i] = (msgLen & (int)1 << (payload_size_filed_len - i - 1)) ? '1' : '0';
    }
    lifiWireMsg_p->payload_size[payload_size_filed_len] = '\0';
}

void setPayload(unsigned msgLen, wiremsg *lifiWireMsg_p, char *input_buffer) {
    int counter = 0;
    for (int i = 0; i < msgLen; i++) {
        for (int j = 7; j >= 0; j--) {
            lifiWireMsg_p->payload[counter] = (input_buffer[i] & (1 << j)) ? '1' : '0';
            counter++;
        }
    }
}

int main() {
    struct timeval tval_before, tval_after, tval_result;
    char input_buffer[2500];
    wiremsg lifiWireMsg = {.preamble = {'1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1'}};

    struct gpiod_chip *chip;
    struct gpiod_line *line;
    unsigned int line_offset = 4;  // GPIO pin 4
    const char *consumer = "lifi_consumer";

    chip = gpiod_chip_open("/dev/gpiochip0");
    if (!chip) {
        perror("Error opening GPIO chip");
        return -1;
    }

    line = gpiod_chip_get_line(chip, line_offset);
    if (!line) {
        perror("Error getting GPIO line");
        gpiod_chip_close(chip);
        return -1;
    }

    gpiod_line_request_output(line, consumer, 0);

    while (1) {
        printf("Please input the message to send: ");
        fgets(input_buffer, sizeof(input_buffer), stdin);
        int msg_len = strlen(input_buffer);
        setHeaderPayloadSizeField(msg_len, &lifiWireMsg);
        setPayload(msg_len, &lifiWireMsg, input_buffer);
        int bitPos = 0;

        int wireMsgLen = sizeof(lifiWireMsg.preamble) + sizeof(lifiWireMsg.payload_size) + msg_len;

        gettimeofday(&tval_before, NULL);

        while (bitPos < wireMsgLen) {
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
            if (bitPos < sizeof(lifiWireMsg.preamble)) {
                bitToSend = lifiWireMsg.preamble[bitPos];
            } else if (bitPos < sizeof(lifiWireMsg.preamble) + sizeof(lifiWireMsg.payload_size)) {
                bitToSend = lifiWireMsg.payload_size[bitPos - sizeof(lifiWireMsg.preamble)];
            } else {
                bitToSend = lifiWireMsg.payload[bitPos - sizeof(lifiWireMsg.preamble) - sizeof(lifiWireMsg.payload_size)];
            }

            gpiod_line_set_value(line, (bitToSend == '1'));

            bitPos++;
        }
    }

    gpiod_line_release(line);
    gpiod_chip_close(chip);
    return 0;
}
