import cv2
import tkinter as tk
from PIL import Image, ImageTk
import numpy as np
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


