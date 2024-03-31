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

transmit_msg = lib.transmit_message
transmit_msg.argtypes = [ctypes.c_char_p]
transmit_msg.restype = None

def transmit_message(message):
    transmit_msg(message.encode('utf-8'))
    