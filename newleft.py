from collections import defaultdict
import cv2
import time
import socket
from ultralytics import YOLO
from lanedetector import *
import numpy as np
from send import *
import threading
from queue import Queue

# Load the YOLOv8 model
model = YOLO('train3/weights/best.pt')

# Open the video file
video_path = "test_images/output.mp4"
cap = cv2.VideoCapture(video_path)

# Constants for speed calculation
VIDEO_FPS = int(cap.get(cv2.CAP_PROP_FPS))
FACTOR_KM = 3.6
LATENCY_FPS = VIDEO_FPS / 2

# Initialize lane detector
ld = LineDrawerGUI(video_path)
line_coords = ld.line_coords
option_val = ld.option_val
ld2 = LaneDetector(video_path, line_coords)
ld2.calculate_all_points()
points = ld2.points

# Server socket initialization for receiving on RPi1
s_receive = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
receive_address = ('', 12345)  # IP of RPi1
s_receive.bind(receive_address)
s_receive.listen(1)

# Server socket initialization for sending on RPi1
s_send = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
send_address = ('', 12346)  # Choose a different port for sending
s_send.bind(send_address)
s_send.listen(1)

# Connect to the other RPi for receiving
s_client_send, _ = s_receive.accept()
print("Sender socket connected")

# Connect to the other RPi for sending
s_client_receive, _ = s_send.accept()
print("Receiver socket connected")

# Function to calculate Euclidean distance
def calculate_distance(p1, p2):
    return np.sqrt((p2[0] - p1[0]) ** 2 + (p2[1] - p1[1]) ** 2)

# Function to calculate speed using Euclidean distance
def calculate_speed(distances, factor_km, latency_fps):
    if len(distances) <= 1:
        return 0.0
    average_speed = (np.mean(distances) * factor_km) / latency_fps * 10
    return average_speed

# Function to generate 9-bit binary code based on conditions
def generate_binary_code(class_id, speed, is_stationary, is_wrong_side):
    binary_code = ['0'] * 8

    # Stationary bit
    binary_code[0] = '1' if is_stationary else '0'

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

    return ''.join(binary_code)

# Function to display warning message on the frame
def display_warning_message(frame, binary_code):
    warning_message = f"Warning: {binary_code}"
    cv2.putText(frame, warning_message, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 0, 0), 2)

# Store track history for each vehicle
track_history = defaultdict(list)
stationary_timers = defaultdict(float)

# Counter to keep track of frames
frame_counter = 0

# Placeholder for the previous frame and points for optical flow
prev_frame = None
prev_pts = None

# Placeholder for the previous binary code
prev_binary_code = None
recv_binary_code = None
prev_track_id = None

# Function to handle receiving data from the client socket
def receive_data_from_client(s_client_receive):
    while True:
        try:
            recv_binary_code = s_client_receive.recv(1024).decode()
            transmit_message(recv_binary_code)
        except KeyboardInterrupt:
            print("Keyboard interrupt detected.")
            break

# Create and start a thread for receiving data
receive_thread = threading.Thread(target=receive_data_from_client, args=(s_client_receive,))
receive_thread.daemon = True  # Set as a daemon thread so it terminates when the main program exits
receive_thread.start()

while cap.isOpened():
    ret = cap.grab()
    if ret:
        success, frame = cap.retrieve()

        # Check if YOLO inference should be performed on this frame
        if frame_counter % 2 == 0:
            results = model.track(frame, persist=True, tracker='botsort.yaml', imgsz=640)
            annotated_frame = results[0].plot()

            if results[0].boxes.id is not None:
                boxes = results[0].boxes.xywh.cpu().numpy().astype(int)
                class_id = results[0].boxes.cls.cpu().numpy().astype(int)
                track_ids = results[0].boxes.id.cpu().numpy().astype(int)

                for i, box in enumerate(boxes):
                    x, y, w, h = box
                    xmin, ymin, xmax, ymax = x, y, x + w, y + h
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
                        vehicle_pos = calculate_centroid(xmin, ymin, xmax, ymax)
                        correct_lane = lane_detector(points, vehicle_pos, int(option_val))
                        is_wrong_side = correct_lane != 1.0 and correct_lane != 0

                        binary_code = generate_binary_code(class_id[i], speed, is_stationary, is_wrong_side)

                        if track_ids[i] != prev_track_id or binary_code != prev_binary_code:
                            s_client_send.sendall(binary_code.encode())
                            prev_track_id = track_ids[i]
                            prev_binary_code = binary_code

                        display_warning_message(annotated_frame, binary_code)
                        cv2.putText(annotated_frame, f"Speed: {speed:.2f} km/h", (int(x), int(y) - 10),
                                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 0, 0), 2)
                        roi = frame[int(y):int(y + h), int(x):int(x + w)]

                        if prev_frame is not None and prev_pts is not None:
                            prev_frame_resized = cv2.resize(prev_frame, (roi.shape[1], roi.shape[0]))
                            flow = cv2.calcOpticalFlowPyrLK(prev_frame_resized, roi, prev_pts, None,
                                                             winSize=(15, 15), maxLevel=2)
                            flow_distances = np.sqrt(np.sum((prev_pts - flow[0]) ** 2, axis=2))

                            good_pts = flow_distances > 0.5
                            for j, is_good in enumerate(good_pts):
                                if is_good:
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

# Close Connections
s_client_receive.close()
s_client_send.close()
# Close Sockets
s_receive.close()
s_send.close()
# Release resources
cap.release()
cv2.destroyAllWindows()