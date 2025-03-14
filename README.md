# ZenithVCR
This repository is for the Zenith Voice Controlled Radio.

## 🎵 Overview
This project brings a **1940s Zenith Radio** back to life with modern streaming capabilities! It features:
- **ESP32-S3 DevKitC** as the core controller.
- **Python Flask Server** for serving music and managing playlists.
- **Voice control** via the **DFRobot SEN0539-EN** module.
- **Stepper-motor-controlled tuning dial** for station selection.
- **Wi-Fi connectivity** with adaptive access point switching.


## 🛠️ Features
✅ **Stream Internet Radio, Podcasts, and Local MP3/OGG Files**  
✅ **Control Playback via Voice Commands or Encoder Button**  
✅ **mDNS for Easy Web Interface Access**  
✅ **Multi-Threaded Flask Server for Smooth UI & Streaming**  
✅ **Auto-Switching Between Wi-Fi Access Points**  

## 🔧 Hardware Used
- **ESP32-S3 DevKitC-1 (YD-ESP32-S3 N8R2)**
- **DFRobot SEN0539-EN Speech Recognition Module**
- **Stepper Motor & Controller for Dial Pointer**
- **Permanent Magnet Speaker (Replaced Original Field Coil)**
- **Custom 3D-Printed Mounts for New Components**
- **9V 2A Power Supply + Buck/Boost Converters**
- **Vintage 1940s Zenith 4G903 Radio Chassis**

## 🖥️ How It Works
### **1️⃣ Python Flask Server (Raspberry Pi / Linux)**
- Hosts music via **FFmpeg for real-time transcoding**.
- Serves a **web-based album browser** for easy selection.
- Runs two Flask servers:
  - **Port 8000:** Streaming server.
  - **Port 8001:** Web UI for album selection.

### **2️⃣ ESP32-S3 (Zenith Internet Radio)**
- Fetches music from the server.
- Uses **I²C speech recognition** for voice control.
- Switches between **Internet radio, local MP3s, and podcasts**.
- Displays playback info using an **interactive tuning dial**.

## 🚀 Installation & Setup
### **1️⃣ Set Up the Flask Music Server**
#### **Install Dependencies**
```sh
pip install flask gevent ffmpeg-python
