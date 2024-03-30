#include <stdio.h>
#include <malloc.h>
#include <sys/time.h>
#include <string.h>
#include <gpiod.h>

char result[3000] = {'1','0','1','0','1','0','1','0','1','0','1','1','1','1','1','1','1','1','1','1'};
int counter = 20;
int pos = 0;


void int2bin(unsigned integer, int n)
{  
    for (int i = 0; i < n; i++)   
    {
        result[counter] = (integer & (int)1 << (n - i - 1)) ? '1' : '0';
        result[36] = '\0';
        counter++;
    }
}

int main()
{
    struct timeval tval_before, tval_after, tval_result;

    // GPIO Initialization
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    chip = gpiod_chip_open("/dev/gpiochip4");
    if (!chip) {
        perror("Open chip failed\n");
        return 1;
    }
    line = gpiod_chip_get_line(chip, 4);
    if (!line) {
        perror("Get line failed\n");
        gpiod_chip_close(chip);
        return 1;
    }
    int ret = gpiod_line_request_output(line, "example", 0);
    if (ret < 0) {
        perror("Request line as output failed\n");
        gpiod_chip_close(chip);
        return 1;
    }

    // Read message
    char msg[3000];
    int len, k, length;
    gpiod_line_set_value(line, 0);

    printf("\n Enter the Message: ");
    scanf("%[^'\n']", msg);

    for(int i = 0; i<8 ; i++){
        
        result[counter+1+i] == msg[i];
    
    }    
    printf ("Frame Header (Synchro and Text) = %s\n", result);
    length = strlen(result);
    gettimeofday(&tval_before, NULL);
    while(pos != length)
    {
        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);
        double time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0f);

        while(time_elapsed < 0.001)
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
        else if(result[pos] == '0') {
            gpiod_line_set_value(line, 0);
            pos++;
        }
    }

    // Cleanup
    gpiod_line_set_value(line, 0);
    gpiod_line_release(line);
    gpiod_chip_close(chip);

    return 0;
}
