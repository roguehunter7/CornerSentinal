import time
import gpiod
import sys

result = ['1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1']
counter = 20

def char_to_bin(c):
    global counter
    for i in range(7, -1, -1):
        result[counter] = '1' if (c & (1 << i)) else '0'
        counter += 1

def int_to_bin(integer, n):
    global counter
    for i in range(n):
        result[counter] = '1' if (integer & (1 << (n - i - 1))) else '0'
        counter += 1
    result[36] = '\\0'

pos = 0

def main():
    global pos
    chip = gpiod.chip(4)  # Replace 0 with the appropriate chip number

    line = chip.get_line(4)  # Replace 0 with the appropriate line number
    line.request(consumer="Custom Timing", type=gpiod.LINE_REQ_DIR_OUT)

    msg = input("Enter the Message: ")
    len_msg = len(msg)
    int_to_bin(len_msg * 8, 16)  # Multiply by 8 because one byte is 8 bits
    print(f"Frame Header (Synchro and Textlength = {''.join(result)})")

    for char in msg:
        char_to_bin(ord(char))

    length = len(result)
    tval_before = time.time()

    while pos != length:
        tval_after = time.time()
        time_elapsed = tval_after - tval_before

        while time_elapsed < 0.001:
            tval_after = time.time()
            time_elapsed = tval_after - tval_before

        tval_before = time.time()

        if result[pos] == '1':
            line.set_value(1)
            pos += 1
        elif result[pos] == '0':
            line.set_value(0)
            pos += 1

    line.release()

if __name__ == "__main__":
    main()