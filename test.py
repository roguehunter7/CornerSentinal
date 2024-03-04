import cv2
import numpy as np
from ultralytics import YOLO
from tkinter import Tk, Canvas, Button, Label
from numba import njit, prange
from collections import defaultdict
import threading

import supervision as sv

# Define the initial source points (will be updated interactively)
SOURCE = []

TARGET_WIDTH = 25
TARGET_HEIGHT = 250

TARGET = np.array(
    [
        [0, 0],
        [TARGET_WIDTH - 1, 0],
        [TARGET_WIDTH - 1, TARGET_HEIGHT - 1],
        [0, TARGET_HEIGHT - 1],
    ]
)


@njit(parallel=True)
def calculate_speed(coordinates, video_info):
    labels = []
    for tracker_id in prange(len(coordinates)):
        if len(coordinates[tracker_id]) < video_info.fps / 2:
            labels.append(f"#{tracker_id}")
        else:
            coordinate_start = coordinates[tracker_id][-1]
            coordinate_end = coordinates[tracker_id][0]
            distance = abs(coordinate_start - coordinate_end)
            time = len(coordinates[tracker_id]) / video_info.fps
            speed = distance / time * 3.6
            labels.append(f"#{tracker_id} {int(speed)} km/h")
    return labels


class ViewTransformer:
    def __init__(self, source: np.ndarray, target: np.ndarray) -> None:
        self.source = source.astype(np.float32)
        self.target = target.astype(np.float32)
        self.m = cv2.getPerspectiveTransform(self.source, self.target)

    def transform_points(self, points: np.ndarray) -> np.ndarray:
        if points.size == 0:
            return points

        reshaped_points = points.reshape(-1, 1, 2).astype(np.float32)
        transformed_points = cv2.perspectiveTransform(reshaped_points, self.m)
        return transformed_points.reshape(-1, 2)


class PointSelector:
    def __init__(self):
        self.points = []
        self.root = Tk()
        self.root.title("Point Selector")
        self.canvas = Canvas(self.root, width=800, height=600)
        self.canvas.pack()
        self.label = Label(self.root, text="Click on points to fine-tune the region.")
        self.label.pack()
        self.button = Button(self.root, text="Confirm Points", command=self.confirm_points)
        self.button.pack()
        self.canvas.bind("<Button-1>", self.on_click)
        self.confirmation_event = threading.Event()
        threading.Thread(target=self.root.mainloop, daemon=True).start()

    def on_click(self, event):
        x, y = event.x, event.y
        self.points.append([x, y])
        self.canvas.create_oval(x - 5, y - 5, x + 5, y + 5, fill="red")

    def confirm_points(self):
        if len(self.points) >= 3:  # Require at least three points for perspective transformation
            self.confirmation_event.set()
            self.root.destroy()
        else:
            self.label.config(text="Please select at least three points for fine-tuning.")
            self.confirmation_event.clear()  # Clear the event to prevent false confirmation


def process_video():
    # Set the hardcoded input video path
    video_path = "test_images/leftside.mp4"
    
    # Set hardcoded parameters for optimization
    confidence_threshold = 0.3
    iou_threshold = 0.7

    video_info = sv.VideoInfo.from_video_path(video_path=video_path)
    model = YOLO('train3/weights/best.onnx', task='detect')

    byte_track = sv.ByteTrack(
        frame_rate=video_info.fps, track_thresh=confidence_threshold
    )

    thickness = sv.calculate_dynamic_line_thickness(
        resolution_wh=video_info.resolution_wh
    )
    text_scale = sv.calculate_dynamic_text_scale(resolution_wh=video_info.resolution_wh)
    bounding_box_annotator = sv.BoundingBoxAnnotator(thickness=thickness)
    label_annotator = sv.LabelAnnotator(
        text_scale=text_scale,
        text_thickness=thickness,
        text_position=sv.Position.BOTTOM_CENTER,
    )
    trace_annotator = sv.TraceAnnotator(
        thickness=thickness,
        trace_length=video_info.fps * 2,
        position=sv.Position.BOTTOM_CENTER,
    )

    frame_generator = sv.get_video_frames_generator(source_path=video_path)

    # Allow the user to interactively select points for fine-tuning
    point_selector = PointSelector()
    point_selector.confirmation_event.wait()  # Wait for confirmation before proceeding
    SOURCE.extend(point_selector.points)

    # Ensure at least three points are selected for perspective transformation
    if len(SOURCE) < 3:
        print("Error: Please select at least three points for perspective transformation.")
        return

    polygon_zone = sv.PolygonZone(
        polygon=SOURCE, frame_resolution_wh=video_info.resolution_wh
    )
    view_transformer = ViewTransformer(source=SOURCE, target=TARGET)

    coordinates = defaultdict(lambda: deque(maxlen=video_info.fps))

    for frame in frame_generator:
        result = model(frame)[0]
        detections = sv.Detections.from_ultralytics(result)
        detections = detections[detections.confidence > confidence_threshold]
        detections = detections[polygon_zone.trigger(detections)]
        detections = detections.with_nms(threshold=iou_threshold)
        detections = byte_track.update_with_detections(detections=detections)

        points = detections.get_anchors_coordinates(
            anchor=sv.Position.BOTTOM_CENTER
        )
        points = view_transformer.transform_points(points=points).astype(int)

        for tracker_id, [_, y] in zip(detections.tracker_id, points):
            coordinates[tracker_id].append(y)

        labels = calculate_speed(coordinates, video_info)

        annotated_frame = frame.copy()
        annotated_frame = trace_annotator.annotate(
            scene=annotated_frame, detections=detections
        )
        annotated_frame = bounding_box_annotator.annotate(
            scene=annotated_frame, detections=detections
        )
        annotated_frame = label_annotator.annotate(
            scene=annotated_frame, detections=detections, labels=labels
        )

        cv2.imshow("frame", annotated_frame)
        if cv2.waitKey(1) & 0xFF == ord("q"):
            break
    cv2.destroyAllWindows()


if __name__ == "__main__":
    # Process the video with hardcoded parameters and input video path
    process_video()
