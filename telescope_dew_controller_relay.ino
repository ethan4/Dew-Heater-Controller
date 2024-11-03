// https://github.com/ethan4/Dew-Heater-Controller

#include <Wire.h>
#include <WEMOS_SHT3X.h>

SHT3X sht30(0x45);  // Initialize SHT3X object with the custom I2C address (WEMOS SHT30 shield)

#define RELAY_PIN D0       // GPIO pin for controlling the dew heater relay
#define THERMISTOR_PIN A0  // Pin for the collector plate thermistor

bool isHeaterForcedOn = false;        // Variable to track if the relay is forced on
bool isManualMode = false;            // Variable to track if manual mode is enabled
float manualTargetTemperature = 0.0;  // Target temperature for manual mode

// Thermistor characteristics
const float BETA = 3950;                 // Beta parameter for the thermistor
const float THERMISTOR_NOMINAL = 10000;  // Resistance of thermistor at 25 degrees C
const float TEMPERATURE_NOMINAL = 25;    // Temperature for nominal resistance (in °C)
const int ADC_MAX = 1023;                // ADC value for 3.3V
const float R_FIXED = 10000.0;           // Fixed resistor value in the voltage divider (10KΩ)

// Averaging settings
const int AVERAGING_WINDOW_SIZE = 5;     // Number of readings to average
int adcReadings[AVERAGING_WINDOW_SIZE];  // Array to store ADC readings
int currentReadingIndex = 0;             // Current index for circular buffer
bool isBufferFilled = false;             // Flag to track if buffer has been filled once

// Hysteresis values
const float HYSTERESIS = 2.0;  // Hysteresis value in °C
float lastRelayStateTemp = 0;  // Last temperature when the relay state changed

void setup() {
  Serial.begin(115200);
  Wire.begin();                  // Initialize I2C
  pinMode(RELAY_PIN, OUTPUT);    // Set relay pin as output for the dew heater
  digitalWrite(RELAY_PIN, LOW);  // Initialize the relay to off
  delay(100);
}

float readThermistor() {
  // Read the raw ADC value and store it in the circular buffer
  int rawValue = analogRead(THERMISTOR_PIN);
  adcReadings[currentReadingIndex] = rawValue;
  currentReadingIndex = (currentReadingIndex + 1) % AVERAGING_WINDOW_SIZE;

  // Check if the buffer is filled
  if (currentReadingIndex == 0) {
    isBufferFilled = true;
  }

  // Calculate the average of the last readings
  int sum = 0;
  int count = isBufferFilled ? AVERAGING_WINDOW_SIZE : currentReadingIndex;
  for (int i = 0; i < count; i++) {
    sum += adcReadings[i];
  }
  float averagedRawValue = (float)sum / count;

  // Calculate the resistance of the thermistor based on the voltage divider
  float resistance = R_FIXED * (averagedRawValue / (ADC_MAX - averagedRawValue));

  // Calculate collector plate temperature using the Beta formula
  float temperature = 1.0 / ((log(resistance / THERMISTOR_NOMINAL) / BETA) + (1.0 / (TEMPERATURE_NOMINAL + 273.15)));
  return temperature - 273.15;  // Convert to Celsius
}

void loop() {
  // Check if the relay is forced on or if manual mode is active
  if (!isHeaterForcedOn && !isManualMode) {
    // Attempt to get the environmental sensor data
    int result = sht30.get();

    if (result == 0) {  // Successful reading from the SHT30 sensor
      // Retrieve ambient temperature and humidity
      float ambientTemperature = sht30.cTemp;
      float humidity = sht30.humidity;

      // Calculate dew point using the Magnus-Tetens formula
      float a = 17.62;
      float b = 243.12;
      float gamma = (a * ambientTemperature) / (b + ambientTemperature) + log(humidity / 100.0);
      float dewPoint = (b * gamma) / (a - gamma);

      // Print values for debugging
      Serial.print("Ambient Temperature: ");
      Serial.print(ambientTemperature);
      Serial.println(" °C");

      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.println(" %");

      Serial.print("Dew Point: ");
      Serial.print(dewPoint);
      Serial.println(" °C");

      // Read the collector plate temperature from the thermistor
      float collectorPlateTemperature = readThermistor();
      Serial.print("Collector Plate Temperature: ");
      Serial.print(collectorPlateTemperature);
      Serial.println(" °C");

      // Check if the thermistor is connected
      if (collectorPlateTemperature < -20) {  // If thermistor is -20°C or lower
        Serial.println("Thermistor not connected, using open-loop control.");
        // Open-loop control for dew heater
        if (ambientTemperature < dewPoint) {
          digitalWrite(RELAY_PIN, HIGH);  // Turn the dew heater relay on
          Serial.println("Dew Heater ON: Ambient temperature is below dew point.");
        } else {
          digitalWrite(RELAY_PIN, LOW);  // Turn the dew heater relay off
          Serial.println("Dew Heater OFF: Ambient temperature is above dew point.");
        }
      } else {  // Closed-loop control for the dew heater
        // Determine target temperature for the collector plate
        float targetTemperature = dewPoint + 5.0;

        // Control the dew heater relay based on the collector plate temperature
        if (collectorPlateTemperature < targetTemperature - HYSTERESIS) {
          digitalWrite(RELAY_PIN, HIGH);  // Turn the dew heater relay on
          lastRelayStateTemp = collectorPlateTemperature;
          Serial.println("Dew Heater ON: Collector plate temperature is below target.");
        } else if (collectorPlateTemperature > targetTemperature + HYSTERESIS) {
          digitalWrite(RELAY_PIN, LOW);  // Turn the dew heater relay off
          lastRelayStateTemp = collectorPlateTemperature;
          Serial.println("Dew Heater OFF: Collector plate temperature is above target.");
        }
      }
    } else {
      // Improved error handling for sensor readings
      Serial.print("Error reading data from SHT30 sensor. Error code: ");
      Serial.println(result);
      delay(500);  // Wait a bit before trying again
    }
  } else if (isManualMode) {
    // Manual control based on manualTargetTemperature
    float collectorPlateTemperature = readThermistor();
    Serial.print("Manual Mode - Collector Plate Temperature: ");
    Serial.print(collectorPlateTemperature);
    Serial.println(" °C");

    if (collectorPlateTemperature < manualTargetTemperature - HYSTERESIS) {
      digitalWrite(RELAY_PIN, HIGH);  // Turn the dew heater relay on
      Serial.println("Dew Heater ON: Collector plate temperature is below manual setpoint.");
    } else if (collectorPlateTemperature > manualTargetTemperature + HYSTERESIS) {
      digitalWrite(RELAY_PIN, LOW);  // Turn the dew heater relay off
      Serial.println("Dew Heater OFF: Collector plate temperature is above manual setpoint.");
    }
  }

  // Check for serial input to manually control the dew heater relay or set manual mode
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');  // Read the command until newline
    if (command.equals("on")) {                     // 'on' to turn dew heater relay ON
      digitalWrite(RELAY_PIN, HIGH);
      isHeaterForcedOn = true;  // Set the state to forced on
      Serial.println("Dew Heater forced ON via serial command.");
    } else if (command.equals("off")) {  // 'off' to turn dew heater relay OFF
      digitalWrite(RELAY_PIN, LOW);
      isHeaterForcedOn = false;  // Reset the state to allow dew point checking
      Serial.println("Dew Heater forced OFF via serial command.");
    } else if (command.startsWith("manual ")) {  // 'manual <temp>' to enter manual mode with a target temperature
      manualTargetTemperature = command.substring(7).toFloat();
      isManualMode = true;
      isHeaterForcedOn = false;  // Ensure forced mode is off
      Serial.print("Manual mode enabled. Target temperature set to ");
      Serial.print(manualTargetTemperature);
      Serial.println(" °C");
    } else if (command.equals("auto")) {  // 'auto' to return to automatic control
      isManualMode = false;
      Serial.println("Returning to automatic mode.");
    }
  }

  delay(2000);  // Wait before the next reading
}
