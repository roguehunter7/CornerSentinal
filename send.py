import sys
import time
import gpiod

result = ['1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1']
counter = 20
pos = 0

def chartobin(c):
    global counter
    binary = []
    for i in range(7, -1, -1):
        binary.append('1' if (c & (1 << i)) else '0')
        counter += 1
    return binary

def int2bin(integer, n):
    binary = []
    for i in range(n):
        binary.append('1' if (integer & (1 << (n - i - 1))) else '0')
        counter += 1
    return binary

def main():
    chip = gpiod.Chip("/dev/gpiochip4")
    lines = chip.get_lines(4) # Example pin number, change as needed
    lines.request(consumer="example-gpiod", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[0])

    msg = input("\nEnter the Message: ")

    len_msg = len(msg)
    int2bin(len_msg * 8, 16)
    print(f"Frame Header (Synchro and Textlength = {''.join(result)}")

    for char in msg:
        result.extend(chartobin(ord(char)))

    length = len(result)

    while pos != length:
        start_time = time.time()

        while time.time() - start_time < 0.001:
            pass

        if result[pos] == '1':
            lines.set_value(1)
            pos += 1
        elif result[pos] == '0':
            lines.set_value(0)
            pos += 1

    line.release()
    chip.close()

if __name__ == "__main__":
    sys.exit(main())
