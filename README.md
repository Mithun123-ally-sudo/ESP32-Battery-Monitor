# ESP32-Based Battery Monitoring and Fault Logging System
https://wokwi.com/projects/479204729680711681

## Project Overview
This project simulates a battery monitoring system using an ESP32 in Wokwi. It accepts battery voltage, current, and temperature readings through the Serial Monitor, estimates the state of charge (SOC), detects abnormal conditions, and displays the readings and system status on a 16×2 I2C LCD.

## Objectives
- Monitor battery voltage, current, and temperature.
- Estimate battery state of charge (SOC).
- Detect abnormal operating conditions.
- Record fault events and maintain report counters.
- Validate normal, fault, and invalid-input scenarios through simulation.

## Hardware and Tools
- ESP32 microcontroller (simulated)
- 16×2 I2C LCD
- Wokwi online simulator
- Arduino C++
- Serial Monitor

## Features
- Interactive serial input for voltage, current, and temperature.
- Input validation to reject invalid or out-of-range values.
- Simplified voltage-based SOC estimation.
- LCD display of readings and system status.
- Detection of low voltage, high voltage, high temperature, and high current.
- RAM-based fault log storing up to 10 recent fault events.
- Fault counters and report tracking.

## Fault Detection Thresholds

| Condition | Threshold |
|---|---|
| Low voltage | Below 9.6 V |
| High voltage | Above 12.6 V |
| High temperature | Above 45°C |
| High current | Above 5 A |

## Testing and Results
The following scenarios were tested in the Wokwi simulation:

| Test | Input | Expected result |
|---|---|---|
| Normal operation | 12 V, 2 A, 30°C | Normal status |
| Low voltage | 9 V, 2 A, 30°C | Low-voltage warning |
| High voltage | 13 V, 2 A, 30°C | High-voltage warning |
| High temperature | 12 V, 2 A, 50°C | High-temperature warning |
| High current | 12 V, 6 A, 30°C | High-current warning |
| Invalid input | Text or out-of-range value | Input rejected |

All tests above were performed in simulation, not on a physical battery.

## Limitations
- SOC is estimated using a simplified voltage-based formula and is not calibrated for a specific battery chemistry.
- Fault events are stored in RAM and are lost when the ESP32 restarts.
- Wokwi simulation does not establish real-world battery safety or measurement accuracy.

## Future Improvements
- Add real voltage, current, and temperature sensors.
- Calibrate SOC estimation for a specific battery chemistry.
- Store fault history in non-volatile memory.
- Add battery protection and communication features.

## Author
Mithun V

## Project Type
Embedded Systems | Battery Monitoring | ESP32 | Arduino C++ | Wokwi Simulation
