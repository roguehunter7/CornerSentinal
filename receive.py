from gpiozero import Button
import time

# Define GPIO pin for LDR
LDR_PIN = 27  # Replace with the actual GPIO pin number connected to the LDR
ldr = Button(LDR_PIN)

def read_binary_data():
    while True:
        # Wait for the start bit (long bit)
        while not is_long_bit(0.5):
            pass

        # Read the 7-bit binary data
        binary_data = ""
        for _ in range(7):
            time.sleep(0.2)
            bit_value = 1 if ldr.value else 0
            binary_data += str(bit_value)

        # Wait for the stop bit (long bit)
        while not is_long_bit(0.5):
            pass

        # Print received binary data
        print("Received Binary Data:", binary_data)

        # Decode and process the message
        decode_binary_data(binary_data)

def is_long_bit(duration):
    # Helper function to check if the bit duration is long
    start_time = time.time()
    while ldr.value == 1:
        if time.time() - start_time >= duration:
            return True
    return False

def decode_binary_data(binary_data):
    is_stationary = binary_data[0] == '1'
    vehicle_type_bits = binary_data[1:4]
    is_wrong_side = binary_data[4] == '1'
    speed_bits = binary_data[5:7]

    # Decode and display the appropriate warning message
    display_warning_message(is_stationary, vehicle_type_bits, is_wrong_side, speed_bits)

def display_warning_message(is_stationary, vehicle_type_bits, is_wrong_side, speed_bits):
    # Display warning message based on decoded binary data
    if is_stationary:
        print("Warning: Vehicle is stationary")

    vehicle_type = get_vehicle_type(vehicle_type_bits)
    print(f"Vehicle Type: {vehicle_type}")

    if is_wrong_side:
        print("Warning: Vehicle is on the wrong side")

    speed_category = get_speed_category(speed_bits)
    print(f"Speed Category: {speed_category}")

def get_vehicle_type(vehicle_type_bits):
    # Decode vehicle type based on binary bits
    if vehicle_type_bits == '001':
        return "Motorcycle"
    elif vehicle_type_bits == '010':
        return "Car"
    elif vehicle_type_bits == '011':
        return "Bus/Truck"
    else:
        return "Unknown"

def get_speed_category(speed_bits):
    # Decode speed category based on binary bits
    if speed_bits == '11':
        return "Overspeed Vehicle"
    elif speed_bits == '10':
        return "High Speed Vehicle"
    elif speed_bits == '01':
        return "Normal Speed Vehicle"
    else:
        return "Unknown Speed"

# Start listening for binary data
read_binary_data()