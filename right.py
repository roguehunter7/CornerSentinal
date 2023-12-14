import socket
from collections import defaultdict
import cv2
import numpy as np
from time import time
from ultralytics import YOLO

import socket

def send_binary_code(ip, port, binary_code):
    # Create a socket object
    sender_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # Connect to the receiver
    sender_socket.connect((ip, port))
    # Send the binary code
    sender_socket.sendall(binary_code.encode())
    # Close the socket
    sender_socket.close()

def receive_binary_code(ip, port):
    # Create a socket object
    receiver_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # Bind the socket to a specific IP address and port
    receiver_socket.bind((ip, port))
    # Listen for incoming connections
    receiver_socket.listen()
    print("Waiting for connection...")
    # Accept a connection from the sender
    sender_socket, sender_address = receiver_socket.accept()
    print(f"Connected to {sender_address}")
    # Receive the binary code
    binary_code_received = sender_socket.recv(1024).decode('utf-8')
    # Close the sockets
    sender_socket.close()
    receiver_socket.close()
    return binary_code_received

# Load the YOLOv8 model
model = YOLO('yolov8n.pt')

# Open the video file
video_path = "/test_images/rightside.mp4"
cap = cv2.VideoCapture(video_path)

# Define the constants for speed calculation
VIDEO_FPS = int(cap.get(cv2.CAP_PROP_FPS))
print(f"Video FPS: {VIDEO_FPS}")
FACTOR_KM = 3.6
LATENCY_FPS = int(cap.get(cv2.CAP_PROP_FPS)) / 2
print(f"Dynamic Latency FPS: {LATENCY_FPS}")

# Store the track history for each vehicle
track_history = defaultdict(list)
stationary_timers = defaultdict(float)

# Define the output video file path with MP4 format
output_video_path = "/test_images/rightside_out.mp4"
fourcc = cv2.VideoWriter_fourcc(*"mp4v")  # Use "mp4v" for H.264 compression
out = cv2.VideoWriter(output_video_path, fourcc, VIDEO_FPS, (int(cap.get(3)), int(cap.get(4))))

# Function to calculate Euclidean distance
def calculate_distance(point1, point2):
    return np.sqrt((point2[0] - point1[0]) ** 2 + (point2[1] - point1[1]) ** 2)

# Function to calculate speed using Euclidean distance
def calculate_speed(distances, factor_km=FACTOR_KM, latency_fps=LATENCY_FPS):
    average_speed = (np.mean(distances) * factor_km) / latency_fps
    return average_speed

# Function to generate 9-bit binary code based on conditions
def generate_binary_code(class_id, speed, is_stationary, is_wrong_side):
    binary_code = ['0'] * 9
    binary_code[0] = '1'  # start bit

    # Stationary bit
    binary_code[1] = '1' if is_stationary else '0'

    # Replace class id section
    if class_id == 2:  # Motorcycle
        binary_code[2:5] = '001'
    elif class_id == 3:  # Car
        binary_code[2:5] = '010'
    elif class_id in [5, 7]:  # Bus or Truck
        binary_code[2:5] = '011'

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

# Placeholder function to simulate displaying binary data
def display_warning_message(frame, binary_code):
    warning_message = "Warning: " + binary_code
    cv2.putText(frame, warning_message, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 0, 0), 2)

# Placeholder for the previous frame and points for optical flow
prev_frame = None
prev_pts = None

# Replace this with the actual IP address of Raspberry Pi 2
pi2_ip = "192.168.1.1"
common_port = 12345  # Choose a suitable common port for communication

while cap.isOpened():
    # Read a frame from the video
    success, frame = cap.read()

    if success:
        # Convert the frame to grayscale for optical flow
        frame_gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

        # Run YOLOv8 tracking on the RGB frame, persisting tracks between frames
        results = model.track(frame, persist=True, tracker='botsort.yaml', classes=[2, 3, 5, 7])
        annotated_frame = results[0].plot()

        # Check if the results are not None and contain boxes
        if results[0].boxes.id is not None:
            # Get the boxes, ids, and confidences
            boxes = results[0].boxes.xywh.cpu().numpy().astype(int)
            class_id = results[0].boxes.cls.cpu().numpy().astype(int)
            track_ids = results[0].boxes.id.cpu().numpy().astype(int)

            # Plot the tracks and calculate/display speeds using Euclidean distance
            for i, box in enumerate(boxes):
                x, y, w, h = box
                track = track_history[track_ids[i]]
                track.append((float(x + w / 2), float(y + h / 2)))  # x, y center point

                # Check if the motion is towards the camera
                if len(track) >= 2 and track[-2][1] < track[-1][1]:  # Check y-coordinate for motion towards the camera
                    # Calculate Euclidean distance
                    distances = [calculate_distance(track[j], track[j + 1]) for j in range(len(track) - 1)]

                    # Calculate speed using Euclidean distance
                    if len(distances) > 1:
                        speed = calculate_speed(distances)

                        # Check if the vehicle is stationary
                        is_stationary = speed < 1.0  # You can adjust the threshold as needed

                        # Update stationary timer
                        stationary_timers[track_ids[i]] = time() if not is_stationary else stationary_timers[track_ids[i]]

                        # Check if the vehicle has been stationary for more than 10 seconds
                        if time() - stationary_timers[track_ids[i]] > 10.0:
                            is_stationary = True

                        # Check if the vehicle is on the wrong side
                        is_wrong_side = False  # Replace with your logic

                        # Generate binary code based on conditions
                        if is_stationary and is_wrong_side:
                            binary_code = generate_binary_code(class_id[i], speed, is_stationary, is_wrong_side)
                        elif class_id[i] in [5, 7]:
                            binary_code = generate_binary_code(class_id[i], speed, is_stationary, is_wrong_side)
                        elif speed > 60 or is_wrong_side:
                            binary_code = generate_binary_code(class_id[i], speed, is_stationary, is_wrong_side)
                        else:
                            binary_code = generate_binary_code(class_id[i], speed, is_stationary, is_wrong_side)

                        # Display warning message at the top left of the frame
                        display_warning_message(annotated_frame, binary_code)

                        # Display speed on the frame (corrected scaling)
                        cv2.putText(annotated_frame, f"Speed: {speed:.2f} km/h", (int(x), int(y) - 10),
                                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 0, 0), 2)

                        # Extract the region around the detected object
                        roi = frame_gray[int(y):int(y+h), int(x):int(x+w)]

                        if prev_frame is not None and prev_pts is not None:
                            # Resize the previous frame to match the size of roi
                            prev_frame_resized = cv2.resize(prev_frame, (roi.shape[1], roi.shape[0]))

                            # Calculate optical flow
                            flow = cv2.calcOpticalFlowPyrLK(prev_frame_resized, roi, prev_pts, None,
                                                               winSize=(15, 15), maxLevel=2)

                            # Calculate Euclidean distance in optical flow
                            flow_distances = np.sqrt(np.sum((prev_pts - flow[0])**2, axis=2))

                            # Display optical flow vectors on the frame
                            for j in range(len(flow_distances)):
                                if flow_distances[j] > 0.5:  # Threshold for optical flow distance
                                    x1, y1 = prev_pts[j].astype(int).ravel()
                                    x2, y2 = (x + flow[0][j][0], y + flow[0][j][1]).astype(int)
                                    cv2.line(annotated_frame, (x1, y1), (x2, y2), (0, 255, 0), 2)

                            # Update previous frame and points for the next iteration
                            prev_frame = roi
                            prev_pts = np.array([[(int(w/2), int(h/2))]], dtype=np.float32)

                            # Send the binary code to Raspberry Pi 2
                            send_binary_code(pi2_ip, common_port, binary_code)

                            # Receive binary code from Raspberry Pi 2 and print it
                            received_binary_code = receive_binary_code("0.0.0.0", common_port)
                            print(f"Received Binary Code from Raspberry Pi 2: {received_binary_code}")

            # Display the annotated frame
            out.write(annotated_frame)  # Save the frame to the output video
            cv2.imshow("Frame", annotated_frame)
        # Break the loop if 'q' is pressed
        if cv2.waitKey(1) & 0xFF == ord("q"):
            break
    else:
        # Break the loop if the end of the video is reached
        break

# Release the video writer and close the display window
out.release()
cap.release()
cv2.destroyAllWindows()
