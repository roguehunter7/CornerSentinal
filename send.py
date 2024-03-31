import ctypes

# Load the shared library
lib = ctypes.CDLL('./module.so')

# Define the function signature
custom_delay = lib.custom_delay
custom_delay.argtypes = [ctypes.c_double]
custom_delay.restype = None

division = lib.division
division.argtypes = [ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int), ctypes.c_int, ctypes.c_int, ctypes.c_int]
division.restype = None

transmit_message = lib.transmit_message
transmit_message.argtypes = [ctypes.c_char_p]
transmit_message.restype = None

if __name__ == "__main__":
    # Example usage
    message = input("Enter the message: ")
    transmit_message(message.encode('utf-8'))