import cv2
import numpy as np
from ultralytics import YOLO
from tkinter import Tk, Canvas, Button, Label
from numba import njit, prange
from collections import defaultdict, deque
from PIL import Image, ImageTk

import supervision as sv

TARGET_WIDTH = 25
TARGET_HEIGHT = 250

TARGET = np.array([[0, 0], [TARGET_WIDTH - 1, 0], [TARGET_WIDTH - 1, TARGET_HEIGHT - 1], [0, TARGET_HEIGHT - 1]])

@njit(parallel=True)
def calculate_speed(coordinates, video_fps):
    labels = []
    for tracker_id in prange(len(coordinates)):
        if len(coordinates[tracker_id]) < video_fps / 2:
            labels.append(f"#{tracker_id}")
        else:
            coordinate_start, coordinate_end = coordinates[tracker_id][-1], coordinates[tracker_id][0]
            distance = abs(coordinate_start - coordinate_end)
            time = len(coordinates[tracker_id]) / video_fps
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

    def select_points(self, frame):
        root = Tk()
        root.title("Point Selector")

        canvas = Canvas(root, width=frame.shape[1], height=frame.shape[0])
        canvas.pack()

        label = Label(root, text="Click on points to fine-tune the region.")
        label.pack()

        button = Button(root, text="Confirm Points", command=root.destroy)
        button.pack()

        canvas.create_image(0, 0, anchor="nw", image=self.cv2_to_tkinter(frame))

        canvas.bind("<Button-1>", lambda event: self.on_click(event, canvas))
        root.mainloop()

        return np.array(self.points)

    def on_click(self, event, canvas):
        x, y = event.x, event.y
        self.points.append([x, y])
        canvas.create_oval(x - 5, y - 5, x + 5, y + 5, fill="red")

    def cv2_to_tkinter(self, image):
        image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
        return ImageTk.PhotoImage(Image.fromarray(image))


def process_video():
    video_path = "test_images/leftside.mp4"
    confidence_threshold = 0.3
    iou_threshold = 0.7

    video_info = sv.VideoInfo.from_video_path(video_path=video_path)
    model = YOLO('train3/weights/best.onnx', task='detect')

    byte_track = sv.ByteTrack(frame_rate=video_info.fps, track_thresh=confidence_threshold)

    point_selector = PointSelector()
    frame_generator = sv.get_video_frames_generator(source_path=video_path)

    # Select points only once outside the loop
    SOURCE = point_selector.select_points(next(frame_generator))
    SOURCE.extend(point_selector.points)

    if len(SOURCE) < 3:
        print("Error: Please select at least three points for perspective transformation.")
        return

    polygon_zone = sv.PolygonZone(polygon=SOURCE, frame_resolution_wh=video_info.resolution_wh)
    view_transformer = ViewTransformer(source=SOURCE, target=TARGET)

    coordinates = defaultdict(lambda: deque(maxlen=video_info.fps))

    for frame in frame_generator:
        result = model(frame)[0]
        detections = sv.Detections.from_ultralytics(result)
        detections = detections[detections.confidence > confidence_threshold]
        detections = detections[polygon_zone.trigger(detections)]
        detections = detections.with_nms(threshold=iou_threshold)
        detections = byte_track.update_with_detections(detections=detections)

        points = detections.get_anchors_coordinates(anchor=sv.Position.BOTTOM_CENTER)
        points = view_transformer.transform_points(points=points).astype(int)

        for tracker_id, [_, y] in zip(detections.tracker_id, points):
            coordinates[tracker_id].append(y)

        labels = calculate_speed(coordinates, video_info.fps)

        annotated_frame = frame.copy()
        annotated_frame = sv.TraceAnnotator().annotate(scene=annotated_frame, detections=detections)
        annotated_frame = sv.BoundingBoxAnnotator().annotate(scene=annotated_frame, detections=detections)
        annotated_frame = sv.LabelAnnotator().annotate(scene=annotated_frame, detections=detections, labels=labels)

        cv2.imshow("frame", annotated_frame)
        if cv2.waitKey(1) & 0xFF == ord("q"):
            break

    cv2.destroyAllWindows()

if __name__ == "__main__":
    process_video()
