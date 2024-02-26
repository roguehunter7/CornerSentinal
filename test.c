#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <gpiod.h>

#define GPIO_CHIP_PATH "/dev/gpiochip4"
#define GPIO_PIN_NUMBER 4

typedef struct {
    char preamble[20];
    char payload_size[16];
    char payload[2500];
} wiremsg;

void initializeGPIO(gpiod_chip* chip, const char* chip_path, int pin_number, const char* consumer)
{
    chip = gpiod_chip_open(chip_path);
    if (!chip) {
        perror("Error opening GPIO chip");
        exit(EXIT_FAILURE);
    }

    gpiod_line* line = gpiod_chip_get_line(chip, pin_number);
    if (!line) {
        perror("Error getting GPIO line");
        gpiod_chip_close(chip);
        exit(EXIT_FAILURE);
    }

    if (gpiod_line_request_output(line, consumer, GPIOD_LINE_ACTIVE_STATE_HIGH) < 0) {
        perror("Error requesting GPIO output");
        gpiod_chip_close(chip);
        exit(EXIT_FAILURE);
    }
}

void cleanupGPIO(gpiod_chip* chip)
{
    gpiod_chip_close(chip);
}

void setPinState(gpiod_line* line, int value)
{
    gpiod_line_set_value(line, value);
}

void setHeaderPayloadSizeField(unsigned msgLen, wiremsg* lifiWireMsg_p)
{
    int payload_size_filed_len = sizeof(lifiWireMsg_p->payload_size);
    for (int i = 0; i < payload_size_filed_len; i++)
    {
        lifiWireMsg_p->payload_size[i] = (msgLen & (1 << (payload_size_filed_len - i - 1))) ? '1' : '0';
    }
    lifiWireMsg_p->payload_size[payload_size_filed_len] = '\0';
}

void setPayload(unsigned msgLen, wiremsg* lifiWireMsg_p, char* input_buffer)
{
    for (int i = 0; i < msgLen; i++)
    {
        for (int j = 7; j >= 0; j--)
        {
            lifiWireMsg_p->payload[i] = (input_buffer[i] & (1 << j)) ? '1' : '0';
        }
    }
}

int main()
{
    struct timeval tval_before, tval_after, tval_result;
    char input_buffer[2500];
    wiremsg lifiWireMsg = {
        .preamble = {'1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1'}};

    gpiod_chip* chip = NULL;
    gpiod_line* line = NULL;

    initializeGPIO(chip, GPIO_CHIP_PATH, GPIO_PIN_NUMBER, "lifi_gpio");

    while (1)
    {
        printf("Please input the message to send: ");
        scanf("%[^'\n']", input_buffer);
        int msg_len = strlen(input_buffer);
        setHeaderPayloadSizeField(msg_len, &lifiWireMsg);
        setPayload(msg_len, &lifiWireMsg, input_buffer);
        int bitPos = 0;

        int wireMsgLen = sizeof(lifiWireMsg.preamble) + sizeof(lifiWireMsg.payload_size) + msg_len;
        gettimeofday(&tval_before, NULL);

        while (bitPos <= wireMsgLen)
        {
            gettimeofday(&tval_after, NULL);
            timersub(&tval_after, &tval_before, &tval_result);
            double time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0f);

            while (time_elapsed < 0.001)
            {
                gettimeofday(&tval_after, NULL);
                timersub(&tval_after, &tval_before, &tval_result);
                time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0f);
            }
            gettimeofday(&tval_before, NULL);
            char bitToSend;

            if (bitPos < sizeof(lifiWireMsg.preamble))
            {
                bitToSend = lifiWireMsg.preamble[bitPos];
            }
            else if (bitPos < sizeof(lifiWireMsg.preamble) + sizeof(lifiWireMsg.payload_size))
            {
                bitToSend = lifiWireMsg.payload_size[bitPos - sizeof(lifiWireMsg.preamble)];
            }
            else
            {
                bitToSend = lifiWireMsg.payload[bitPos - sizeof(lifiWireMsg.preamble) - sizeof(lifiWireMsg.payload_size)];
            }

            // Set GPIO pin state
            setPinState(line, (bitToSend == '1') ? 1 : 0);
            bitPos++;
        }
    }

    cleanupGPIO(chip);
    return 0;
}
