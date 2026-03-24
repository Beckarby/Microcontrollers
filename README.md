## Installation & Setup Guide

### 1. Install PlatformIO
This project uses PlatformIO instead of the standard Arduino IDE for better dependency management and project structure.
1. Open VS Code.
2. Go to the **Extensions** view (`Ctrl+Shift+X` or `Cmd+Shift+X`).
3. Search for **PlatformIO IDE** and click **Install**.
4. Wait for the installation to finish and restart VS Code if prompted. You should see an "Alien" icon in the sidebar.

### 2. Prepare the Project
Clone or download this repository and open the folder in VS Code.
Ensure your web files (HTML, CSS, JS) are located inside a folder named `data`. PlatformIO uses the `data` folder to build the LittleFS filesystem image.

### 3. Compile and Upload the Code

1. Connect your ESP32 to your computer via USB.
2. Open the PlatformIO project.
3. Click the right arrow icon in the bottom status bar to compile and upload the project.

### 4. Build and Upload the Filesystem

The web files are not uploaded with the main code; they must be flashed to a separate partition.

1. Click on the PlatformIO icon on the sidebar.
2. Click Build Filesystem Image.
3. Once built, click Upload Filesystem Image.

### 5. Execute and Connect

1. Click the Serial Monitor icon (a plug shape) in the bottom VS Code status bar.
2. Press the EN or RST button on your ESP32 to reboot it.
3. Watch the Serial Monitor. Once it initializes, it will print out the ESP32's Access Point IP Address (usually 192.168.4.1).
4. On your computer or phone, connect to the WiFi network (AP) broadcasted by the ESP32.
5. Open a web browser and enter the IP address from the Serial Monitor into the URL bar.

You should now see the Web UI and start receiving live sensor data! owo