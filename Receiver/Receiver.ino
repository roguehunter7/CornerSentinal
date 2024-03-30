//global variables
static unsigned int state;
String sequence = "00000";
String dataBits = "";
boolean synchro_Done = false;
boolean receiveData_Done = false;

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
}

ISR(TIMER1_COMPA_vect) {
  String data = "0";
  int sensorValue = analogRead(A0);
  float voltage = sensorValue * (5.0 / 1023.0);
  if (voltage >= 0.5) {
    data = "1";
  } else {
    data = "0";
  }

  //This is the "real" loop function
  switch (state) {
    case 0: //looking for synchronization sequence
      synchro_Done = false;
      lookForSynchro(data);
      if (synchro_Done == true) {
        state = 1;
        sequence = "00000"; // Reset sequence for next synchronization
      }
      break;
    case 1: //receive Data
      receiveData_Done = false;
      receiveData(data);
      if (receiveData_Done == true) {
        Serial.println(dataBits);
        dataBits = "";
        state = 0; // Reset state to look for next synchronization sequence
      }
      break;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
}

void lookForSynchro(String bit) {
  String preambel = "10101";
  sequence.concat(bit);
  sequence.remove(0, 1);
  if (sequence == preambel) {
    Serial.println("Synchro done");
    synchro_Done = true;
    sequence = "00000";
  }
}

void receiveData(String bit) {
  dataBits.concat(bit);
  if (dataBits.length() == 8) {
    receiveData_Done = true;
  }
}