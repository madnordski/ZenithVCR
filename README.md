# ZenithVCR
This repository is for the Zenith Voice Controlled Radio.

## 🎵 Overview
This project brings a **1940s Zenith Radio** back to life with modern streaming capabilities! It features:
- **ESP32-S3 DevKitC** as the core controller (w/at least 2MB PSRAM).
- **Python Flask Server** for serving music, managing playlists and serving podcasts.
- **Voice control** via the **DFRobot SEN0539-EN** module.
- **Stepper-motor-controlled tuning dial** for station selection.
- **Wi-Fi connectivity** with adaptive access point switching.

## 🛠️ Features
✅ **Stream Internet Radio, Podcasts, and Local MP3/OGG Files**  
✅ **Control Playback via Voice Commands or Encoder Knob w/Button**  
✅ **mDNS for Easy Web Interface Access (http://zenith.local)**  
✅ **Multi-Threaded Flask Server for Smooth UI & Streaming**  
✅ **Auto-Switching Between Wi-Fi Access Points**  

## 🔧 Hardware Used
- **ESP32-S3 DevKitC-1 (YD-ESP32-S3 N8R2)**
- **DFRobot SEN0539-EN Speech Recognition Module**
- **Stepper Motor (PM08-2) & Controller (TMC2209) for Dial Pointer**
- **Permanent Magnet Speaker (Replaced Original Field Coil)**
- **9V 3A Power Supply + Buck/Boost Converters**
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
- The station playing is indicated by an **interactive tuning dial**.
- **Servea webpage at zenith.local** for changing Internet Radio URLs
- Press and hold the volume knob when turning on to setup WiFi for your network.
  - Connect to access point, **Zenith Setup** and IP **192.168.1.4**.

## 🚀 Installation & Setup
### **1️⃣ Set Up the Flask Music Server**
#### **Install Dependencies**
```sh
pip install flask gevent ffmpeg-python
python3 stream_machine.py
```
2️⃣ Flash the ESP32 with the Radio Firmware
Use Arduino IDE or PlatformIO to upload the code.
🎙️ Voice Commands
### Command	Action
#### "Zenith, play my favorite"	Switches to saved station
#### "Zenith, next track"	Skips to the next song
#### "Zenith, I like this"	Saves the current station as a favorite
🎛️ Web Interface
### Access the album browser at http://zenith.local:8001
### Stream control handled at http://zenith.local:8000/stream
⚡ Troubleshooting
### Problem: Web interface is slow while streaming
✅ Solution: Flask now runs two separate servers (8000 & 8001) to keep the UI responsive.

### Problem: ESP32 disconnects and Flask hangs
✅ Solution: Improved error handling ensures FFmpeg stops properly.

### Problem: Album names with special characters break requests
✅ Solution: URLs are properly encoded & decoded in JavaScript and Flask.

📜 License
### MIT License - Open Source! Feel free to modify and improve.


