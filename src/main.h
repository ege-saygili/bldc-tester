#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <SimpleFOC.h>

// LED (LED pins)
#define PIN_LED1 2
#define PIN_LED2 15

// DRV8302 PWM Interface (PWM pins)
#define PIN_PWM_AH 23  // High-side Phase A
#define PIN_PWM_AL 22  // Low-side Phase A
#define PIN_PWM_BH 21  // High-side Phase B
#define PIN_PWM_BL 19  // Low-side Phase B
#define PIN_PWM_CH 18  // High-side Phase C
#define PIN_PWM_CL 5   // Low-side Phase C

// DRV8302 Control/Status (Control pins)
#define PIN_EN_GATE 13  // Enable gate drivers
#define PIN_FAULT   14  // Fault indicator (input only)
#define PIN_DC_CAL  4   // DC calibration

// Hall Sensor Inputs (Input only pins)
#define PIN_HALL_A    // Changed from 39 (SENSOR_VP)
#define PIN_HALL_B    // SVN
#define PIN_HALL_C    // SVP

// Voltage/Current Sensing (ADC pins)
#define PIN_VA_SENSE 35  // Phase A voltage sensing
#define PIN_VB_SENSE 32  // Phase B voltage sensing
#define PIN_VC_SENSE 33  // Phase C voltage sensing
#define PIN_I_SENSE1 39  // Low-side current sense 1
#define PIN_I_SENSE2 34  // Low-side current sense 2

// Input Voltage Sensing (ADC pin)
#define PIN_VIN_SENSE 36  // Input voltage sensing ADC pin

// ADC conversion constants for input voltage 
#define VIN_SCALE_FACTOR (60.0f / 3.2f)  // For 60V max input scaled to 3.2V
#define ADC_BITS        12                // ESP32 ADC resolution
#define ADC_MAX_VALUE   ((1 << ADC_BITS) - 1)  // 4095 for 12-bit

// Motor driver instance
extern BLDCDriver3PWM driver;
extern BLDCMotor motor;
extern HallSensor sensor;

// Function declarations
void setupMotor();
void setupDriver();
void setupHallSensor();
float readSupplyVoltage();

#endif // MAIN_H 