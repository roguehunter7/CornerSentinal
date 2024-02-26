from gpiozero import LED
import time

# Define GPIO pin for LED
LED_PIN = 4  # Replace with the actual GPIO pin number connected to the LED
led = LED(LED_PIN)

def transmit_binary_data(binary_data):
    # Start bit
    led.on()
    time.sleep(0.1)

    # Transmit each bit
    print('Binary Data to be transmitted:',binary_data[1:8])
    for bit in binary_data[1:8]:
        if bit == '0':
            print(bit)
            led.off()
        else:
            led.on()
            print(bit)
        time.sleep(0.05)

    # Stop bit
    led.on()
    time.sleep(0.1)
    led.off()
