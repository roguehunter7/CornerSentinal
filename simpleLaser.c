#include <stdio.h>
#include <malloc.h>
#include <sys/time.h>
#include <string.h>
#include <wiringPi.h>


char result[3000]={'1','0','1','0','1','0','1','0','1','0','1','1','1','1','1','1','1','1','1','1'};
int counter=20;

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
    //binary[i] = (integer & (int)1<<(n-i-1)) ? '1' : '0';
    //binary[n]='\0';
    result[counter]= (integer & (int)1<<(n-i-1)) ? '1' : '0';
    result[36]='\0';
    counter++;
    }
    
}




int pos=0;

int main()
{
    struct timeval tval_before, tval_after, tval_result;
    
    wiringPiSetup () ;
	pinMode (0, OUTPUT) ;
    
    
    //Read message
    char msg[3000]; //gibt die Länge vor wie viele Zeichen man für die Nachricht benutzen darf
        int len, k, length;
       
        printf("\n Enter the Message: ");
        scanf("%[^'\n']",msg);
        
        len=strlen(msg);
        
        
        int2bin(len*8, 16); //Mal 8, weil ein Byte 8 Bit sind   
        printf ("Frame Header (Synchro and Textlength = %s\n", result);
        
        for(k=0;k<len;k++)
        {
                chartobin(msg[k]);            
        }
    
    
    
    length=strlen(result);
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
            digitalWrite (0, HIGH);
            pos++;
            }
            
        else if(result[pos]=='0'){
            digitalWrite (0,  LOW) ;
            pos++;
            }
    }
    

    return 0;
}
