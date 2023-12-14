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
    for bit in binary_data[1:8]:
        if bit == '0':
            led.off()
        else:
            led.on()

        time.sleep(0.2)

    # Stop bit
    led.on()
    time.sleep(0.5)
