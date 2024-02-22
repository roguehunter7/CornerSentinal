from collections import defaultdict
import cv2
import numpy as np
import time
from ultralytics import YOLO
import tkinter as tk
from PIL import Image, ImageTk
import numpy as np

# Load the YOLOv8 model
model = YOLO('./train3/weights/best.onnx')

# Open the video file
video_path = "test_images/leftside.mp4"
cap = cv2.VideoCapture(video_path)

# Constants for speed calculation
VIDEO_FPS = int(cap.get(cv2.CAP_PROP_FPS))
FACTOR_KM = 3.6
LATENCY_FPS = int(cap.get(cv2.CAP_PROP_FPS)) / 2

# Code For Wrong Lane Detection
class LineDrawerGUI:
    def __init__(self, video_source):
        # Trigger mouse event to draw line
        self.start_x, self.start_y = None, None
        self.end_x, self.end_y = None, None
        self.line_coords = None
        self.root = tk.Tk()
        self.root.title("Draw Detection Line")
        self.video_source = video_source
        self.video = cv2.VideoCapture(video_source)
        ret, frame = self.video.read()
        # Resize the image to a new width and height
        new_width, new_height = 240, 424
        resized_frame = cv2.resize(frame, (new_width, new_height))
        self.image = Image.fromarray(
            cv2.cvtColor(resized_frame, cv2.COLOR_BGR2RGB))
        self.photo = ImageTk.PhotoImage(self.image)
        self.canvas = tk.Canvas(
            self.root, width=self.image.width, height=self.image.height)
        self.canvas.pack(expand=True, fill="both")
        self.canvas.create_image(0, 0, image=self.photo, anchor=tk.NW)
        self.canvas.bind("<Button-1>", self.on_button_press)
        self.canvas.bind("<B1-Motion>", self.on_move_press)
        self.canvas.bind("<ButtonRelease-1>", self.on_button_release)

        self.option_val = tk.StringVar(value="")
        tk.Label(self.root, text="Specify the side of the lane:").pack()
        tk.Radiobutton(self.root, text="Left Lane Correct",
                       variable=self.option_val, value=1 ).pack()
        tk.Radiobutton(self.root, text="Right Lane Correct",
                       variable=self.option_val, value=2 ).pack()
        
        tk.Button(self.root, text="Confirm", command=self.save_names).pack()
        tk.Button(self.root, text="Clear", command=self.clear_lines).pack()
        self.root.mainloop()

    def on_button_press(self, event):
        self.start_x, self.start_y = event.x, event.y

    def on_move_press(self, event):
        self.end_x, self.end_y = event.x, event.y
        self.redraw_lines()

    def on_button_release(self, event):
        self.line_coords = self.redraw_lines()

    def redraw_lines(self):
        self.canvas.delete("line")
        self.canvas.create_line(
            self.start_x, self.start_y, self.end_x, self.end_y, tags="line", fill="red", width=1)
        line_coords = ((self.start_x, self.start_y), (self.end_x, self.end_y))
        return line_coords
    
    def clear_lines(self):
        self.line_coords = []
        self.canvas.delete("line")
        
    def save_names(self):
        self.option_val = self.option_val.get()
        self.video.release()
        self.root.after(1000, self.root.destroy)


class LaneDetector:
    def __init__(self, video_source, line_coords):
        self.first_coords = list(np.array(line_coords[0])*2)
        self.second_coords = list(np.array(line_coords[1])*2)
        self.video_source = video_source
        self.video = cv2.VideoCapture(video_source)
        self.points = None
        self.detection_line_coords = None

    def calculate_all_points(self):
        width_vid = int(self.video.get(cv2.CAP_PROP_FRAME_WIDTH))
        self.points = [[[0, self.first_coords[1]], self.first_coords, self.second_coords, [0, self.second_coords[1]]], [
            [width_vid, self.first_coords[1]], self.first_coords, self.second_coords, [width_vid, self.second_coords[1]]]]
        self.detection_line_coords = [self.first_coords, self.second_coords]

    def draw_shape(self, frame):
        color = (255, 0, 0)
        thickness = 2
        is_closed = False

        curr_point = self.points[0]
        curr_point = np.array(curr_point)
        curr_point = curr_point.reshape((-1, 1, 2))

        return cv2.polylines(frame, [curr_point], is_closed, color, thickness)


def calculate_centroid(xmin, ymin, xmax, ymax):
    x_center = (xmin + xmax) / 2
    y_center = (ymin + ymax) / 2
    return [x_center, y_center]


def draw_detection_line(frame, line):
    color = (253, 55, 165)
    thickness = 2
    is_closed = False

    detection_line = line
    detection_line = np.array(detection_line)
    detection_line = detection_line.reshape((-1, 1, 2))
    frame = cv2.polylines(frame, [detection_line], is_closed, color, thickness)
    return frame


def lane_detector(lane_area, point, option_val):
    correct_lane = cv2.pointPolygonTest(
        np.array(lane_area[option_val - 1]), point, False)
    return correct_lane == 1.0 or correct_lane == 0


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

# Counter to keep track of frames
frame_counter = 0

# Placeholder for the previous frame and points for optical flow
prev_frame = None
prev_pts = None

# Placeholder for the previous binary code
prev_binary_code = None

while cap.isOpened():
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
cv2.destroyAllWindows()