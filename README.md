# Blind Pedestrian Guidance System

This project was developed as part of a TIPE (Travaux d'Initiative Personnelle Encadrés) on the theme of **Health and Prevention**. It aims to assist visually impaired individuals in navigating their environment more safely using an autonomous electronic system.

## Project Purpose

The goal of this system is to provide real-time obstacle detection and audio feedback for blind pedestrians, allowing them to move with increased confidence and autonomy. Inspired by existing technologies like NELO and the Traveller device, this solution emphasizes portability, real-time response, and audio alert mechanisms without requiring internet connectivity.

## Features

### Obstacle Detection
- Utilizes an **HC-SR04 ultrasonic sensor** to detect nearby obstacles.
- Alerts the user if an object is detected within 2 meters.

### Real-Time Audio Feedback
- Audio alerts are generated using a speaker driven by the **TMRpcm library**.
- Pre-recorded messages are stored and played from a **microSD card**.

### Hardware Integration
- Compact system embedded in a custom-designed case (initially 3D-printed, later built in wood).
- Includes a power switch and external battery supply.

## Hardware Components

- **Microcontroller:** Arduino Mega 2560
- **Ultrasonic Sensor:** HC-SR04
- **Audio Module:** TMRpcm library with LM386 amplifier and speaker
- **Storage:** microSD card reader
- **Power Supply:** 9V battery with toggle switch
- **Optional:** Pixy CMUcam5 (experimental)

## Software Requirements

- **Arduino IDE**
- **TMRpcm Library** (for audio playback):  
  Install via Library Manager or from [TMRpcm on Arduino](https://www.arduino.cc/reference/en/libraries/tmrpcm/)

## Installation and Compilation

Clone the repository and open the project in Arduino IDE:

```bash
git clone https://github.com/your-username/blind-guidance-system.git
cd blind-guidance-system
```

Make sure to:
1. Connect your Arduino Mega 2560.
2. Install required libraries (notably TMRpcm).
3. Load and upload `Blind_Guidance.ino` to your board.

## How It Works

The system continuously measures the distance using the HC-SR04 sensor. If an object is detected closer than the threshold, it plays an audio message via the speaker to warn the user.

## Future Improvements

- Reduce enclosure size and improve ergonomics.
- Support for Bluetooth headphones.
- Add voltage regulation using LM2596 module.
- Customize messages based on distance.

## Authors
