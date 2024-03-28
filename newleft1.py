from collections import defaultdict
import cv2
import numpy as np
import time
from ultralytics import YOLO
import socket
import threading
import queue
from send import *
from lanedetector import *

# Load the YOLOv8 model
model = YOLO('train3/weights/best.pt')

# Open the video file
video_path = "test_images/leftside.mp4"
cap = cv2.VideoCapture(video_path)

# Constants for speed calculation
VIDEO_FPS = int(cap.get(cv2.CAP_PROP_FPS))
FACTOR_KM = 3.6
LATENCY_FPS = VIDEO_FPS / 2

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
s_client_receive, _ = s_receive.accept()
print("Receiver socket connected")

# Connect to the other RPi for sending
s_client_send, _ = s_send.accept()
print("Sender socket connected")

ld = LineDrawerGUI(video_path)
print("Line coordinates:", ld.line_coords)
print("Options:", ld.option_val)
ld2 = LaneDetector(video_path, ld.line_coords)
print("ld.line coords:",ld.line_coords)
ld2.calculate_all_points()

# Function to calculate Euclidean distance
def calculate_distance(point1, point2):
    return np.sqrt((point2[0] - point1[0]) ** 2 + (point2[1] - point1[1]) ** 2)

# Function to calculate speed using Euclidean distance
def calculate_speed(distances, factor_km, latency_fps):
    if len(distances) <= 1:
        return 0.0
    average_speed = (np.mean(distances) * factor_km) / latency_fps * 10
    return average_speed

# Function to generate 9-bit binary code based on conditions
def generate_binary_code(class_id, speed, is_stationary, is_wrong_side):
    binary_code = ['0'] * 9
    binary_code[0] = '1'  # start bit

    # Stationary bit
    binary_code[1] = '1' if is_stationary else '0'

    if class_id == 0:  # Ambulance
        binary_code[2:5] = '100'
    elif class_id in [2,6,4]:  # Car or Van or Taxi/Auto
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

# Store track history for each vehicle
track_history = defaultdict(list)
stationary_timers = defaultdict(float)

# Queue for sending binary codes
send_queue = queue.Queue()

# Flag to indicate whether the program should continue running
running = True

# Function to send binary code to the other Raspberry Pi
def send_binary_code(sock):
    while True:
        try:
            binary_code = send_queue.get(block=True)
            if binary_code == "STOP":
                break
            sock.sendall(binary_code.encode())
        except Exception as e:
            print(f"Error sending binary code: {e}")
            break

    sock.close()

# Function to receive binary code from the other Raspberry Pi
def receive_binary_code(sock):
    global running
    
    while running:
        try:
            data = sock.recv(1024)
            binary_code = data.decode()
            if binary_code == "STOP":
                running = False
            else:
                print(f"Received binary code: {binary_code}")
                transmit_binary_data(binary_code)
                
        except Exception as e:
            print(f"Error receiving binary code: {e}")
            break
    
    sock.close()

# Function to process video frames
def process_video():
    global running
    # Placeholder for the previous frame and points for optical flow
    prev_frame = None
    prev_pts = None
    # Placeholder for the previous binary code
    prev_binary_code = None
    # Counter to keep track of frames
    frame_counter = 0
    
    while cap.isOpened() and running:
        success, frame = cap.read()

        if success:
            frame_gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

            # Check if YOLO inference should be performed on this frame
            if frame_counter % 2 == 0:
                results = model.track(frame, persist=True, tracker='botsort.yaml', imgsz=(320, 320), int8=True, conf=0.25)
                annotated_frame = results[0].plot()

                if results[0].boxes.id is not None:
                    boxes = results[0].boxes.xywh.cpu().numpy().astype(int)
                    class_id = results[0].boxes.cls.cpu().numpy().astype(int)
                    track_ids = results[0].boxes.id.cpu().numpy().astype(int)

                    for i, box in enumerate(boxes):
                        x, y, w, h = box
                        xmin, ymin, xmax, ymax = x, y, x+w, y+h
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
                            vehicle_pos = calculate_centroid (xmin, ymin, xmax, ymax)
                            print("option:",ld.option_val)
                            correct_lane = lane_detector (ld2.points, vehicle_pos, int(ld.option_val))
                            print("wrong lane or not:", correct_lane)
                         
                            if correct_lane == 1.0 or 0:
                                is_wrong_side = False 
                            else: 
                                is_wrong_side = True
                             
                            binary_code = generate_binary_code(class_id[i], speed, is_stationary, is_wrong_side)
                            
                            display_warning_message(annotated_frame, binary_code)
                            cv2.putText(annotated_frame, f"Speed: {speed:.2f} km/h", (int(x), int(y) - 10),
                                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 0, 0), 2)
                            roi = frame_gray[int(y):int(y + h), int(x):int(x + w)]

                            if binary_code != prev_binary_code:
                                send_queue.put(binary_code)
                                prev_binary_code = binary_code

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

    # Send a signal to stop the other threads
    send_queue.put("STOP")
    running = False

# Start the threads
receive_thread = threading.Thread(target=receive_binary_code,args = (s_client_receive,))
send_thread = threading.Thread(target=send_binary_code, args=(s_client_send,))
video_thread = threading.Thread(target=process_video)

receive_thread.start()
send_thread.start()
video_thread.start()

# Wait for the threads to finish
video_thread.join()
send_thread.join()
receive_thread.join()

# Close Sockets
s_receive.close()
s_send.close()
# Release resources
cap.release()
cv2.destroyAllWindows()