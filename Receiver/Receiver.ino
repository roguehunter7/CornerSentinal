//global variables
static unsigned int state;
String sequence="0000000000000000";
String dataBits="";
boolean synchro_Done =false;
boolean receiveData_Done =false;
int decimalValue=800; //just for initialisiation (gets later dynamically calculated)
boolean crc_check_value=false; //Flag for CRC calculation


void setup() {
  //Timer Interrupt settings:
  // TIMER SETUP- the timer interrupt allows preceise timed measurements of the reed switch
  //for mor info about configuration of arduino timers see https://nerd-corner.com/arduino-timer-interrupts-how-to-program-arduino-registers/
  cli();//stop interrupts

  //set timer1 interrupt at 1kHz ; 
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0;
  // set timer count for 1khz increments
  OCR1A = 2001;// = (16*10^6) / (1000*8) - 1        OCR1A = 2001 for 1kHz;
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS11 bit for 8 prescaler
  TCCR1B |= (1 << CS11);   
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  
  sei();//allow interrupts
  //END TIMER SETUP
  
  Serial.begin(9600);

  //Input Pin for the Solarplate
  pinMode(A0,INPUT);

  //initial State is looking for Synchronization sequence
  state = 0;
  
}


ISR(TIMER1_COMPA_vect) 
{
  String data="0";
  int sensorValue = analogRead(A0);
  float voltage = sensorValue * (5.0 / 1023.0);

  //Serial.println(sensorValue);

  if (sensorValue<30) 
  {
    data="1";
    //Serial.println("1");
  }
  else
  {
    data="0";
    //Serial.println("0");
  }



  
  //This is the "real" loop function
  switch (state)
  {
    case 0:
      //looking for synchronization sequence
      synchro_Done=false;
      lookForSynchro(data);

      if (synchro_Done== true)
      {
        state=1;
      }
      break;
    case 1:
      //receive Data
      receiveData_Done =false;
      receiveData(data);

      if (receiveData_Done==true)
      {
        state=0; 
      }
      break;
  }

  
}


void loop() {
  // put your main code here, to run repeatedly:

}


void lookForSynchro(String bit)
{
  String preambel="1010101111111111";
  sequence.concat(bit);
  sequence.remove(0,1);
  //Serial.println("Sequence: "+sequence);
  if (sequence==preambel)
  {
    Serial.println("Synchro done");
    synchro_Done=true;  
    sequence="0000000000000000";
  }
}

void receiveData(String bit)
{
  dataBits.concat(bit);

  if (dataBits.length()==16)
  {
    Serial.println("data Bits: "+dataBits);
    char char_array[17];  // Prepare the character array (the buffer)
    dataBits.toCharArray(char_array, 17);
    decimalValue= strtol(char_array, NULL, 2);//function for converting string into long data type integer
    Serial.println(decimalValue);

  }

  if(dataBits.length()==decimalValue+16+8) //Stop dynamically at the end of the message; +16 because 2 bytes frame for data message length; +8 for last byte CRC
  {

    if (crc_check_value==false) //do the CRC check
    {
      checkCRC(dataBits);  
    }

    if (crc_check_value==true) //only show message when not corrupted
    {
      //Converting the Bits to Text here
      String output = "";
      for(int i = 2; i < (dataBits.length()/8)-1; i++) //first 2 bytes are for the data length and have no msg data; -1 because last Byte is for CRC and has no message data
      {
          String pl = "";
          for(int l = i*8; l < 8*(i+1); l++){ 
              pl += dataBits[l];
          }    
          
          int n = 0;
          for(int j = 0; j < 8; j++)
          {
              int x = (int)(pl[j])-(int)'0';
              for(int k = 0; k < 7-j; k++)  x *= 2;
              n += x;
          }
          output += (char)n;
      }
      Serial.println(output);
    
      dataBits="";
      receiveData_Done=true; 
      crc_check_value=false;
    }
    
  }
}
void checkCRC(String dataFrame)
{
  int polynom[9]={1,0,0,1,0,1,1,1,1};
  dataFrame="10101010101111111111"+dataFrame;
  int k=dataFrame.length();  
  int p=9; //int p=strlen(polynom); // lenght of polynom (normaly fix, but it is better to use a variable if I want to change the polynom later
  int n=k+p-1; //add some zeros to the end of the data for the polynom division
  int frame[n]; //buffer frame with perfect size for CRC 
    
  //convert char array to int array
  for(int i=0;i<n;i++){
    if(i<k){
      frame[i]=dataFrame[i]-'0'; //converts an char number to corresponding int number
    }
    else{
      frame[i]=0;
    } 
  }
   
  //make the division
  int i=0;
  while (  i <  k  ){                     
    for( int j=0 ; j < p ; j++){
            if( frame[i+j] == polynom[j] )  {
                frame[i+j]=0;
            }
            else{
                frame[i+j]=1;
            }     
    }
    while( i< n && frame[i] != 1)
      i++; 
  }
  bool CRC_Done_false=false;  
  for(int j=k; j-k<p-1;j++)
  {
    //erst am Ende des Frames die CRC Sequenz deswegen j=k
    if (frame[j]==1){
      CRC_Done_false=true;
    }     
  }  

  if(CRC_Done_false==false)
  {
    Serial.println("Message has no errors!");
    Serial.println();
    Serial.print("Message: ");
    crc_check_value=true;  
  }

  if(CRC_Done_false==true)
  {
    Serial.println("Message had an error and was dropped!");
    crc_check_value=false;  
    dataBits="";
    receiveData_Done=true;
  }
   
}
