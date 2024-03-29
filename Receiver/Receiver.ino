const int dataPin = A1;
byte receivedData;
int threshold = 512; // Initial threshold value (adjust as needed)
int sum = 0;
int count = 0;

// Preamble (5-bit pattern)
const byte preamble = 0b10101;

// CRC polynomial (0xB or 1011)
const byte crcPolynomial = 0xB;

void setup() {
  pinMode(dataPin, INPUT);
  Serial.begin(9600);
}

void loop() {
  if (detectPreamble()) {
    receivedData = receive();
    byte crc = receiveCRC();
    if (crc == calculateCRC(receivedData, crcPolynomial)) {
      Serial.println(receivedData, BIN); // Print the received 8-bit binary code
    } else {
      Serial.println("CRC Error");
    }
  }
}

bool detectPreamble() {
  byte preambleData = 0;
  int val;

  for (int i = 0; i < 5; i++) { // Receive the 5-bit preamble
    val = analogRead(dataPin);
    preambleData = (preambleData << 1) | (val > threshold);
    delayMicroseconds(1000); // 1ms delay after each bit sent
  }

  // No need for a longer delay after the preamble

  // Check if the received preamble matches the expected pattern
  return (preambleData == preamble);
}

byte receive() {
  byte data = 0;
  int val;

  for (int i = 0; i < 8; i++) { // Receive 8 bits
    val = analogRead(dataPin);
    sum += val; // Update the sum for dynamic threshold calculation
    count++; // Increment the count for dynamic threshold calculation
    data = (data << 1) | (val > threshold); // Shift the data and set the current bit
    delayMicroseconds(1000); // 1ms delay after each bit sent
  }

  // No need for a longer delay after receiving the byte

  // Update the dynamic threshold value
  threshold = sum / count;
  sum = 0;
  count = 0;

  return data;
}

byte receiveCRC() {
  byte crc = 0;
  int val;

  for (int i = 0; i < 4; i++) { // Receive 4 bits for CRC
    val = analogRead(dataPin);
    crc = (crc << 1) | (val > threshold);
    delayMicroseconds(1000); // 1ms delay after each bit sent
  }

  // No need for a longer delay after receiving the CRC

  return crc;
}

byte calculateCRC(byte data, byte polynomial) {
  byte crc = 0;
  for (int i = 0; i < 8; i++) {
    bool bit = bitRead(data, 7 - i);
    byte topBit = crc & 0x80;
    crc = (crc << 1) | (bit ? 1 : 0);
    if (topBit) {
      crc ^= polynomial;
    }
  }
  return crc;
}
