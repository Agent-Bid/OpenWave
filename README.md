This is an attempt to make an open source, modern(ish) walkman using commonly available parts today with a simple menu based operating system shell

Firmware Build Instructions:
This project uses [PlatformIO](<https://platformio.org/>) for firmware development, the project is being developement with an ESP32 Devkit V1 (esp32dev).

Prerequisites:
[PlatformIO CLI](<https://docs.platformio.org/en/latest/core/index.html>)
Or
[PlatfirmIO IDE - VSCode](<https://docs.platformio.org/en/latest/integration/ide/vscode.html>)

Building:

1. Navigate to the firmware directory: `cd Firmware/Walkman_Firmware`

2. Resolve dependencies and build project: `pio run`

Uploading:

1. Connect the ESP32 board via usb and run: `pio run --t upload`

2. To monitor serial from the board: `pio device monitor`
