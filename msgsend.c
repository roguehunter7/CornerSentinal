#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gpiod.h>
#include <sys/time.h>
#include <errno.h>

typedef struct {
    char preamble[20];
    char payload_size[16];
    char payload[2500];
} wiremsg;

void chartobin(char c, char* result, int* counter) {
    int i;
    for (i = 7; i >= 0; i--) {
        result[*counter] = (c & (1 << i)) ? '1' : '0';
        (*counter)++;
    }
}

void setHeaderPayloadSizeField(unsigned msgLen, wiremsg* lifiWireMsg_p) {
    int payload_size_filed_len = sizeof(lifiWireMsg_p->payload_size);
    for (int i = 0; i < payload_size_filed_len; i++) {
        lifiWireMsg_p->payload_size[i] = (msgLen & (int)1 << (payload_size_filed_len - i - 1)) ? '1' : '0';
    }
    lifiWireMsg_p->payload_size[payload_size_filed_len] = '\0';
}

void setPayload(unsigned msgLen, wiremsg* lifiWireMsg_p, char* input_buffer) {
    for (int i = 0; i < msgLen; i++) {
        for (int j = 7; j >= 0; j--) {
            lifiWireMsg_p->payload[i * 8 + (7 - j)] = (input_buffer[i] & (1 << j)) ? '1' : '0';
        }
    }
    lifiWireMsg_p->payload[msgLen * 8] = '\0';
}

int main() {
    struct timeval tval_before, tval_after, tval_result;
    char input_buffer[2500];
    wiremsg lifiWireMsg = {
        .preamble = "10101010101111111111"
    };

    const char* chip_name = "gpiochip4";
    struct gpiod_chip* chip = gpiod_chip_open_by_name(chip_name);
    if (!chip) {
        fprintf(stderr, "Failed to open GPIO chip %s: %s\n", chip_name, strerror(errno));
        return 1;
    }

    int line_num = 4;
    struct gpiod_line* line = gpiod_chip_get_line(chip, line_num);
    if (!line) {
        fprintf(stderr, "Failed to get GPIO line %d: %s\n", line_num, strerror(errno));
        gpiod_chip_close(chip);
        return 1;
    }

    int request_output_status = gpiod_line_request_output(line, "lifi", 0);
    if (request_output_status < 0) {
        fprintf(stderr, "Failed to request GPIO line %d as output: %s\n", line_num, strerror(-request_output_status));
        gpiod_line_release(line);
        gpiod_chip_close(chip);
        return 1;
    }

    while (1) {
        printf("Please input the message to send: ");
        if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
            fprintf(stderr, "Error reading input: %s\n", strerror(errno));
            break;
        }
        int msg_len = strlen(input_buffer) - 1;  // remove newline character

        setHeaderPayloadSizeField(msg_len, &lifiWireMsg);
        setPayload(msg_len, &lifiWireMsg, input_buffer);

        int bitPos = 0;
        int wireMsgLen = strlen(lifiWireMsg.preamble) + strlen(lifiWireMsg.payload_size) + msg_len * 8;

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
            if (bitPos < strlen(lifiWireMsg.preamble)) {
                bitToSend = lifiWireMsg.preamble[bitPos];
            } else if (bitPos < strlen(lifiWireMsg.preamble) + strlen(lifiWireMsg.payload_size)) {
                bitToSend = lifiWireMsg.payload_size[bitPos - strlen(lifiWireMsg.preamble)];
            } else {
                bitToSend = lifiWireMsg.payload[bitPos - strlen(lifiWireMsg.preamble) - strlen(lifiWireMsg.payload_size)];
            }

            int set_value_status = gpiod_line_set_value(line, bitToSend == '1' ? 1 : 0);
            if (set_value_status < 0) {
                fprintf(stderr, "Failed to set GPIO line %d value: %s\n", line_num, strerror(-set_value_status));
                break;
            }

            bitPos++;
        }
    }

    gpiod_line_release(line);
    gpiod_chip_close(chip);

    return 0;
}