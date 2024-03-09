#include <util/atomic.h>

#define PREAMBLE 0x15 // Binary: 10101
#define MSG_LEN 8
#define CRC_LEN 3
#define FRAME_LEN (MSG_LEN + CRC_LEN)
#define TOTAL_LEN (FRAME_LEN + 5)

volatile unsigned char frame[FRAME_LEN];
volatile unsigned char preamble_buffer;
volatile unsigned int bit_count = 0;
volatile bool preamble_received = false;
volatile bool frame_received = false;

void calculate_crc(unsigned char *data, int len) {
    unsigned char crc = 0;
    unsigned char polynomial = 0xB; // x^3 + x + 1

    for (int i = 0; i < len; i++) {
        crc ^= (data[i] << (CRC_LEN - 1));
        for (int j = 0; j < CRC_LEN; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc = (crc << 1);
            }
        }
    }

    for (int i = 0; i < CRC_LEN; i++) {
        if (crc & (1 << (CRC_LEN - 1 - i))) {
            frame[len + i] = 1;
        } else {
            frame[len + i] = 0;
        }
    }
}

void setup() {
    Serial.begin(9600);

    // Configure pin A0 as input
    pinMode(A0, INPUT);

    // Timer0 setup for bit sampling
    TCCR0A = 0;
    TCCR0B = 0;
    TCNT0 = 0;
    OCR0A = 99; // 1 millisecond delay at 16 MHz clock
    TCCR0A |= (1 << WGM01); // CTC mode
    TCCR0B |= (1 << CS01); // Prescaler = 8
    TIMSK0 |= (1 << OCIE0A); // Enable Timer0 Compare Match A Interrupt

    sei(); // Enable global interrupts
}

ISR(TIMER0_COMPA_vect) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        int sensorValue = analogRead(A0);
        Serial.println(sensorValue);
        bool bit = sensorValue >= 512; // Threshold at 2.5V

        if (!preamble_received) {
            preamble_buffer = (preamble_buffer << 1) | bit;
            if (bit_count == 4) {
                bit_count = 0;
                if (preamble_buffer == PREAMBLE) {
                    preamble_received = true;
                }
                preamble_buffer = 0;
            } else {
                bit_count++;
            }
        } else if (!frame_received) {
            for (int i = 0; i < FRAME_LEN; i++) {
                frame[i] = (frame[i] << 1) | bit;
            }
            if (++bit_count == TOTAL_LEN) {
                frame_received = true;
                bit_count = 0;
            }
        }
    }
}

void loop() {
    if (frame_received) {
        frame_received = false;
        unsigned char msg[MSG_LEN];
        memcpy(msg, frame, MSG_LEN);

        bool crc_valid = true;
        calculate_crc(msg, MSG_LEN);
        for (int i = 0; i < CRC_LEN; i++) {
            if (frame[MSG_LEN + i] != msg[MSG_LEN + i]) {
                crc_valid = false;
                break;
            }
        }

        if (crc_valid) {
            Serial.print("Received message: 0x");
            for (int i = 0; i < MSG_LEN; i++) {
                Serial.print(msg[i], HEX);
            }
            Serial.println();
        } else {
            Serial.println("CRC error: message corrupted");
        }
    }
}