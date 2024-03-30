#include <stdio.h>
#include <string.h>



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

int main(){
	
	// store data bits in char form	
	char datas[256];
	// store polynomial in char form
	char polys[256];
	
	/// Scan until enter is pressed 
	printf("enter a number\n>>");
	scanf("\n%[^\n]", datas); 
	
	// Scan until enter is pressed
	printf("enter a polyno\n>>");
	scanf("\n%[^\n]", polys); 	
	
	// lenght of string data		
	int k=strlen(datas);
	// lenght of polynomail
	int p=strlen(polys);
	// the end frame must have an appended bit less than the polynomail
	int n=k+p-1;
	
	// creating a frame to send data
	int frame[n];
	// creating a copy of frame
	// as it will get modified during calculation
	int frame_copy[n];
	// creating a copy of the polynomail
	int poly[p];
	for(int i=0;i<p;i++){
		poly[i]=polys[i]-'0';
	
	
	}
	// creating int array from message also appending data
	for(int i=0;i<n;i++){
	
		if(i<k){
		
			frame_copy[i]=frame[i]=datas[i]-'0';
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
			frame_copy[i]=frame[i]=datas[i]-'0';
		}
		
	}
       
    printf("\n Final bits: \n");			
	for(int i=0;i<n;i++){
	
		printf("%d",frame_copy[i]);
	
	}		
 	printf("\n");
 	
	return 0;
	
}



