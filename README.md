# BLDC Motor Tester

A testing and analysis tool for BLDC motors using ESP32 and SimpleFOC. The project is under development.

## Related Repositories
- [BLDC_Tester_Hardware](https://github.com/ege-saygili/bldc-tester-hardware) - Hardware repository containing PCB designs and schematics

## Features
- Automated motor parameter detection
- Real-time motor monitoring via web interface
- Hall sensor validation
- Phase resistance and inductance measurement
- Input voltage monitoring

## Hardware Requirements
- Assembled PCB developed for this project (or a custom one)
- 3D printed case
- BLDC motor with hall sensors/encoder or no sensor
- Power supply (2s-13s LiPo/Li-ion, ensure battery C rating is sufficient for the intended load)
- A device with USB port to upload the code to ESP32 (like a ESP32-WROOM-32, ESP32-S3, etc.)
- A device to access the web interface (Laptop/PC/phone with web browser)

## Software Dependencies
- PlatformIO
- SimpleFOC library
- ESPAsyncWebServer
- AsyncTCP
- ArduinoJson

## Installation
1. Clone this repository
2. Open in PlatformIO
3. Install required libraries
4. Upload to ESP32

## Usage
1. Power up the system
2. Connect to the WiFi AP "BLDC-Tester"
3. Navigate to the web interface
4. Use the web interface to monitor/control the motor

## Configuration
-

## Development
- The project is developed using PlatformIO.
- The code is written in C++ and uses the SimpleFOC library.
- The web interface is created with ESPAsyncWebServer and uses a simple HTML template.

## Contributing
- Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## Contact
egesaygili42@gmail.com