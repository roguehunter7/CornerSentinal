#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <gpiod.h>
#include <sys/time.h>

char result[3000] = {'1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1'};
int counter = 20;

void chartobin(char c)
{
    int i;
    for(i=7; i>=0;i--){
        result[counter]= (c&(1<<i))?'1':'0';  
        counter++;
    }
}

void int2bin(unsigned integer, int n)
{  
    for (int i=0;i<n;i++)   
    {
    result[counter]= (integer & (int)1<<(n-i-1)) ? '1' : '0';
    result[36]='\0';
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
    struct timeval tval_before, tval_after, tval_result;
    // Read message
    char msg[3000]; 
        int len, k, length;
       
        printf("\n Enter the Message: ");
        scanf("%[^'\n']",msg);
        
        len=strlen(msg);
        
        
        int2bin(len*8, 16); 
        printf ("Frame Header (Synchro and Textlength = %s\n", result);
        
        for(k=0;k<len;k++)
        {
            chartobin(msg[k]);            
        }
        
    length = strlen(result);
    gettimeofday(&tval_before, NULL);
    while(pos!=length)
    {
        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);
        double time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec/1000000.0f);
        
        while(time_elapsed < 0.001)
        {
            gettimeofday(&tval_after, NULL);
            timersub(&tval_after, &tval_before, &tval_result);
            time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec/1000000.0f);
        }
        gettimeofday(&tval_before, NULL);
        
        if (result[pos]=='1')
        {
            gpiod_line_set_value(line, 1);// Set GPIO line to HIGH
            pos++;
            }
            
        else if(result[pos]=='0'){
            gpiod_line_set_value(line, 0); // Set GPIO line to LOW
            pos++;
            }  
    }

    // Cleanup libgpiod
    gpiod_line_set_value(line, 0); // Set GPIO line to LOW
    gpiod_line_release(line);
    gpiod_chip_close(chip);

    return 0;
}
