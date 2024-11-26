#ifndef MOTOR_ANALYSIS_H
#define MOTOR_ANALYSIS_H

#include <Arduino.h>
#include <SimpleFOC.h>
#include "main.h"

// Declare external objects from main.cpp
extern BLDCMotor motor;
extern BLDCDriver3PWM driver;
extern HallSensor sensor;

struct MotorParameters {
    float phaseResistance;    // in ohms
    float phaseInductance;    // in henries
    int polePairs;            // number of pole pairs
    bool hallValid;           // hall sensor status
    float inputVoltage;        // Add this line
    float motorKv;    // Motor KV rating (RPM/V)
};

extern MotorParameters motorParams;

// Structure to hold motor health status
struct PhaseStatus {
    bool phaseA_OK;
    bool phaseB_OK;
    bool phaseC_OK;
    float phaseA_resistance;
    float phaseB_resistance;
    float phaseC_resistance;
};

struct HallStatus {
    bool hallA_OK;
    bool hallB_OK;
    bool hallC_OK;
    bool hallA_changing;
    bool hallB_changing;
    bool hallC_changing;
};

struct MotorHealth {
    PhaseStatus phases;
    HallStatus halls;
    bool inductanceOK;
    bool motorTemperatureOK;
    String errorMessage;
};

struct OpenLoopTestResult {
    bool success;
    float maxCurrent;
    float avgCurrent;
    bool currentLimitExceeded;
    bool hallsWorking;
    String errorMessage;
};

// Function declarations
MotorHealth checkMotorHealth();  // Returns detailed health status
bool measureMotorParameters();    // Original function for basic measurements
float measurePhaseResistance();
float measurePhaseInductance();
int detectPolePairs();
bool verifyHallSensors();
void checkPhaseConnections(PhaseStatus* phases);
OpenLoopTestResult runOpenLoopTest(float dutyCycle, uint32_t duration = 5000);
float getCurrentReading();
float measureInputVoltage();
float calculateMotorKv(float voltage, float rpm);

#endif