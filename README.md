# Dew Heater Controller for Schmidt-Cassegrain Telescope

This Arduino sketch is designed to manage a dew heater ring attached to a Schmidt-Cassegrain telescope. The dew heater prevents dew formation on the telescope's corrector plate by heating it based on ambient humidity and temperature conditions. The system can operate in automatic mode, which adjusts heating to keep the corrector plate above the dew point, or in manual mode, where the user sets a target temperature.

This project uses a WEMOS SHT30 environmental sensor for ambient temperature and humidity, along with a thermistor attached to the corrector plate to monitor its temperature. The relay controller switches the dew heater on or off based on the control algorithm's decision.

---

## Hardware Requirements
1. Arduino board with I2C and ADC capabilities.
2. WEMOS SHT30 sensor to measure ambient temperature and humidity.
3. Thermistor attached to the telescope's corrector plate to monitor its temperature.
4. Relay module for switching the dew heater on or off.

## Circuit Connections
- **SHT30 Sensor:** Connect to I2C pins.
- **Relay Module:** Connect the relay to `D0` pin.
- **Thermistor:** Connect to analog pin `A0`.

## Software Overview

The code supports two main modes:
1. **Automatic Mode (Closed-Loop Control):** In this mode, the dew heater adjusts based on ambient conditions and the corrector plate temperature.
2. **Manual Mode:** The user can set a target temperature, and the system will maintain that temperature using the thermistor reading from the corrector plate.

The system also includes a forced "on/off" feature for direct control of the relay.

### Thermistor Calculation
Thermistor values are averaged over several readings for stability, then converted to temperature using the Beta formula. This temperature is used for closed-loop control in automatic mode.

### Dew Point Calculation
The dew point is calculated using the Magnus-Tetens formula, which relies on ambient temperature and humidity readings from the SHT30 sensor. The system will attempt to keep the corrector plate temperature above the dew point to prevent dew formation.

### Hysteresis
A hysteresis of ±2°C is applied to the target temperature to prevent the relay from switching too frequently, extending the relay's lifespan.

---

## Usage Instructions

1. **Automatic Mode:** By default, the system starts in automatic mode. It will check the ambient temperature and humidity, calculate the dew point, and use the thermistor reading to control the dew heater.
2. **Manual Mode:** Enter manual mode by sending a command through the serial interface. In manual mode, the user sets a target temperature for the corrector plate.

---

## Serial Commands

The system responds to serial commands for various controls:

- **`on`** – Forces the dew heater relay to turn on, regardless of temperature readings.
- **`off`** – Forces the dew heater relay to turn off, regardless of temperature readings.
- **`manual <temp>`** – Enables manual mode and sets the target temperature for the corrector plate to `<temp>`. For example, `manual 20` sets the target temperature to 20°C.
- **`auto`** – Returns the system to automatic mode, where it controls the heater based on dew point and plate temperature.

## Control Logic

- **Open-Loop Control:** If the thermistor reports a temperature lower than -20°C, it's assumed disconnected, and the system uses an open-loop approach by comparing ambient temperature to the dew point.
- **Closed-Loop Control:** When the thermistor is functioning correctly, the system uses closed-loop control to maintain a target temperature 5°C above the dew point.

## Example Serial Output
```plaintext
Ambient Temperature: 15.00 °C
Humidity: 85.00 %
Dew Point: 12.13 °C
Collector Plate Temperature: 13.00 °C
Dew Heater ON: Collector plate temperature is below target.
```

---

## Notes

- **Credits:** ChatGPT, an AI developed by OpenAI, assisted in creating this code and documentation. The solution is also inspired by various concepts and formulas available in public training datasets.
