#include <stdio.h>
#include <gpiod.h>
#include <unistd.h>
int main()
{
    struct gpiod_chip *chip;
    struct gpiod_line *line;

    chip = gpiod_chip_open("/dev/gpiochip4");
    if (!chip)
    {
        perror("Error opening GPIO chip");
        return -1;
    }

    line = gpiod_chip_get_line(chip, 4);

    // Toggle GPIO line
    gpiod_line_set_value(line, 1);
    usleep(100000);
    gpiod_line_set_value(line, 0);

    // Cleanup libgpiod
    gpiod_line_release(line);
    gpiod_chip_close(chip);

    return 0;
}
