#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <gpiod.h>

#define GPIO_CHIP_NAME "gpiochip4"  // GPIO chip name
#define GPIO_PIN_NUMBER 4            // GPIO pin number

char result[3000] = {'1','0','1','0','1','0','1','0','1','0','1','1','1','1','1','1','1','1','1','1'};
int counter = 20;

void chartobin(char c)
{
    int i;
    for(i = 7; i >= 0; i--){
        result[counter] = (c & (1 << i)) ? '1' : '0';  
        counter++;
    }
}

void int2bin(unsigned integer, int n)
{  
    for (int i = 0; i < n; i++)   
    {
        result[counter] = (integer & (1 << (n - i - 1))) ? '1' : '0';
        result[36] = '\0';
        counter++;
    }
}

int pos = 0;

int main()
{
    struct timeval tval_before, tval_after, tval_result;
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    
    chip = gpiod_chip_open_by_name(GPIO_CHIP_NAME);
    if (!chip) {
        perror("Error opening GPIO chip");
        return EXIT_FAILURE;
    }

    line = gpiod_chip_get_line(chip, GPIO_PIN_NUMBER);
    if (!line) {
        perror("Error getting GPIO line");
        gpiod_chip_close(chip);
        return EXIT_FAILURE;
    }

    if (gpiod_line_request_output(line, "gpio-output", 0) < 0) {
        perror("Error setting GPIO line as output");
        gpiod_line_release(line);
        gpiod_chip_close(chip);
        return EXIT_FAILURE;
    }

    // Read message
    char msg[3000];
    int len, k, length;

    printf("\nEnter the Message: ");
    scanf("%[^'\n']", msg);

    len = strlen(msg);

    int2bin(len * 8, 16); // Multiply by 8 because one byte is 8 bits
    printf("Frame Header (Synchro and Textlength = %s\n", result);

    for (k = 0; k < len; k++)
    {
        chartobin(msg[k]);
    }

    length = strlen(result);
    gettimeofday(&tval_before, NULL);
    while (pos != length)
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

        if (result[pos] == '1')
        {
            gpiod_line_set_value(line, 1);
            pos++;
        }
        else if (result[pos] == '0')
        {
            gpiod_line_set_value(line, 0);
            pos++;
        }
    }

    gpiod_line_release(line);
    gpiod_chip_close(chip);

    return EXIT_SUCCESS;
}
