# Corner Sentinel

### Advanced Early Warning System for Steep Corner Accident Prevention 


## Project Overview

**Corner Sentinel** is an advanced system designed to enhance road safety at sharp, blind corners, particularly in hilly regions. It combines **Image Processing**, **Machine Learning**, and **Li-Fi Communication** technologies to detect potential hazards, notify drivers in real time, and help prevent collisions. By leveraging these technologies, Corner Sentinel addresses the challenges posed by poor visibility, speeding, and wrong-side driving in dangerous areas.

This repository contains the complete source code, project report, and relevant technical documentation.

## Table of Contents
- [Project Overview](#project-overview)
- [Features](#features)
- [System Architecture](#system-architecture)
- [Technologies Used](#technologies-used)
- [Installation](#installation)
- [Usage](#usage)
- [Results](#results)
- [Future Scope](#future-scope)
- [References](#references)

---

## Features

1. **Real-Time Vehicle Detection:** Leverages the YOLOv8 model to detect and classify vehicles in real time.
2. **Wrong-Lane Detection:** Identifies vehicles driving in the wrong lane and triggers alerts.
3. **Speed Estimation:** Uses optical flow techniques to estimate the speed of detected vehicles.
4. **Accident/Crash Detection:** Predicts and detects accidents based on sudden stops and abnormal vehicle movements.
5. **Li-Fi Communication:** Sends real-time hazard warnings via modulated light signals to approaching vehicles.
6. **Scalable Architecture:** The system's modular design makes it easy to upgrade and scale for future improvements.

---
## System Architecture

The system is composed of two primary modules:

1. **Machine Learning Module:** This module runs on a Raspberry Pi 5 and performs real-time vehicle detection, classification, and tracking using the YOLOv8 model. It analyzes traffic feeds from cameras positioned at blind corners.
2. **Li-Fi Communication Module:** The module generates binary codes corresponding to detected hazards (e.g., wrong-lane driving, stationary vehicles, speeding) and transmits this data to approaching vehicles via Li-Fi.

The **working** of the system is as follows:
1. **Video Acquisition and Preprocessing**: Cameras capture live traffic feeds, and frames are resized and normalized.
2. **Vehicle Detection and Classification**: YOLOv8 model identifies vehicles and outputs bounding boxes and class labels.
3. **Vehicle Tracking**: The ByteTrack algorithm assigns unique IDs and tracks vehicles across frames.
4. **Hazard Assessment**: Based on factors like speed, lane usage, and the presence of emergency or stationary vehicles, Corner Sentinel assesses potential risks.
5. **Li-Fi Transmission**: Binary code representing the detected hazards is transmitted using LEDs via Manchester Encoding.
6. **Warning Reception**: Vehicles equipped with receivers demodulate signals and warn drivers of potential dangers.

---

## Technologies Used

### 1. **Image Processing**
The system uses image processing to acquire and analyze video streams in real time. Techniques such as **Object Detection** (via YOLOv8) and **Lane Detection** are employed to identify vehicles and lane boundaries. **Optical Flow** techniques are used for speed estimation.

### 2. **Machine Learning**
A custom-trained **YOLOv8 Nano** model is employed for vehicle detection. The model was trained on a combination of Google’s Open Images Dataset and an Indian-Vehicles dataset sourced from Roboflow. The system tracks vehicles, detects their speed, and classifies them by type (e.g., cars, trucks, ambulances).

### 3. **Li-Fi Communication**
Li-Fi, short for Light Fidelity, is used for real-time communication between the system and approaching vehicles. The system encodes binary messages based on hazard assessments and transmits them using **On-Off Keying (OOK)** modulation and **Manchester Encoding** via high-power LEDs. Receivers on vehicles decode these signals and alert drivers about potential risks.

### 4. **Hardware Components**
- **Raspberry Pi 5**: The core device that runs YOLOv8 and manages Li-Fi transmission.
- **Camera**: Captures live traffic feeds for real-time analysis.
- **LEDs**: Used for transmitting binary-coded warnings to vehicles.
- **Solar Panels**: Act as receivers for decoding the Li-Fi signals and extracting binary codes.

---

## Installation

### Prerequisites
- Raspberry Pi 5 with **Raspbian OS** installed.
- Python 3.9+ with libraries: `opencv-python`, `ultralytics`, `numpy`.
- YOLOv8 weights.

### Steps
1. Clone this repository to your Raspberry Pi:
    ```bash
    git clone https://github.com/yourusername/corner-sentinel.git
    cd corner-sentinel
    ```

2. Install the necessary Python packages:
    ```bash
    pip install -r requirements.txt
    ```

3. Run the Program.

---

## Usage

### Running the System
To start the system, run the following command:
```bash
python3 final.py
```
The program will begin analyzing traffic feeds, detecting vehicles, and sending real-time alerts using Li-Fi transmission.

### Binary Encoding System
Data sent via Li-Fi is encoded in an 8-bit format:

- **Bit 7**: Stationary Vehicle Detection
- **Bit 6**: Accident Detection
- **Bits 5-3**: Vehicle Type (Ambulance, Car, Bike, Truck)
- **Bit 2**: Wrong-Lane Detection
- **Bits 1-0**: Speed Category (below 40 km/h, 40-60 km/h, above 60 km/h)
---
### Results

- **Vehicle Detection Accuracy**: The YOLOv8 model achieved a high detection accuracy rate during testing.

- **Speed Estimation**: Accurate speed estimation was achieved using the optical flow method.

- **Li-Fi Transmission**: The system successfully transmitted hazard alerts using Li-Fi, which were received and decoded by equipped vehicles.
---
### Future Scope
- **Enhanced Weather Adaptability**: Improving the system to handle adverse weather conditions like rain and fog.

- **Range Extension**: Research into extending the communication range of Li-Fi for broader coverage.

- **Optimized Machine Learning Models**: Utilizing more efficient versions of YOLO and exploring alternative algorithms to enhance performance.

- **Li-Fi Circuit Enhancements**: Introducing noise reduction filters and optimized encoding schemes to improve the robustness of data transmission.
---
### References
The project reworked code from: [Sending Text Messages with Visible Light Communication](https://nerd-corner.com/sending-text-messages-with-visible-light-communication/)

For a complete list of references, please refer to the Project Report and the Project Presentation.