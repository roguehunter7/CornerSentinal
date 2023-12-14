
from gpiozero import Button
import time

# Define GPIO pin for LDR
LDR_PIN = 27  # Replace with the actual GPIO pin number connected to the LDR
ldr = Button(LDR_PIN)

def read_binary_data():
    while True:
        # Wait for the start bit
        while ldr.value==0:  # Adjust this threshold based on your LDR characteristics
            time.sleep(0.5)

        # Read the 6-bit binary data (excluding start bit)
        binary_data = ""
        for _ in range(7):
            bit_value = 1 if ldr.value else 0  # Adjust this threshold
            binary_data += '1' if bit_value == 1 else '0'
            time.sleep(0.5)

        # Print received binary data
        print("Received Binary Data:", binary_data)

        # Decode and process the message
        decode_binary_data(binary_data)

def decode_binary_data(binary_data):
    # Check if the input is a valid 6-bit binary data
    if len(binary_data) == 7 and all(bit in ('0', '1') for bit in binary_data):
        ambulance = "Yes" if binary_data[1] == '1' else "No"
        vehicle_type_bits = binary_data[2:4]
        wrong_lane_bits = binary_data[4]
        speed_bits = binary_data[5:]

        # Display message for ambulance
        if ambulance == "Yes":
            print("Ambulance approaching")
            return

        # Display message for HMV
        if vehicle_type_bits == '00':
            print("Heavy Motor Vehicle (HMV) approaching")
            return

        # Check speed for LMV or Two Wheeler
        if speed_bits == '10':
            print("Vehicle approaching in high speed")
            return

        # Check bits for LMV or Two Wheeler and wrong lane condition
        elif vehicle_type_bits == '01':
            vehicle_type = "LMV"
            wrong_lane = "Yes" if wrong_lane_bits == '1' else "No"
            
            # Display message for wrong lane
            if wrong_lane == "Yes":
                print("Vehicle approaching in wrong lane")

        elif vehicle_type_bits == '10':
            vehicle_type = "Two Wheeler"
            wrong_lane = "Yes" if wrong_lane_bits == '1' else "No"
            
            # Display message for wrong lane
            if wrong_lane == "Yes":
                print("Vehicle approaching in wrong lane")

# Start listening for binary data
read_binary_data()
