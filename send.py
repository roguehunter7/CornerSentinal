import ctypes

# Load the shared library
transmitter = ctypes.CDLL('./module.so')

# Define the function signature
transmitter.transmit_message.argtypes = [ctypes.c_char_p]

def transmit_message(msg):
    # Convert Python string to C string
    c_msg = ctypes.c_char_p(msg.encode('utf-8'))
    
    # Call the C function
    transmitter.transmit_message(c_msg)

if __name__ == "__main__":
    # Example usage
    message = input("Enter the message: ")
    transmit_message(message)