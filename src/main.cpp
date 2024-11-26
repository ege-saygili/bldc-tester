#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "main.h"
#include "webserver.h"
#include "motor_analysis.h"

// Global objects
BLDCMotor motor = BLDCMotor(7); // Default to 7 pole pairs
BLDCDriver3PWM driver = BLDCDriver3PWM(PIN_PWM_AH, PIN_PWM_BH, PIN_PWM_CH, PIN_EN_GATE);
HallSensor sensor = HallSensor(PIN_HALL_A, PIN_HALL_B, PIN_HALL_C, 7); // Default to 7 pole pairs

// Add these global variables at the top with other globals
unsigned long lastCheckTime = 0;
const unsigned long CHECK_INTERVAL = 1000; // Check every 1 second
bool isTestRunning = false;
bool isMeasuring = false;
unsigned long lastVoltageCheck = 0;
const unsigned long VOLTAGE_CHECK_INTERVAL = 1000; // Check every second

void setup() {
  Serial.begin(115200);
  
  // Initialize WiFi in AP mode
  WiFi.mode(WIFI_AP);
  WiFi.softAP("BLDC-Tester", "password123");
  
  // Initialize motor hardware
  driver.init();
  motor.linkDriver(&driver);
  
  // Default motor configuration
  motor.voltage_limit = 12;
  motor.sensor_direction = Direction::CW;
  
  // Initialize motor
  motor.init();
  
  // Setup web server
  setupWebServer();
  
  // Broadcast initial settings
  StaticJsonDocument<200> doc;
  doc["voltageLimit"] = motor.voltage_limit;
  doc["direction"] = (motor.sensor_direction == Direction::CW) ? "CW" : "CCW";
  String jsonString;
  serializeJson(doc, jsonString);
  broadcastJson(jsonString);
}

void loop() {
    // Basic FOC loop
    motor.loopFOC();

    // Current time
    unsigned long currentTime = millis();

    // Regular monitoring when not running tests
    if (!isTestRunning && !isMeasuring && (currentTime - lastCheckTime >= CHECK_INTERVAL)) {
        lastCheckTime = currentTime;
        
        // Get motor health status
        MotorHealth health = checkMotorHealth();
        
        // If there are issues, broadcast to web clients
        if (!health.phases.phaseA_OK || !health.phases.phaseB_OK || !health.phases.phaseC_OK ||
            !health.halls.hallA_OK || !health.halls.hallB_OK || !health.halls.hallC_OK) {
            
            // Create JSON with health status
            String json = "{\"health\":{" 
                         "\"phases\":{" 
                         "\"phaseA_OK\":" + String(health.phases.phaseA_OK ? "true" : "false") + ","
                         "\"phaseB_OK\":" + String(health.phases.phaseB_OK ? "true" : "false") + ","
                         "\"phaseC_OK\":" + String(health.phases.phaseC_OK ? "true" : "false") + "},"
                         "\"halls\":{" 
                         "\"hallA_OK\":" + String(health.halls.hallA_OK ? "true" : "false") + ","
                         "\"hallB_OK\":" + String(health.halls.hallB_OK ? "true" : "false") + ","
                         "\"hallC_OK\":" + String(health.halls.hallC_OK ? "true" : "false") + "},"
                         "\"errorMessage\":\"" + health.errorMessage + "\""
                         "}}";
            
            // Broadcast to all connected clients
            // We'll need to implement this in webserver.cpp
            broadcastJson(json);
        }

        // Monitor current
        float current = getCurrentReading();
        if (current > 0.1) { // Only send if there's significant current
            String json = "{\"current\":" + String(current, 3) + "}";
            broadcastJson(json);
        }
    }

    // Monitor voltage periodically
    if (currentTime - lastVoltageCheck >= VOLTAGE_CHECK_INTERVAL) {
        lastVoltageCheck = currentTime;
        float voltage = measureInputVoltage();
        
        // Create JSON for WebSocket
        String json = "{\"voltage\":" + String(voltage, 2) + "}";
        broadcastJson(json);
        
        // Update motor voltage limit if it changed significantly
        if (abs(motor.voltage_limit - voltage) > 0.5) {
            motor.voltage_limit = voltage;
        }
    }

    // Handle any pending web requests
    // This is handled by ESPAsyncWebServer automatically
}

void setupDriver() {
    // Power supply voltage
    driver.voltage_power_supply = readSupplyVoltage();
    
    // Driver configuration
    driver.pwm_frequency = 20000; // 20kHz
    
    // Initialize driver
    driver.init();
    
    // Link the motor to the driver
    motor.linkDriver(&driver);
}

void setupHallSensor() {
    // Initialize hall sensor hardware
    sensor.init();
    
    // Link sensor to motor
    motor.linkSensor(&sensor);
}

void setupMotor() {
    // Set motor parameters
    motor.voltage_limit = readSupplyVoltage(); // Set voltage limit to supply voltage
    motor.velocity_limit = 1000; // RPM - adjust as needed
    motor.sensor_direction = Direction::CW;
    
    // Initialize motor
    motor.init();
    motor.initFOC();
}

float readSupplyVoltage() {
    const int samples = 10;  // Average over 10 samples for stability
    float voltageSum = 0;
    
    for(int i = 0; i < samples; i++) {
        int adcValue = analogRead(PIN_VIN_SENSE);
        float voltage = (adcValue * 3.2f / ADC_MAX_VALUE) * VIN_SCALE_FACTOR;
        voltageSum += voltage;
        delay(1);
    }
    
    return voltageSum / samples;
}