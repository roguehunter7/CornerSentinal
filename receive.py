from gpiozero import Button
import time

# Define GPIO pin for LDR
LDR_PIN = 27  # Replace with the actual GPIO pin number connected to the LDR
ldr = Button(LDR_PIN)

def read_binary_data():
    binary_data = ""

    while True:
        valid_msg = False
        capture_msg = False

        # Wait for the start condition
        while ldr.value == 0:
            pass
        # Capture the start condition
        for _ in range(5):
            time.sleep(0.1)
            capture_msg = True if ldr.value == 1 else False

        # Read the 7 bits after the start condition
        if capture_msg:
            for _ in range(7):
                time.sleep(0.25)
                binary_data += '1' if ldr.value == 1 else '0'

        print('binary data:', binary_data)

        # Check for the stop condition
        if capture_msg:
            for _ in range(5):
                time.sleep(0.1)
                valid_msg = True if ldr.value == 1 else False

        # Process and display the message bits
        if valid_msg:
            process_binary_data(binary_data)

        binary_data = ""  # Reset binary_data after processing
        time.sleep(0.1)  # Short delay to avoid rapid processing of overlapping messages

def process_binary_data(message_bits):
    if len(message_bits) == 7:  # Ensure exactly 7 bits are available
        is_stationary = message_bits[0] == '1'
        vehicle_type_bits = message_bits[1:4]
        is_wrong_side = message_bits[4] == '1'
        speed_bits = message_bits[5:7]

        display_warning_message(is_stationary, vehicle_type_bits, is_wrong_side, speed_bits)
    else:
        print("Error: Incorrect number of bits for decoding")

def display_warning_message(is_stationary, vehicle_type_bits, is_wrong_side, speed_bits):
    if is_stationary:
        if is_wrong_side:
            print("Warning: Vehicle is broken down on the wrong side")
        else:
            print("Warning: Vehicle is stationary")

    vehicle_type = get_vehicle_type(vehicle_type_bits)
    print(f"Vehicle Type: {vehicle_type}")

    if is_wrong_side and not is_stationary:
        print("Warning: Vehicle is on the wrong side")

    speed_category = get_speed_category(speed_bits)
    print(f"Speed Category: {speed_category}")

    print("-" * 40)  # Add a line between warning messages

def get_vehicle_type(vehicle_type_bits):
    vehicle_types = {'001': 'Motorcycle', '010': 'Car', '011': 'Bus/Truck', '100': 'Emergency Vehicle'}
    return vehicle_types.get(vehicle_type_bits, 'Unknown')

def get_speed_category(speed_bits):
    speed_categories = {'11': 'Overspeed Vehicle', '10': 'High Speed Vehicle', '01': 'Normal Speed Vehicle', '00': 'No Speed'}
    return speed_categories.get(speed_bits, 'Unknown Speed')

# Start listening for binary data
read_binary_data()