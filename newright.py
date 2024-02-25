import socket
from collections import defaultdict
import cv2
import numpy as np
from time import time, sleep
from threading import Thread
from ultralytics import YOLO
from send import transmit_binary_data

# Function to calculate Euclidean distance
def calculate_distance(point1, point2):
    return np.sqrt((point2[0] - point1[0]) ** 2 + (point2[1] - point1[1]) ** 2)

# Function to calculate speed using Euclidean distance
def calculate_speed(distances, factor_km, latency_fps):
    if len(distances) <= 1:
        return 0.0

    average_speed = (np.mean(distances) * factor_km) / latency_fps
    return average_speed

# Function to generate 9-bit binary code based on conditions
def generate_binary_code(class_id, speed, is_stationary, is_wrong_side):
    binary_code = ['0'] * 9
    binary_code[0] = '1'  # start bit

    # Stationary bit
    binary_code[1] = '1' if is_stationary else '0'

    if class_id == 0:  # Ambulance
        binary_code[2:5] = '100'
    elif class_id in [2, 6, 4]:  # Car or Van or Taxi/Auto
        binary_code[2:5] = '010'
    elif class_id in [5, 1]:  # Bus or Truck
        binary_code[2:5] = '011'
    elif class_id == 3:  # Motorcycle
        binary_code[2:5] = '001'

    # Wrong side warning bit
    binary_code[5] = '1' if is_wrong_side else '0'

    # Replace speed section
    if speed > 60:  # Overspeed Vehicle
        binary_code[6:8] = '11'
    elif 40 <= speed < 60:
        binary_code[6:8] = '10'
    elif 1.5 <= speed < 40:
        binary_code[6:8] = '01'

    binary_code[8] = '1'  # stop bit
    return ''.join(binary_code)

# Function to display warning message on the frame
def display_warning_message(frame, binary_code):
    warning_message = f"Warning: {binary_code}"
    cv2.putText(frame, warning_message, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 0, 0), 2)

# Function to handle LiFi transmission in a separate thread
def lifi_transmission_thread():
    while True:
        if recv_binary_code_queue.qsize() > 0:
            received_binary_code = recv_binary_code_queue.get()
            transmit_binary_data(received_binary_code)
        sleep(0.1)  # Adjust sleep time based on your requirements

# Function to transmit binary data
def transmit_binary_data(binary_code):
    # Placeholder implementation
    print(f"Transmitting Binary Code: {binary_code}")

# Function to receive binary data
def receive_binary_data(client_socket):
    return client_socket.recv(1024).decode()

# Function to send binary data
def send_binary_data(client_socket, binary_code):
    client_socket.sendall(binary_code.encode())

# Load the YOLOv8 model
model = YOLO('train3/weights/best.onnx')

# Open the video file
video_path = "test_images/rightside.mp4"
cap = cv2.VideoCapture(video_path)

# Constants for speed calculation
VIDEO_FPS = 30  # Assuming the video is recorded at 30 fps
FACTOR_KM = 3.6
LATENCY_FPS = VIDEO_FPS / 2

# Store track history for each vehicle
track_history = defaultdict(list)
stationary_timers = defaultdict(float)

# Counter to keep track of frames
frame_counter = 0

# Placeholder for the previous frame and points for optical flow
prev_frame = None
prev_pts = None
prev_binary_code = None

# Server socket initialization on RPi1
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_address = ('192.168.1.2', 8888)  # IP of RPi1
server_socket.bind(server_address)
server_socket.listen()

connection, client_address = server_socket.accept()

# Client socket initialization for connecting to RPi2
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client_address = ('192.168.1.1', 8888)  # IP of RPi2

# Create a queue for communication between the main thread and LiFi transmission thread
recv_binary_code_queue = queue.Queue()

# Start the LiFi transmission thread
lifi_transmission_thread = Thread(target=lifi_transmission_thread)
lifi_transmission_thread.daemon = True
lifi_transmission_thread.start()

# Connected flag for client socket
connected = False
while not connected:
    try:
        client_socket.connect(client_address)
        print("Connected to RPi2")
        connected = True
    except ConnectionRefusedError:
        print("Connection to RPi2 refused. Retrying...")
        time.sleep(1)

while cap.isOpened():
    success, frame = cap.read()
    
    if success:
        frame_gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        
        # Check if YOLO inference should be performed on this frame
        if frame_counter % 2 == 0:
            results = model.track(frame, persist=True, tracker='botsort.yaml', imgsz=(320, 320), int8=True, conf=0.15)
            annotated_frame = results[0].plot()

            if results[0].boxes.id is not None:
                boxes = results[0].boxes.xywh.cpu().numpy().astype(int)
                class_id = results[0].boxes.cls.cpu().numpy().astype(int)
                track_ids = results[0].boxes.id.cpu().numpy().astype(int)

                for i, box in enumerate(boxes):
                    x, y, w, h = box
                    track = track_history[track_ids[i]]
                    track.append((float(x + w / 2), float(y + h / 2)))

                    if len(track) >= 2 and track[-2][1] < track[-1][1]:
                        distances = [calculate_distance(track[j], track[j + 1]) for j in range(len(track) - 1)]

                        speed = calculate_speed(distances, FACTOR_KM, LATENCY_FPS)
                        is_stationary = speed < 1.0
                        stationary_timers[track_ids[i]] = time.time() if not is_stationary else stationary_timers[
                            track_ids[i]]

                        if time.time() - stationary_timers[track_ids[i]] > 10.0:
                            is_stationary = True

                        is_wrong_side = False
                        binary_code = generate_binary_code(class_id[i], speed, is_stationary, is_wrong_side)

                        # Transmit only if the binary code is different from the previous one
                        if binary_code != prev_binary_code:
                            send_binary_data(client_socket, binary_code)
                            prev_binary_code = binary_code
                            
                        # Receive binary_code from the other Raspberry Pi
                        recv_binary_code = receive_binary_data(client_socket)
                        recv_binary_code_queue.put(recv_binary_code)
                        
                        display_warning_message(annotated_frame, binary_code)
                        cv2.putText(annotated_frame, f"Speed: {speed:.2f} km/h", (int(x), int(y) - 10),
                                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 0, 0), 2)
                        roi = frame_gray[int(y):int(y + h), int(x):int(x + w)]
 
                        if prev_frame is not None and prev_pts is not None:
                            prev_frame_resized = cv2.resize(prev_frame, (roi.shape[1], roi.shape[0]))
                            flow = cv2.calcOpticalFlowPyrLK(prev_frame_resized, roi, prev_pts, None,
                                                           winSize=(15, 15), maxLevel=2)
                            flow_distances = np.sqrt(np.sum((prev_pts - flow[0]) ** 2, axis=2))

                            for j in range(len(flow_distances)):
                                if flow_distances[j] > 0.5:
                                    x1, y1 = prev_pts[j].astype(int).ravel()
                                    x2, y2 = (x + flow[0][j][0], y + flow[0][j][1]).astype(int)
                                    cv2.line(annotated_frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
                            prev_frame = roi
                            prev_pts = np.array([[(int(w / 2), int(h / 2))]], dtype=np.float32)

        # Increment frame counter
        frame_counter += 1
        cv2.imshow("Frame", annotated_frame)

        if cv2.waitKey(1) & 0xFF == ord("q"):
            break
    else:
        break

# Release resources
cap.release()
# Closing the server socket
server_socket.close()

# Closing the client socket
client_socket.close()
cv2.destroyAllWindows()
