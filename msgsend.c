#include <stdio.h>
#include <string.h>
#include <gpiod.h>
#include <sys/time.h>

#define RESULT_SIZE 3000
#define GPIO_CHIP_PATH "/dev/gpiochip4"
#define GPIO_LINE_NUMBER 17
#define DELAY_THRESHOLD 0.001

char result[RESULT_SIZE] = {'1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1'};
int counter = 20;

void chartobin(char c)
{
    for (int i = 7; i >= 0; i--)
    {
        result[counter] = (c & (1 << i)) ? '1' : '0';
        counter++;
    }
}

void int2bin(unsigned integer, int n)
{
    for (int i = 0; i < n; i++)
    {
        result[counter] = (integer & (1 << (n - i - 1))) ? '1' : '0';
        counter++;
    }
}

int pos = 0;

void cleanup(struct gpiod_line *line, struct gpiod_chip *chip)
{
    gpiod_line_set_value(line, 0); // Set GPIO line to LOW before releasing
    gpiod_line_release(line);
    gpiod_chip_close(chip);
}

int main()
{
    // Initialize libgpiod
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    chip = gpiod_chip_open(GPIO_CHIP_PATH);
    if (!chip)
    {
        perror("Error opening GPIO chip");
        return -1;
    }

    line = gpiod_chip_get_line(chip, GPIO_LINE_NUMBER);
    if (!line)
    {
        perror("Error getting GPIO line");
        cleanup(line, chip);
        return -1;
    }

    // Read message
    char msg[RESULT_SIZE]; // Limiting message length to the size of the result array
    printf("\nEnter the Message: ");
    scanf("%2999[^\n]", msg); // Limit input to avoid buffer overflow
    int len = strlen(msg);

    int2bin(len * 8, 16);
    printf("Frame Header (Synchro and Textlength) = %s\n", result);

    for (int k = 0; k < len; k++)
    {
        chartobin(msg[k]);
    }

    int length = strlen(result);
    struct timeval tval_before, tval_after, tval_result;

    gettimeofday(&tval_before, NULL);
    while (pos != length)
    {
        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);
        double time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0f);

        while (time_elapsed < DELAY_THRESHOLD)
        {
            gettimeofday(&tval_after, NULL);
            timersub(&tval_after, &tval_before, &tval_result);
            time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0f);
        }
        gettimeofday(&tval_before, NULL);

        if (result[pos] == '1')
        {
            gpiod_line_set_value(line, 1); // Set GPIO line to HIGH
            pos++;
        }
        else if (result[pos] == '0')
        {
            gpiod_line_set_value(line, 0); // Set GPIO line to LOW
            pos++;
        }
    }

    // Cleanup libgpiod
    cleanup(line, chip);

    return 0;
}
