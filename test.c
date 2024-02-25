#include <stdio.h>
#include <string.h>
#include <gpiod.h>
#include <unistd.h>

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
    if (!line)
    {
        perror("Error getting GPIO line");
        gpiod_chip_close(chip);
        return -1;
    }

    // Configure the GPIO line
    if (gpiod_line_request_output(line, "led-control", 0) < 0)
    {
        perror("Error configuring GPIO line");
        gpiod_line_release(line);
        gpiod_chip_close(chip);
        return -1;
    }

    // Toggle the LED
    for (int i = 0; i < 5; ++i) // Toggle the LED 5 times
    {
        gpiod_line_set_value(line, 1); // Set GPIO line to HIGH
        sleep(1);                      // Sleep for 1 second
        gpiod_line_set_value(line, 0); // Set GPIO line to LOW
        sleep(1);                      // Sleep for 1 second
    }

    // Cleanup libgpiod
    gpiod_line_release(line);
    gpiod_chip_close(chip);

    return 0;
}
