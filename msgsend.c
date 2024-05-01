#include <stdio.h>
#include <malloc.h>
#include <sys/time.h>
#include <string.h>
#include <gpiod.h>


char result[3000] = {0};
int counter = 20;
int pos = 0;

void custom_delay(double milliseconds) {
    struct timeval tval_before, tval_after, tval_result;
    gettimeofday(&tval_before, NULL);
    double time_elapsed = 0;
    while (time_elapsed < milliseconds / 1000.0) {
        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);
        time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0);
    }
}

void division(int frame_copy[],int poly[],int n,int k,int p)
{
	int i=0;
	while (  i <  k  )			
	{											
		for( int j=0 ; j < p ; j++)							
		{
			
				if( frame_copy[i+j] == poly[j] )	
        			{
                    		frame_copy[i+j]=0;
				}
        			else
		 		{
                    		frame_copy[i+j]=1;
				}			
		}
		
		while( i< n && frame_copy[i] != 1)
			i++; 
	
	}
     
}

int main() {
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
    int len, length;
    gpiod_line_set_value(line, 0);

    // Append preamble
    strcpy(result, "101001");

    // Append user's input
    printf("\n Enter the Message: ");
    scanf("%[^\n]", msg);
    

    int poly[4]={1,0,1,1}; 	
	
	// lenght of string data		
	int k = strlen(msg);
	// lenght of polynomail
	int p = 4;
	// the end frame must have an appended bit less than the polynomail
	int n=k+p-1;
	
	// creating a frame to send data
	int frame[n];
	// creating a copy of frame
	// as it will get modified during calculation
	int frame_copy[n];

	// creating int array from message also appending data
	for(int i=0;i<n;i++){
	
		if(i<k){
		
			frame_copy[i]=frame[i]=msg[i]-'0';
		}
		else{
			frame_copy[i]=frame[i]=0;
		}
	
	
	}
	
	printf("Frame without CRC: ");
	for(int i=0;i<n;i++){
	
			printf("%d",frame_copy[i]);
	
	}	
 	printf("\n");
	
	
	printf("Polynomial : ");
	for(int i=0;i<p;i++){
	
			printf("%d",poly[i]);
	
	}	
	
 	printf("\n");
	int i,j;
 	//Division
    division(frame_copy,poly,n,k,p);
    //CRC
    int crc[15];
    for(i=0,j=k;i<p-1;i++,j++){
        crc[i]=frame_copy[j];
    }
    printf("\n CRC bits: ");
    for(i=0;i<p-1;i++){
        printf("%d",crc[i]);
    }
    printf("\n");
        
	for(int i=0;i<n;i++){
	
		if(i<k){
			frame_copy[i]=frame[i]=msg[i]-'0';
		}
		
	}
       
    printf("\n Final bits: \n");			
	for(int i=0;i<n;i++){
	
		printf("%d",frame_copy[i]);
	
	}		
 	printf("\n");

    char frame_str[n + 1]; // +1 for null terminator
    for (int i = 0; i < n; i++) {
        frame_str[i] = frame_copy[i] + '0'; // Convert integer to character
    }
    frame_str[n] = '\0'; // Null terminate the string

    // Now you can use frame_str with string functions
    length = strlen(frame_str);
    strncat(result, frame_str, length);

    printf("Frame Header (Synchro and Text and CRC ) = %s\n", result);
    length = strlen(result);
    gettimeofday(&tval_before, NULL);
    while(pos != length) {
        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);
        double time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0f);
        while(time_elapsed < 0.001) {
            gettimeofday(&tval_after, NULL);
            timersub(&tval_after, &tval_before, &tval_result);
            time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0f);
        }
        gettimeofday(&tval_before, NULL);
        if (result[pos] == '1') {
            gpiod_line_set_value(line, 1);
            pos++;
        } else if(result[pos] == '0') {
            gpiod_line_set_value(line, 0);
            pos++;
        }
    }

    // Cleanup
    custom_delay(1);
    gpiod_line_set_value(line, 0);
    gpiod_line_release(line);
    gpiod_chip_close(chip);
    return 0;
}