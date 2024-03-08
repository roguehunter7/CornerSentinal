#include <Arduino.h>
#include <string.h>

// Global variables
static unsigned int StateType;
String bitSequence = "";
int payloadLen = 0;
int HEADER_LEN = 16;
enum StateType { waitingPreamble, receivingData };

void setup() {
    // Setup timer interrupts for sampling of received bits
    cli(); // Clear interrupts

    // Timer Register Inits
    TCCR1A = 0; // Set entire TCCR1A register to 0
    TCCR1B = 0; // Same for TCCR1B
    TCNT1 = 0;  // Initialize counter value to 0

    // Set timer1 interrupt at 1kHz
    // Set timer count for 1kHz increments
    OCR1A = 2001; // = (16*10^6) / (1000*8) - 1 = 2001 for 1kHz

    // Turn on CTC mode
    TCCR1B |= (1 << WGM12);

    // Set CS11 bit for 8 prescaler
    TCCR1B |= (1 << CS11);

    // Enable timer compare interrupt
    TIMSK1 |= (1 << OCIE1A);

    sei(); // Enable interrupts

    Serial.begin(9600);

    // Input Pin for the Solar Cell
    pinMode(A0, INPUT);

    // Set state to unsynchronized (looking for preamble)
    StateType = waitingPreamble;
}

// Timer interrupt handler
ISR(TIMER1_COMPA_vect) {
    String bit;
    int solarCellAnalogValue = analogRead(A0);
    float voltageQuantize = solarCellAnalogValue * (5.0 / 1023.0);

    if (voltageQuantize >= 1) {
        bit = "1";
    } else {
        bit = "0";
    }

    // This is the "real" loop function
    switch (StateType) {
    case waitingPreamble:
        // Look for preamble
        checkForPreamble(bit);
        break;
    case receivingData:
        receiveData(bit);
        break;
    }
}

void loop() {
    // Put your main code here, to run repeatedly
}

void checkForPreamble(String bit) {
    String preamble = "10101010101111111111";
    bitSequence.concat(bit);
    if (bitSequence.length() > preamble.length())
        bitSequence.remove(0, 1);

    if (bitSequence == preamble) {
        Serial.println("Synchronization done");
        StateType = receivingData;
        bitSequence = "";
    }
}

void receiveData(String bit) {
    bitSequence.concat(bit);

    if (bitSequence.length() == HEADER_LEN) {
        char charArray[HEADER_LEN + 1];
        bitSequence.toCharArray(charArray, HEADER_LEN + 1);
        payloadLen = strtol(charArray, NULL, 2);
    }

    if (bitSequence.length() == payloadLen + HEADER_LEN) // Received the entire message
    {
        // Decode the ASCII from binary
        String output = "";
        for (int i = HEADER_LEN / 8; i < (bitSequence.length() / 8); i++) {
            String pl = "";
            for (int l = i * 8; l < 8 * (i + 1); l++)
                pl += bitSequence[l];

            int n = 0;
            for (int j = 0; j < 8; j++) {
                int x = (int)(pl[j]) - (int)'0';
                for (int k = 0; k < 7 - j; k++)
                    x *= 2;
                n += x;
            }
            output += (char)n;
        }

        Serial.println(output);
        bitSequence = "";
        StateType = waitingPreamble;
    }
}
