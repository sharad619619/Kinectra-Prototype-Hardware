#ifndef KINECTRA_CONFIG_H
#define KINECTRA_CONFIG_H

#include <Arduino.h>

// ==========================================
// Network & Gateway Configuration
// ==========================================
#define WIFI_SSID             "Kinectra_Local_AP"
#define WIFI_PASSWORD         "biomechanics2026"
#define WS_SERVER_HOST        "192.168.1.100"
#define WS_SERVER_PORT        8000
#define WS_ENDPOINT           "/ws/hardware"

// ==========================================
// Hardware Pin Definitions
// ==========================================
#define PIN_I2C_SDA           21
#define PIN_I2C_SCL           22
#define PIN_MPU_INT           19
#define PIN_HAPTIC_MOTOR      18
#define PIN_LED_STATUS        2
#define PIN_LED_NETWORK       4
#define PIN_BATTERY_ADC       34

// ==========================================
// Sensor Telemetry Parameters
// ==========================================
#define IMU_SAMPLE_RATE_HZ    200
#define IMU_FIFO_PACKET_SIZE  42
#define MPU_ACCEL_SCALE_G     16.0f   // +/- 16g
#define MPU_GYRO_SCALE_DPS    2000.0f // +/- 2000 deg/s

// ==========================================
// Haptic Alert Durations
// ==========================================
#define HAPTIC_PULSE_SHORT_MS 80
#define HAPTIC_PULSE_WARN_MS  250
#define HAPTIC_PULSE_ALARM_MS 500

#endif // KINECTRA_CONFIG_H
