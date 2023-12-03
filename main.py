# Importing Libraries
import cv2
import numpy as np
from datetime import datetime
from ultralytics import YOLO
from sort.sort import Sort
import os

tracker = Sort()
model = YOLO("yolov8n.pt")

# Setting Lines and Variables
BLUE_LINE = [(980, 780), (1225, 780)]
GREEN_LINE = [(980, 800), (1270, 800)]
RED_LINE = [(980, 820), (1310, 820)]

# Only keeping lines for the right side
PREV_LINE_RIGHT = [(980, 770), (1210, 770)]
PASS_LINE_RIGHT = [(995, 860), (1310, 860)]

cross_blue_line = {}
cross_green_line = {}
cross_red_line = {}

avg_speeds = {}

VIDEO_FPS = 30
FACTOR_KM = 3.6
LATENCY_FPS = 15

# Euclidean Distance Function
def euclidean_distance(point1: tuple, point2: tuple):
    x1, y1 = point1
    x2, y2 = point2
    distance = ((x2 - x1) ** 2 + (y2 - y1) ** 2) ** 0.5
    return distance

# Speed Calculation on the Right Side of the Road
def calculate_avg_speed_right(track_id):
    time_bg = (cross_green_line[track_id]["time"] - cross_blue_line[track_id]["time"]).total_seconds()
    time_gr = (cross_red_line[track_id]["time"] - cross_green_line[track_id]["time"]).total_seconds()

    distance_bg = euclidean_distance(cross_green_line[track_id]["point"], cross_blue_line[track_id]["point"])
    distance_gr = euclidean_distance(cross_red_line[track_id]["point"], cross_green_line[track_id]["point"])

    speed_bg = round((distance_bg / (time_bg * VIDEO_FPS)) * (FACTOR_KM * LATENCY_FPS), 2)
    speed_gr = round((distance_gr / (time_gr * VIDEO_FPS)) * (FACTOR_KM * LATENCY_FPS), 2)

    return round((speed_bg + speed_gr) / 2, 2)

# Load the video
VIDEOS_DIR = "/content/CornerSentinal/test_images"
video_path = os.path.join(VIDEOS_DIR, 'pexels_videos_2733 (1080p).mp4')
cap = cv2.VideoCapture(video_path)

# Define the codec and create VideoWriter object
H, W, _ = (int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT)), int(cap.get(cv2.CAP_PROP_FRAME_WIDTH)), 3)
video_path_out = '{}_out.mp4'.format(video_path)
out = cv2.VideoWriter(video_path_out, cv2.VideoWriter_fourcc(*'mp4v'), int(cap.get(cv2.CAP_PROP_FPS)), (W, H))

while cap.isOpened():
    ret, frame = cap.read()

    if not ret:
        break

    height = frame.shape[0]
    width = frame.shape[1]

    mask_right = np.zeros((height, width), dtype=np.uint8)
    pts_right = np.array([[[950, 1050], [950, 580], [1030, 580], [1540, 1080]]])
    pts_right.reshape((-1, 1, 2))
    cv2.fillPoly(mask_right, pts_right, 255)

    right_zone = cv2.bitwise_and(frame, frame, mask=mask_right)

    results = model(right_zone, stream=True)

    for res in results:
        filtered_indeces = np.where(res.boxes.conf.cpu().numpy() > 0.05)[0]
        boxes = res.boxes.xyxy.cpu().numpy()[filtered_indeces].astype(int)

        tracks = tracker.update(boxes)
        tracks = tracks.astype(int)

        for xmin, ymin, xmax, ymax, track_id in tracks:
            xc, yc = int((xmin + xmax) / 2), ymax

            if track_id not in cross_blue_line:
                cross_blue = (BLUE_LINE[1][0] - BLUE_LINE[0][0]) * (yc - BLUE_LINE[0][1]) - (
                            BLUE_LINE[1][1] - BLUE_LINE[0][1]) * (xc - BLUE_LINE[0][0])
                if cross_blue >= 0:
                    cross_blue_line[track_id] = {
                        "time": datetime.now(),
                        "point": (xc, yc)
                    }

            elif track_id not in cross_green_line and track_id in cross_blue_line:
                cross_green = (GREEN_LINE[1][0] - GREEN_LINE[0][0]) * (yc - GREEN_LINE[0][1]) - (
                            GREEN_LINE[1][1] - GREEN_LINE[0][1]) * (xc - GREEN_LINE[0][0])
                if cross_green >= 0:
                    cross_green_line[track_id] = {
                        "time": datetime.now(),
                        "point": (xc, yc)
                    }

            elif track_id not in cross_red_line and track_id in cross_green_line:
                cross_red = (RED_LINE[1][0] - RED_LINE[0][0]) * (yc - RED_LINE[0][1]) - (
                            RED_LINE[1][1] - RED_LINE[0][1]) * (xc - RED_LINE[0][0])
                if cross_red >= 0:
                    cross_red_line[track_id] = {
                        "time": datetime.now(),
                        "point": (xc, yc)
                    }

                    avg_speed = calculate_avg_speed_right(track_id)
                    avg_speeds[track_id] = f"{avg_speed} km/h"

            cross_line_right = (PASS_LINE_RIGHT[1][0] - PASS_LINE_RIGHT[0][0]) * (
                        yc - PASS_LINE_RIGHT[0][1]) - (PASS_LINE_RIGHT[1][1] - PASS_LINE_RIGHT[0][1]) * (
                                       xc - PASS_LINE_RIGHT[0][0])

            if track_id in avg_speeds and track_id in cross_blue_line and track_id in cross_green_line and track_id in cross_red_line and cross_line_right <= 0:
                cv2.putText(img=frame, text=avg_speeds[track_id], org=(xmin, ymin - 10),
                            fontFace=cv2.FONT_HERSHEY_PLAIN, fontScale=1, color=(0, 255, 0), thickness=2)

            cv2.rectangle(img=frame, pt1=(xmin, ymin), pt2=(xmax, ymax), color=(255, 255, 0), thickness=2)

    cv2.line(frame, PASS_LINE_RIGHT[0], PASS_LINE_RIGHT[1], (0, 0, 255), 3)
    cv2.line(frame, PREV_LINE_RIGHT[0], PREV_LINE_RIGHT[1], (252, 111, 3), 3)
    out.write(frame)

    cv2.imshow("Frame", frame)

    if cv2.waitKey(10) & 0xFF == ord('q'):
        cap.release()
        cv2.destroyAllWindows()
        break
cap.release()
out.release()
cv2.destroyAllWindows()
