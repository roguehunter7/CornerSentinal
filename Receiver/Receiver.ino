#include <LiquidCrystal.h>
// Setup the LiquidCrystal library with the pin numbers we have
// physically connected the module to.
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

//global variables
static unsigned int state;
String sequence = "000000";
String dataBits = "";
boolean synchro_Done = false;
boolean receiveData_Done = false;
boolean crc_check_value=false;

unsigned long sensorValueSum = 0; // Sum of sensor values in the window
const int windowSize = 30; // Window size for dynamic threshold calculation
int sensorValueCount = 0; // Sensor value count

float threshold = 0.5; // Initial value for Threshold
void setup() {
  //Timer Interrupt settings:
  // TIMER SETUP- the timer interrupt allows precise timed measurements of the reed switch
  cli(); //stop interrupts
  //set timer1 interrupt at 1kHz
  TCCR1A = 0; // set entire TCCR1A register to 0
  TCCR1B = 0; // same for TCCR1B
  TCNT1 = 0; //initialize counter value to 0
  // set timer count for 1kHz increments
  OCR1A = 2001; // = (16*10^6) / (1000*8) - 1 (OCR1A = 2001 for 1kHz)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS11 bit for 8 prescaler
  TCCR1B |= (1 << CS11);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei(); //allow interrupts
  //END TIMER SETUP

  Serial.begin(9600);
  //Input Pin for the Solarplate
  pinMode(A0, INPUT);
  //initial State is looking for Synchronization sequence
  state = 0;
  //setup lcd
  lcd.begin(16, 2);
}

ISR(TIMER1_COMPA_vect) {
  String data = "0";
  int sensorValue = analogRead(A0);
  float voltage = sensorValue * (5.0 / 1023.0);
  // Serial.println(voltage);
  // Update the running sum and average
  if (sensorValueCount < windowSize) {
    // Fill the initial window
    sensorValueSum += sensorValue;
    sensorValueCount++;
  } else {
    // Update the threshold with the running average
    threshold = (sensorValueSum  / windowSize) * (5.0 / 1023.0);
    sensorValueSum = 0 ;
    sensorValueCount = 0;
  }

  // Serial.println(threshold);
  
  if (voltage > threshold ) {
    data = "1";
  } else {
    data = "0";
  }
  // Serial.println(data);

  //This is the "real" loop function
  switch (state) {
    case 0: //looking for synchronization sequence
      synchro_Done = false;
      lookForSynchro(data);
      if (synchro_Done == true) {
        state = 1;
        sequence = "000000"; // Reset sequence for next synchronization
      }
      break;
    case 1: //receive Data
      receiveData_Done = false;
      receiveData(data);
      if (receiveData_Done == true) 
      {
        state = 0; // Reset state to look for next synchronization sequence
      }
      break;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
}

void lookForSynchro(String bit) {
  String preambel = "101010";
  sequence.concat(bit);
  sequence.remove(0, 1);
  if (sequence == preambel) {
    synchro_Done = true;
    sequence = "000000";
  }
}

void receiveData(String bit) {
  dataBits.concat(bit);
  if (dataBits.length() == 11) 
  { // Serial.println(dataBits);
    if (crc_check_value==false) //do the CRC check
    {
      checkCRC(dataBits);  
    }

    if (crc_check_value==true) //only show message when not corrupted
    {
      char datamessage[9];
      for(int i=0;i<8;i++){
        datamessage[i]=+dataBits[i];
      }
      decodeBinaryCode(datamessage);
      dataBits = "";
      receiveData_Done=true; 
      crc_check_value=false;
    }
  }
}

void checkCRC(String dataFrame)
{
  int polynom[4]={1,0,1,1};
  int k=dataFrame.length();  
  int p=4; //int p=strlen(polynom); // lenght of polynom (normaly fix, but it is better to use a variable if I want to change the polynom later
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
    crc_check_value=false;  
    dataBits="";
    receiveData_Done=true;
  }
   
}
void decodeBinaryCode(String binary_code) {
  bool is_stationary = binary_code[0] == '1';
  String vehicle_type;
  String speed_range;
  bool is_wrong_side = binary_code[5] == '1';
  if (is_stationary == true)
  {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Stationary");
  lcd.setCursor(0, 1);
  lcd.print("Vehicle Ahead");
  }

  bool is_accident = binary_code[1] == '1';
  if (is_accident == true)
  {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Accident/Crash");
  lcd.setCursor(0, 1);
  lcd.print("Detected Ahead");
  }
  String vehicle_id = binary_code.substring(2, 5);
  if (vehicle_id == "100") {
    vehicle_type = "Ambulance";
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Emergency");
    lcd.setCursor(0, 1);
    lcd.print("Vehicle");
  } else if (vehicle_id == "010") {
    vehicle_type = "Car or Van or Taxi/Auto";
  } else if (vehicle_id == "011") {
    vehicle_type = "Bus or Truck";
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Bus/Truck");
    lcd.setCursor(0, 1);
    lcd.print("Incoming");
  } else if (vehicle_id == "001") {
    vehicle_type = "Motorcycle";
  } else {
    vehicle_type = "Unknown";
  }
  if (is_wrong_side == true)
  {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Vehicle Coming");
  lcd.setCursor(0, 1);
  lcd.print("On Wrong Side");
  }
  String speed_id = binary_code.substring(6, 8);
  if (speed_id == "11") {
    speed_range = "Overspeed Vehicle";
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Overspeed");
    lcd.setCursor(0, 1);
    lcd.print("Vehicle");
  } else if (speed_id == "10") {
    speed_range = "40-60";
  } else if (speed_id == "01") {
    speed_range = "1.5-40";
  } else {
    speed_range = "Unknown";
  }
  Serial.print("Vehicle: ");
  Serial.println(vehicle_type);
  Serial.print("Stationary: ");
  Serial.println(is_stationary ? "Yes" : "No");
  Serial.print("Accident: ");
  Serial.println(is_accident ? "Yes" : "No");
  Serial.print("Wrong side: ");
  Serial.println(is_wrong_side ? "Yes" : "No");
  Serial.print("Speed range: ");
  Serial.println(speed_range);
  Serial.println();
}
