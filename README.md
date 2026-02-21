# OpenWave

This is an attempt to make an open-source, modern(ish) digital audio player using commonly available parts today with a simple menu-based operating system shell.

## Firmware Build Instructions

This project uses [PlatformIO](https://platformio.org/) for firmware development. The project is being developed with an ESP32 Devkit V1 (esp32dev).

### Prerequisites
* [PlatformIO CLI](https://docs.platformio.org/en/latest/core/index.html)
* Or [PlatformIO IDE - VSCode](https://docs.platformio.org/en/latest/integration/ide/vscode.html)

### Building
1. Navigate to the firmware directory: `cd Firmware/Walkman_Firmware`
2. Resolve dependencies and build project: `pio run`

### Uploading
1. Connect the ESP32 board via USB and run: `pio run -t upload`
2. To monitor serial from the board: `pio device monitor`

## Hardware Used
All software tested on:
1. ESP32 Devkit V1
2. Adafruit VS1053b Breakout
3. SSD1306 OLED

## Licensing

This is an open-source hardware project. To ensure all aspects of the design are properly protected and freely shareable, the project uses multiple licenses:

* **Software/Firmware:** All code in this repository is licensed under the [MIT License](LICENSE).
* **Hardware/Electronics:** The KiCad schematics and PCB layout files are licensed under the [CERN Open Hardware Licence Version 2 - Strongly Reciprocal (CERN-OHL-S)](LICENSE-HARDWARE.txt).
* **Documentation & Enclosure:** All documentation and the 3D-printable enclosure models (STEP and STL files) are licensed under [Creative Commons Attribution-ShareAlike 4.0 International (CC-BY-SA 4.0)](LICENSE-DOCS.txt).
