from collections import defaultdict
import cv2
import numpy as np
from ultralytics import YOLO
import supervision as sv

if __name__ == "__main__":
    input_video_path = "test_images/leftside.mp4"
    video_info = sv.VideoInfo.from_video_path(video_path=input_video_path)
    model = YOLO("train3/weights/best.pt")

    byte_track = sv.ByteTrack(
        frame_rate=video_info.fps, track_thresh=0.3  # hardcoded confidence_threshold
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

    frame_generator = sv.get_video_frames_generator(source_path=input_video_path)

    tracker_positions = defaultdict(list)
    tracker_speeds = defaultdict(list)

    for frame in frame_generator:
        result = model(frame)[0]
        detections = sv.Detections.from_ultralytics(result)
        detections = detections[detections.confidence > 0.3]  # hardcoded confidence_threshold
        detections = detections.with_nms(threshold=0.7)  # hardcoded iou_threshold
        detections = byte_track.update_with_detections(detections=detections)

        points = detections.get_anchors_coordinates(
            anchor=sv.Position.BOTTOM_CENTER
        )

        for tracker_id, [_, y] in zip(detections.tracker_id, points):
            tracker_positions[tracker_id].append(y)

            if len(tracker_positions[tracker_id]) >= 2:
                prev_y = tracker_positions[tracker_id][-2]
                curr_y = tracker_positions[tracker_id][-1]
                distance = abs(curr_y - prev_y)
                time = 1 / video_info.fps
                speed = distance / time * 3.6
                tracker_speeds[tracker_id].append(speed)

        labels = []
        for tracker_id in detections.tracker_id:
            if not tracker_speeds[tracker_id]:
                labels.append(f"#{tracker_id}")
            else:
                speed = sum(tracker_speeds[tracker_id]) / len(tracker_speeds[tracker_id])
                labels.append(f"#{tracker_id} {int(speed)} km/h")

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
