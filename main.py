from collections import defaultdict
import cv2
import numpy as np
from ultralytics import YOLO

# Load the YOLOv8 model
model = YOLO('yolov8x.pt')

# Open the video file
video_path = "/content/CornerSentinal/test_images/leftside.mp4"
cap = cv2.VideoCapture(video_path)

# Define the constants for speed calculation
VIDEO_FPS = 30
FACTOR_KM = 3.6
LATENCY_FPS = 15

# Store the track history
track_history = defaultdict(lambda: [])

# Define the output video file path with MP4 format
output_video_path = "/content/CornerSentinal/test_images/leftside_out.mp4"
fourcc = cv2.VideoWriter_fourcc(*"mp4v")  # Use "mp4v" for H.264 compression
out = cv2.VideoWriter(output_video_path, fourcc, VIDEO_FPS, (int(cap.get(3)), int(cap.get(4))))

# Function to calculate speed using Optical Flow
def calculate_speed(flow, factor_km=FACTOR_KM, latency_fps=LATENCY_FPS):
    magnitudes = np.sqrt(flow[:, :, 0] ** 2 + flow[:, :, 1] ** 2)
    average_speed = (np.mean(magnitudes) * factor_km * latency_fps) / 1200
    return average_speed

# Placeholder function to simulate displaying binary data
def display_binary_code(frame, binary_code):
    binary_text = "Binary Code: " + ''.join(binary_code)
    cv2.putText(frame, binary_text, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 0, 0), 2)

while cap.isOpened():
    # Read a frame from the video
    success, frame = cap.read()

    if success:
        # Run YOLOv8 tracking on the frame, persisting tracks between frames
        results = model.track(frame, persist=True, tracker='botsort.yaml', classes=[2, 3, 5, 7])

        # Check if the results are not None and contain boxes
        if results is not None and len(results) > 0:
            # Get the boxes, ids, and confidences
            boxes = results[0].boxes.xyxy.cpu().numpy().astype(int)
            class_id = results[0].boxes.id.cpu().numpy().astype(int)

            # Plot the tracks and calculate/display speeds using Optical Flow
            for box in boxes:
                x, y, w, h = box
                track_id = hash(box)
                track = track_history[track_id]
                track.append((float(x), float(y)))  # x, y center point

                # Calculate speed using Optical Flow
                if len(track) > 1:
                    prev_pts = np.array(track[-2]).reshape(-1, 1, 2).astype(np.float32)
                    curr_pts = np.array(track[-1]).reshape(-1, 1, 2).astype(np.float32)

                    flow, status, _ = cv2.calcOpticalFlowPyrLK(
                        cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY),
                        cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY),
                        prev_pts,
                        None
                    )

                    speed = calculate_speed(flow)

                    # Display speed on the frame (corrected scaling)
                    cv2.putText(annotated_frame, f"Speed: {speed:.2f} km/h", (int(x), int(y) - 10),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 0, 0), 2)
                    # Binary code generation based on conditions
                    binary_code = ['0'] * 7
                    binary_code[0] = '1' #start bit
                    class_id = int(max(results[0].boxes.cls))  # Taking max class id
                    if class_id == 2:  # Motorcycle
                        binary_code[1:4] = ['001']
                    elif class_id == 3: # Car
                        binary_code[1:4] = ['010']
                    elif class_id == 5 or 7: # Bus or Truck
                        binary_code[1:4] = ['101'] 
                    if speed > 60: # Overspeed Vehicle
                        binary_code[5:7] = '11'
                    elif speed >= 40 && speed < 60:
                        binary_code[5:7] = '10'
                    elif speed > 1.5 && speed < 40:
                        binary_code[5:7] = '01' 
                    
                    # Display binary code on the frame
                    display_binary_code(annotated_frame, binary_code)

            # Display the annotated frame
            out.write(annotated_frame)  # Save the frame to the output video

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
