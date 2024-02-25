#include <stdio.h>
#include <string.h>
#include <gpiod.h>

char result[3000] = {'1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1'};
int counter = 20;

void chartobin(char c)
{
    int i;
    for (i = 7; i >= 0; i--)
    {
        result[counter] = (c & (1 << i)) ? '1' : '0';
        counter++;
    }
}

void int2bin(unsigned integer, int n)
{
    for (int i = 0; i < n; i++)
    {
        result[counter] = (integer & (int)1 << (n - i - 1)) ? '1' : '0';
        result[36] = '\0';
        counter++;
    }
}

int pos = 0;

int main()
{
    // Initialize libgpiod
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    chip = gpiod_chip_open("/dev/gpiochip4");
    if (!chip)
    {
        perror("Error opening GPIO chip");
        return -1;
    }

    line = gpiod_chip_get_line(chip, 4);

    // Read message
    char msg[3000];
    int len, k, length;

    printf("\n Enter the Message: ");
    scanf("%[^'\n']", msg);

    len = strlen(msg);

    int2bin(len * 8, 16);
    printf("Frame Header (Synchro and Textlength = %s\n", result);

    for (k = 0; k < len; k++)
    {
        chartobin(msg[k]);
    }

    length = strlen(result);

    while (pos != length)
    {
        if (result[pos] == '1')
        {
            gpiod_line_set_value(line, 1); // Set GPIO line to HIGH
            usleep(100000);                // Sleep for 100 ms (adjust as needed)
            gpiod_line_set_value(line, 0); // Set GPIO line to LOW
        }
        else if (result[pos] == '0')
        {
            gpiod_line_set_value(line, 0); // Set GPIO line to LOW directly
        }

        pos++;
    }

    // Cleanup libgpiod
    gpiod_line_release(line);
    gpiod_chip_close(chip);

    return 0;
}
