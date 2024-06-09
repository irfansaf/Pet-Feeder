## Pet Feeder
This project is for Fablab Innovation Tech Competition.

This project implements an automated pet feeder system using Arduino, MQTT, and a stepper motor. The system allows pet owners to schedule feeding times, adjust the dispensing level, and manually trigger food dispensing via MQTT messages.

### Features:
- **Scheduled Feeding:** Set feeding times for your pet throughout the day.
- **Adjustable Dispensing Level:** Customize the amount of food dispensed based on your pet's needs.
- **Manual Feeding Trigger:** Trigger feeding manually via MQTT messages for immediate feeding.

### Hardware Components:
- **Arduino Board:** Controls the system logic and interacts with sensors and actuators.
- **Stepper Motor:** Dispenses food based on the specified level.
- **Ultrasonic Sensor:** Detects the presence of objects (e.g., pets) near the feeder.
- **RTC Module:** Keeps track of time for scheduling feeding times accurately.
- **WiFi Module:** Enables communication with MQTT broker for remote control.

### Dependencies:
- **Arduino Libraries:**
  - `ThreeWire`: Interface library for DS1302 RTC module.
  - `RtcDS1302`: Library for DS1302 RTC module.
  - `WiFiManager`: Simplifies the process of connecting Arduino to WiFi networks.
  - `NTPClient`: Retrieves time from NTP servers.
  - `WiFiUdp`: UDP library for WiFi communication.
  - `PubSubClient`: MQTT client library for Arduino.

### Setup:
1. Connect the hardware components according to the specified pin configurations.
2. Upload the Arduino sketch provided in this repository to your Arduino board.
3. Ensure the MQTT broker is accessible and configured correctly.
4. Power on the system and monitor the serial output for debugging and status information.

### Usage:
1. Set the dispensing level and feeding schedule via MQTT messages or directly in the Arduino sketch.
2. Monitor the feeding times and system status through the serial monitor.
3. Trigger manual feeding by publishing MQTT messages to the designated topics.
4. Customize and expand the functionality as needed for your pet's requirements.

### Contributors:
- [Irfan Saf](https://github.com/irfansaf): Project creator and main contributor.

### License:
This project is licensed under the [MIT License](LICENSE).
