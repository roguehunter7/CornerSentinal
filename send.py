from gpiozero import LED
import time

# Define GPIO pin for LED
LED_PIN = 4  # Replace with the actual GPIO pin number connected to the LED
led = LED(LED_PIN)

def transmit_binary_data(binary_data):
    # Start bit
    led.on()
    time.sleep(0.5)

    # Transmit each bit
    for bit in binary_data:
        if bit == '0':
            led.off()
        else:
            led.on()

        time.sleep(0.5)

    # Stop bit
    led.on()
    time.sleep(0.1)

# Binary data to transmit
binary_data_to_transmit = "010010"  # Replace with the 8-bit data you want to transmit

# Transmit data
transmit_binary_data(binary_data_to_transmit)
