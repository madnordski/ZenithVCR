# ZenithVCR
This repository is for the **Zenith Voice Controlled Radio**.
- by J.King, March 2025
- Special thanks to OpenAI’s ChatGPT for assistance with debugging, Flask optimization, and streaming improvements.
## 🎵 Overview
This project brings a **1940s Zenith Radio** back to life with modern streaming capabilities! It features:
- **ESP32-S3 N8R2 DevKitC** as the core controller (w/at least 2MB PSRAM).
- **Python Flask Server** for serving music, managing playlists and serving podcasts.
- **Voice control** via the **DFRobot SEN0539-EN** module.
- **Stepper-motor-controlled tuning dial** for station selection.
- **Wi-Fi connectivity** with adaptive access point switching.

## 🛠️ Features
✅ **Stream Internet Radio, Podcasts, and Local MP3/OGG Files**  
✅ **Control Playback via Voice Commands or Encoder Knob w/Button**  
✅ **mDNS for Easy Web Interface Access**  
✅ **Multi-Threaded Flask Server for Smooth UI & Streaming**  
✅ **Auto-Switching Between Wi-Fi Access Points**
✅ **Creates Access Point for Easy WiFi Setup** 
✅ **Web Interface for entering Internet Radio Station URLs** 

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
- Serves a **web-based album browser at http://zenithvcr:8001/** for easy selection.
- Runs two Flask servers:
  - **Port 8000:** Streaming server (used by ESP32 to play local content).
  - **Port 8001:** Web UI for album selection.
- **mDNS** so any computer on your network can find the Flask servers**
  - **http://zenithvcr:8001/browse**

### **2️⃣ ESP32-S3 (Zenith Voice Controlled Radio)**
- Fetches music from the server.
- Uses **I²C speech recognition** for voice control.
- Switches between **Internet radio, local MP3s, and podcasts**.
- The station playing is indicated by an **interactive tuning dial**.
- **Serves a webpage at zenith.local** for changing Internet Radio URLs.
  - this page also includes a complete list of the voice commands.
- Press and hold the volume knob when turning on to setup WiFi for your network.
  - Connect to access point, **Zenith Setup** and IP **192.168.1.4**.

## 🚀 Installation & Setup
### **1** Optionally, Set Up the Flask Music Server
The Zenith Voice Controlled Radio does not depend on the Flask Server.
It's used only to serve your local content to the Zenith.
#### **Install Dependencies**
```sh
pip install flask gevent ffmpeg-python
python3 stream_machine.py
```
### **2** Flash the ESP32 with the Radio Firmware
Use Arduino IDE or PlatformIO to upload the code.
🎙️ Voice Commands
(these are configured by you.  Follow the DFRobot SEN0539 instructions).
The code burned into the ESP32 is ready to respond to commands
like the following.  Use http://zenith.local to associate your voice
commands to all the tasks defined in the code.
### Command	Action (complete list at zenith.local)
#### "Hello robot, pause radio"	  stops playback
#### "Hello robot, resume radio"	resumes where it left off
#### "Hello robot, volume up"     turns up the volume
#### "Hello robot, volume down"   turns down the volume
#### "Hello robot, station up"    moves the dial up one station
#### "Hello robot, station down"  moves the dial down one station

### **3** Put the Zenith Voice Controlled Radio on your Network
When the Zenith runs for the first time, it creates an access point called "Zenith Setup".
You can always force the Zenith into access point mode by holding in the volume knob when
powering on.
#### Step a. Connect your phone or computer WiFi to Zenith Setup.
#### Step b. Go to http://192.168.1.4 to enter the WiFi Credentials
#### Step c. Update and restart the Zenith (follow instructions)
#### Step d. Now you can define your Internet radio stations using http://zenith.local.
#### Step e. Enjoy!
#### Step f. If you are running the Flask server, enter the streaming url (use IP) at http://zenith.local.

🎛️ Web Interface for locally served content (Flask Server)
### Access the album browser at http://zenithvcr.local:8001
### Stream control handled at http://zenithvcr.local:8000/stream
⚡ Troubleshooting
### Problem: Web interface is slow while streaming
✅ Solution: Flask now runs two separate servers (8000 & 8001) to keep the UI responsive.

### Problem: ESP32 disconnects and Flask hangs
✅ Solution: Improved error handling ensures FFmpeg stops properly.

### Problem: Album names with special characters break requests
✅ Solution: URLs are properly encoded & decoded in JavaScript and Flask.

📜 License
### GPL-3.0 - Open Source! Feel free to modify and improve provided you pass on your work to others.


