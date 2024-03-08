#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <gpiod.h>

// CRC related functions
void division(int frame_copy[], int poly[], int n, int k, int p) {
    int i = 0;
    while (i < k) {
        for (int j = 0; j < p; j++) {
            frame_copy[i + j] = (frame_copy[i + j] == poly[j]) ? 0 : 1;
        }

        while (i < n && frame_copy[i] != 1)
            i++;
    }
}

void calculateCRC(char *datas, char *polys, int n, int k, int p, int crc[]) {
    int frame_copy[n];
    int poly[p];

    for (int i = 0; i < p; i++) {
        poly[i] = polys[i] - '0';
    }

    for (int i = 0; i < n; i++) {
        if (i < k) {
            frame_copy[i] = datas[i] - '0';
        } else {
            frame_copy[i] = 0;
        }
    }

    division(frame_copy, poly, n, k, p);

    for (int i = 0, j = k; i < p - 1; i++, j++) {
        crc[i] = frame_copy[j];
    }
}

// Struct for wire message
typedef struct {
    char preamble[5];  // Updated preamble to "10101"
    char payload_size[8];
    char payload[8];
    char crc[3];  // Adjusted CRC length to 3 bits (degree of polynomial - 1)
} wiremsg;

void setHeaderPayloadSizeField(unsigned msgLen, wiremsg *lifiWireMsg_p) {
    int payload_size_field_len = sizeof(lifiWireMsg_p->payload_size);
    for (int i = 0; i < payload_size_field_len; i++) {
        lifiWireMsg_p->payload_size[i] = (msgLen & (1 << (payload_size_field_len - i - 1))) ? '1' : '0';
    }
    lifiWireMsg_p->payload_size[payload_size_field_len] = '\0';
}

void setPayload(unsigned msgLen, wiremsg *lifiWireMsg_p, char *input_buffer) {
    for (int i = 0; i < msgLen; i++) {
        for (int j = 7; j >= 0; j--) {
            lifiWireMsg_p->payload[i * 8 + (7 - j)] = (input_buffer[i] & (1 << j)) ? '1' : '0';
        }
    }
}

void sendBinaryCode(const char *binary_code) {
    struct timeval tval_before, tval_after, tval_result;
    wiremsg lifiWireMsg = {
        .preamble = {'1', '0', '1', '0', '1'},  // Updated preamble to "10101"
    };

    struct gpiod_chip *chip;
    struct gpiod_line *line;
    struct gpiod_line_request_config config = {
        .request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT,
        .flags = GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN,  // Use open drain for simplicity
        .consumer = "lifi_consumer",  // Replace with an appropriate consumer name
    };

    // Open the GPIO chip for gpiochip4
    chip = gpiod_chip_open("/dev/gpiochip4");
    if (!chip) {
        perror("Error opening GPIO chip");
        return;
    }

    // Configure GPIO pin 4 as output
    line = gpiod_chip_get_line(chip, 4);  // GPIO pin 4
    if (!line) {
        perror("Error getting GPIO line");
        gpiod_chip_close(chip);
        return;
    }

    gpiod_line_request_output(line, &config, 0);
    gpiod_line_release(line);

    int msg_len = strlen(binary_code);

    // Calculate CRC with updated polynomial "1011"
    int crc[3];  // Corrected CRC length to 3 bits (degree of polynomial - 1)
    calculateCRC(binary_code, "1011", msg_len + 3, msg_len, 4, crc);  // Updated CRC length to 3 bits

    // Set header, payload, and CRC
    setHeaderPayloadSizeField(msg_len, &lifiWireMsg);
    setPayload(msg_len, &lifiWireMsg, binary_code);
    for (int i = 0; i < 3; i++) {  // Corrected CRC length to 3 bits
        lifiWireMsg.crc[i] = crc[i] + '0';
    }

    int bit_pos = 0;
    int wire_msg_len = sizeof(lifiWireMsg.preamble) + sizeof(lifiWireMsg.payload_size) + sizeof(lifiWireMsg.payload) + sizeof(lifiWireMsg.crc);

    gettimeofday(&tval_before, NULL);

    while (bit_pos < wire_msg_len) {
        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);
        double time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0f);

        while (time_elapsed < 0.001) {
            gettimeofday(&tval_after, NULL);
            timersub(&tval_after, &tval_before, &tval_result);
            time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0f);
        }

        gettimeofday(&tval_before, NULL);

        char bit_to_send;
        if (bit_pos < sizeof(lifiWireMsg.preamble)) {
            bit_to_send = lifiWireMsg.preamble[bit_pos];
        } else if (bit_pos < sizeof(lifiWireMsg.preamble) + sizeof(lifiWireMsg.payload_size)) {
            bit_to_send = lifiWireMsg.payload_size[bit_pos - sizeof(lifiWireMsg.preamble)];
        } else if (bit_pos < sizeof(lifiWireMsg.preamble) + sizeof(lifiWireMsg.payload_size) +
                        sizeof(lifiWireMsg.payload)) {
            bit_to_send = lifiWireMsg.payload[bit_pos - sizeof(lifiWireMsg.preamble) - sizeof(lifiWireMsg.payload_size)];
        } else {
            bit_to_send = lifiWireMsg.crc[bit_pos - sizeof(lifiWireMsg.preamble) - sizeof(lifiWireMsg.payload_size) -
                                           sizeof(lifiWireMsg.payload)];
        }

        gpiod_line_set_value(line, bit_to_send == '1' ? 1 : 0);
        bit_pos++;
    }

    gpiod_chip_close(chip);
}

int main() {
    // Example usage
    char binary_code = "1101101";  // Replace with your binary code
    sendBinaryCode(binary_code);

    return 0;
}
