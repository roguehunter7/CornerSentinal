#include <stdio.h>
#include <malloc.h>
#include <sys/time.h>
#include <string.h>
#include <gpiod.h>

char result[3000] = {'1','0','1','0','1','0','1','0','1','0','1','1','1','1','1','1','1','1','1','1'};
int counter = 20;

void chartobin(char c) {
    int i;
    for(i = 7; i >= 0;i--) {
        result[counter] = (c & (1 << i)) ? '1' : '0';  
        counter++;
    }
}

void int2bin(unsigned integer, int n) {  
  for (int i = 0; i < n; i++)  
  {
    result[counter]= (integer & (int)1<<(n-i-1)) ? '1' : '0';
    result[36]='\0';
    counter++;
  }
}

int pos = 0;

int main() {
    struct timeval tval_before, tval_after, tval_result;

    // libgpiod setup
    const char *chipname = "gpiochip0"; // Adjust if necessary
    unsigned int line_num = 0;          // Replace with your desired GPIO line
    gpiod_chip *chip = gpiod_chip_open_by_name(chipname);
    if (!chip) {
        perror("Error opening GPIO chip\n");
        return -1;
    }
    gpiod_line *line = gpiod_line_request_output(chip, line_num, "my_output", 0);
    if (!line) {
        perror("Error requesting GPIO line\n");
        return -1;
    }

    // Read message
    char msg[3000]; 
    int len, k, length;

    printf("\n Enter the Message: ");
    scanf("%[^'\n']",msg);

    len = strlen(msg);

    int2bin(len*8, 16); 
    printf ("Frame Header (Synchro and Textlength = %s\n", result);

    for(k = 0; k < len; k++) {
        chartobin(msg[k]);       
    }   
    length = strlen(result);

    gettimeofday(&tval_before, NULL);

    while(pos!=length) {
        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);
        double time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec/1000000.0f);

        while(time_elapsed < 0.001) {
            gettimeofday(&tval_after, NULL);
            timersub(&tval_after, &tval_before, &tval_result);
            time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec/1000000.0f);
        }
        gettimeofday(&tval_before, NULL);

        if (result[pos]=='1') {
            gpiod_line_set_value(line, 1);  // HIGH
            pos++;
        } else { 
            gpiod_line_set_value(line, 0);  // LOW
            pos++;
        }
    }

    // Cleanup
    gpiod_line_release(line);
    gpiod_chip_close(chip);

    return 0;
}
