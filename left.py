import cv2
from ultralytics import YOLO

# Load the YOLOv8 model
model = YOLO('yolov8n.pt')

# Open the video file
video_path = r"C:\Users\skr\Documents\GitHub\CornerSentinal\test_images\leftside.mp4"
cap = cv2.VideoCapture(video_path)

# Get video properties
frame_width = int(cap.get(3))
frame_height = int(cap.get(4))
frame_rate = cap.get(cv2.CAP_PROP_FPS)

# Define the codec and create a VideoWriter object with MP4 format (H.265)
output_path = r"C:\Users\skr\Documents\GitHub\CornerSentinal\test_images\leftside_output.mp4"
fourcc = cv2.VideoWriter_fourcc(*'mp4v')  # Use 'H264' for MP4 format
out = cv2.VideoWriter(output_path, fourcc, frame_rate, (frame_width, frame_height))

# Dictionary to store object positions in previous frames
prev_positions = {}

# Constants for conversion
pixels_to_km = 0.1  # Adjust this value based on the actual distance represented by the pixels

# Loop through the video frames
while cap.isOpened():
    # Read a frame from the video
    success, frame = cap.read()

    if success:
        # Run YOLOv8 tracking on the frame, persisting tracks between frames
        results = model.track(frame, persist=True, tracker="bytetrack.yaml")


        # Visualize the results on the frame
        annotated_frame = results[0].plot()

        # Write the annotated frame to the output video file
        out.write(annotated_frame)

        # Display the annotated frame
        cv2.imshow("YOLOv8 Tracking", annotated_frame)

        

        # Break the loop if 'q' is pressed
        if cv2.waitKey(1) & 0xFF == ord("q"):
            break
    else:
        # Break the loop if the end of the video is reached
        break

# Release the video capture object, VideoWriter, and close the display window
cap.release()
out.release()
cv2.destroyAllWindows()
