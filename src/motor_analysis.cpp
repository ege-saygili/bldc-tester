#include "motor_analysis.h"
#include "main.h"
// Add these function prototypes
void checkHallSensors(HallStatus* halls);
void validateHallPatterns(HallStatus* halls);
String getPhaseError(float resistance);
String getHallError(bool changing);
float getCurrentReading();
float measureInputVoltage();

// Global variables for measurement results
MotorParameters motorParams = {
    .phaseResistance = 0.0,
    .phaseInductance = 0.0,
    .polePairs = 0,
    .hallValid = false
};

bool measureMotorParameters() {
    // Disable FOC control during measurement
    motor.disable();
    delay(500);

    // Measure phase resistance
    motorParams.phaseResistance = measurePhaseResistance();
    
    // Check if resistance is within reasonable bounds
    if (motorParams.phaseResistance < 0.01 || motorParams.phaseResistance > 100.0) {
        return false;
    }

    // Measure inductance (simplified method)
    motorParams.phaseInductance = measurePhaseInductance();
    
    // Detect pole pairs using hall sensors
    motorParams.polePairs = detectPolePairs();
    
    // Verify hall sensor functionality
    motorParams.hallValid = verifyHallSensors();

    // Measure input voltage
    motorParams.inputVoltage = measureInputVoltage();
    
    // Update motor voltage limit
    motor.voltage_limit = motorParams.inputVoltage;
    
    return true;
}

float measurePhaseResistance() {
    const float testVoltage = 0.5; // Reduced to 0.5V for safer testing
    const int samples = 100;
    
    // Configure ADC for current sensing
    analogReadResolution(12); // ESP32 12-bit ADC
    analogSetAttenuation(ADC_11db); // For 3.3V range
    
    // Apply test voltage to phase A
    driver.setPwm(testVoltage, 0, 0);
    delay(100); // Wait for current to stabilize
    
    // Sample current through ADC
    float totalCurrent = 0;
    for(int i = 0; i < samples; i++) {
        int adcValue = analogRead(CURRENT_SENSE_PIN);
        float voltage = (adcValue * 3.3) / 4095.0; // Convert to voltage
        float current = voltage / CURRENT_SENSE_RATIO; // Convert to amperes
        totalCurrent += current;
        delay(1);
    }
    float averageCurrent = totalCurrent / samples;
    
    // Disable output
    driver.setPwm(0, 0, 0);
    
    // R = V/I
    if(averageCurrent < 0.001) { // Avoid division by zero
        return 0.0; // Indicates an error condition
    }
    
    float resistance = testVoltage / averageCurrent;
    
    // Sanity check the measured resistance
    if (resistance < 0.01 || resistance > 100.0) {
        return 0.0; // Indicates an error condition
    }
    
    return resistance;
}

float measurePhaseInductance() {
    // Simplified inductance measurement
    // This is a placeholder - proper implementation would require
    // voltage step response analysis
    return 0.001; // Default value in Henries
}

int detectPolePairs() {
    // Initialize variables to store hall sensor states
    int hallState1, hallState2;
    int transitions = 0;
    unsigned long startTime = millis();
    const unsigned long timeout = 5000; // 5 second timeout
    
    // Apply a small voltage to slowly rotate the motor
    motor.move(1.0); // Small constant voltage
    delay(100);
    
    // Get initial hall state
    hallState1 = (digitalRead(HALL_A) << 2) | 
                 (digitalRead(HALL_B) << 1) | 
                  digitalRead(HALL_C);
    
    // Count hall state transitions
    while ((millis() - startTime) < timeout) {
        hallState2 = (digitalRead(HALL_A) << 2) | 
                    (digitalRead(HALL_B) << 1) | 
                     digitalRead(HALL_C);
        
        if (hallState1 != hallState2) {
            transitions++;
            hallState1 = hallState2;
        }
        
        // One mechanical rotation should give us 6 hall state transitions
        // per pole pair
        if (transitions >= 6) {
            break;
        }
        delay(1);
    }
    
    // Stop the motor
    motor.move(0);
    
    // Calculate pole pairs based on transitions
    // In one mechanical rotation, we should see 6 transitions per pole pair
    if (transitions == 0) return 7; // Default if detection fails
    
    return transitions / 6;
}

bool verifyHallSensors() {
    unsigned long startTime = millis();
    const unsigned long timeout = 1000; // 1 second timeout
    bool hallAChanged = false;
    bool hallBChanged = false;
    bool hallCChanged = false;
    
    // Store initial states
    bool hallAState = digitalRead(HALL_A);
    bool hallBState = digitalRead(HALL_B);
    bool hallCState = digitalRead(HALL_C);
    
    // Apply a small voltage to rotate the motor slowly
    motor.move(1.0);
    
    // Monitor for changes in hall sensor states
    while ((millis() - startTime) < timeout) {
        if (digitalRead(HALL_A) != hallAState) hallAChanged = true;
        if (digitalRead(HALL_B) != hallBState) hallBChanged = true;
        if (digitalRead(HALL_C) != hallCState) hallCChanged = true;
        
        // If all sensors have changed, we're done
        if (hallAChanged && hallBChanged && hallCChanged) {
            motor.move(0);
            return true;
        }
        delay(1);
    }
    
    // Stop the motor
    motor.move(0);
    
    return false;
}

MotorHealth checkMotorHealth() {
    MotorHealth health = {
        .phases = {true, true, true, 0.0, 0.0, 0.0},
        .halls = {true, true, true, false, false, false},
        .inductanceOK = false,
        .motorTemperatureOK = true,
        .errorMessage = ""
    };
    
    // Check each phase individually
    checkPhaseConnections(&health.phases);
    
    // Check hall sensors individually
    checkHallSensors(&health.halls);
    
    // Build detailed error message
    if (!health.phases.phaseA_OK) {
        health.errorMessage += "Phase A: " + getPhaseError(health.phases.phaseA_resistance) + ". ";
    }
    if (!health.phases.phaseB_OK) {
        health.errorMessage += "Phase B: " + getPhaseError(health.phases.phaseB_resistance) + ". ";
    }
    if (!health.phases.phaseC_OK) {
        health.errorMessage += "Phase C: " + getPhaseError(health.phases.phaseC_resistance) + ". ";
    }
    
    if (!health.halls.hallA_OK) {
        health.errorMessage += "Hall A: " + getHallError(health.halls.hallA_changing) + ". ";
    }
    if (!health.halls.hallB_OK) {
        health.errorMessage += "Hall B: " + getHallError(health.halls.hallB_changing) + ". ";
    }
    if (!health.halls.hallC_OK) {
        health.errorMessage += "Hall C: " + getHallError(health.halls.hallC_changing) + ". ";
    }
    
    // If no errors found
    if (health.errorMessage.length() == 0) {
        health.errorMessage = "All systems operational";
    }
    
    return health;
}

String getPhaseError(float resistance) {
    if (resistance < 0.01) {
        return "Possible short circuit";
    } else if (resistance > 100.0) {
        return "Possible open circuit";
    } else if (resistance == 0.0) {
        return "No connection detected";
    }
    return "Unknown error";
}

String getHallError(bool changing) {
    if (!changing) {
        return "No state changes detected - sensor may be stuck";
    }
    return "Invalid signal pattern detected";
}

void checkPhaseConnections(PhaseStatus* phases) {
    const float testVoltage = 0.5;  // Small test voltage
    
    // Disable motor control
    motor.disable();
    delay(100);
    
    // Test each phase
    for (int phase = 0; phase < 3; phase++) {
        float current = 0;
        
        // Apply voltage to single phase
        switch (phase) {
            case 0:
                driver.setPwm(testVoltage, 0, 0);
                break;
            case 1:
                driver.setPwm(0, testVoltage, 0);
                break;
            case 2:
                driver.setPwm(0, 0, testVoltage);
                break;
        }
        
        delay(50);  // Wait for current to stabilize
        
        // Measure current
        current = getCurrentReading();
        
        // Calculate resistance
        float resistance = testVoltage / current;
        
        // Store resistance and check if it's within acceptable range
        switch (phase) {
            case 0:
                phases->phaseA_resistance = resistance;
                phases->phaseA_OK = (resistance >= 0.01 && resistance <= 100.0);
                break;
            case 1:
                phases->phaseB_resistance = resistance;
                phases->phaseB_OK = (resistance >= 0.01 && resistance <= 100.0);
                break;
            case 2:
                phases->phaseC_resistance = resistance;
                phases->phaseC_OK = (resistance >= 0.01 && resistance <= 100.0);
                break;
        }
        
        // Reset output
        driver.setPwm(0, 0, 0);
        delay(50);
    }
}

void checkHallSensors(HallStatus* halls) {
    unsigned long startTime = millis();
    const unsigned long timeout = 1000; // 1 second timeout
    
    // Store initial states
    bool hallAState = digitalRead(HALL_A);
    bool hallBState = digitalRead(HALL_B);
    bool hallCState = digitalRead(HALL_C);
    
    // Apply a small voltage to rotate the motor slowly
    motor.move(1.0);
    
    // Monitor for changes in hall sensor states
    while ((millis() - startTime) < timeout) {
        // Check for state changes
        halls->hallA_changing |= (digitalRead(HALL_A) != hallAState);
        halls->hallB_changing |= (digitalRead(HALL_B) != hallBState);
        halls->hallC_changing |= (digitalRead(HALL_C) != hallCState);
        
        delay(1);
    }
    
    // Stop the motor
    motor.move(0);
    
    // Validate hall sensor operation
    halls->hallA_OK = halls->hallA_changing;
    halls->hallB_OK = halls->hallB_changing;
    halls->hallC_OK = halls->hallC_changing;
    
    // Check for valid hall patterns
    validateHallPatterns(halls);
}

void validateHallPatterns(HallStatus* halls) {
    // Valid hall patterns (60° spacing)
    const uint8_t validPatterns[] = {1, 3, 2, 6, 4, 5};
    bool validPatternFound = false;
    
    // Read current hall state
    uint8_t hallState = (digitalRead(HALL_A) << 2) | 
                       (digitalRead(HALL_B) << 1) | 
                        digitalRead(HALL_C);
    
    // Check if current state is valid
    for (int i = 0; i < 6; i++) {
        if (hallState == validPatterns[i]) {
            validPatternFound = true;
            break;
        }
    }
    
    // Update hall status if pattern is invalid
    if (!validPatternFound) {
        halls->hallA_OK = false;
        halls->hallB_OK = false;
        halls->hallC_OK = false;
    }
}

float getCurrentReading() {
    const int samples = 10;
    float totalCurrent = 0;
    
    for (int i = 0; i < samples; i++) {
        int adcValue = analogRead(CURRENT_SENSE_PIN);
        float voltage = (adcValue * 3.3) / 4095.0;
        float current = voltage / CURRENT_SENSE_RATIO;
        totalCurrent += current;
        delay(1);
    }
    
    return totalCurrent / samples;
}

float measureInputVoltage() {
    const int samples = 10;  // Number of samples for averaging
    float voltageSum = 0;
    
    for(int i = 0; i < samples; i++) {
        // Read ADC value
        int adcValue = analogRead(VOLTAGE_SENSE_PIN);
        
        // Convert ADC reading to voltage
        float measuredVoltage = (adcValue / ADC_RESOLUTION) * ADC_REFERENCE;
        
        // Calculate actual voltage using voltage divider formula
        float inputVoltage = measuredVoltage * (VOLTAGE_DIVIDER_R1 + VOLTAGE_DIVIDER_R2) / VOLTAGE_DIVIDER_R2;
        
        voltageSum += inputVoltage;
        delay(1);
    }
    
    return voltageSum / samples;
}

OpenLoopTestResult runOpenLoopTest(float dutyCycle, uint32_t duration) {
    OpenLoopTestResult result = {
        .success = false,
        .maxCurrent = 0.0,
        .avgCurrent = 0.0,
        .currentLimitExceeded = false,
        .hallsWorking = false,
        .errorMessage = ""
    };
    
    const float CURRENT_LIMIT = 10.0;  // 10A max current
    const float MIN_DUTY = 0.0;
    const float MAX_DUTY = 0.5;        // 50% max duty cycle for safety
    
    // Validate duty cycle
    dutyCycle = constrain(dutyCycle, MIN_DUTY, MAX_DUTY);
    
    // Variables for current monitoring
    float currentSum = 0.0;
    int samples = 0;
    
    // Variables for hall sensor monitoring
    bool hallAChanged = false;
    bool hallBChanged = false;
    bool hallCChanged = false;
    uint8_t initialHallState = (digitalRead(HALL_A) << 2) | 
                              (digitalRead(HALL_B) << 1) | 
                              digitalRead(HALL_C);
    
    // Start time
    unsigned long startTime = millis();
    
    // Disable FOC control
    motor.disable();
    delay(100);
    
    // Apply voltage using simple 6-step commutation
    while ((millis() - startTime) < duration) {
        // Read current
        float current = getCurrentReading();
        currentSum += current;
        samples++;
        
        // Update max current
        if (current > result.maxCurrent) {
            result.maxCurrent = current;
        }
        
        // Check for current limit
        if (current > CURRENT_LIMIT) {
            result.currentLimitExceeded = true;
            result.errorMessage = "Current limit exceeded: " + String(current, 2) + "A";
            motor.disable();
            return result;
        }
        
        // Monitor hall sensors
        uint8_t hallState = (digitalRead(HALL_A) << 2) | 
                           (digitalRead(HALL_B) << 1) | 
                           digitalRead(HALL_C);
                           
        if ((hallState >> 2) != (initialHallState >> 2)) hallAChanged = true;
        if (((hallState >> 1) & 1) != ((initialHallState >> 1) & 1)) hallBChanged = true;
        if ((hallState & 1) != (initialHallState & 1)) hallCChanged = true;
        
        // Simple 6-step commutation based on hall state
        switch (hallState) {
            case 1: // 001
                driver.setPwm(dutyCycle, 0, 0);
                break;
            case 3: // 011
                driver.setPwm(0, dutyCycle, 0);
                break;
            case 2: // 010
                driver.setPwm(0, dutyCycle, 0);
                break;
            case 6: // 110
                driver.setPwm(0, 0, dutyCycle);
                break;
            case 4: // 100
                driver.setPwm(0, 0, dutyCycle);
                break;
            case 5: // 101
                driver.setPwm(dutyCycle, 0, 0);
                break;
            default:
                // Invalid hall state
                result.errorMessage = "Invalid hall state detected: " + String(hallState);
                motor.disable();
                return result;
        }
        
        delay(1);
    }
    
    // Stop motor
    motor.disable();
    
    // Calculate average current
    result.avgCurrent = currentSum / samples;
    
    // Check if all hall sensors changed state
    result.hallsWorking = hallAChanged && hallBChanged && hallCChanged;
    
    // Set success status
    result.success = true;
    if (!result.hallsWorking) {
        result.errorMessage = "Not all hall sensors changed state. ";
        if (!hallAChanged) result.errorMessage += "Hall A stuck. ";
        if (!hallBChanged) result.errorMessage += "Hall B stuck. ";
        if (!hallCChanged) result.errorMessage += "Hall C stuck. ";
    }
    
    return result;
}

float calculateMotorKv(float voltage, float rpm) {
    if (voltage <= 0) return 0;
    return rpm / voltage;
}