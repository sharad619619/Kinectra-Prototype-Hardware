/**
 * ============================================================================
 * Kinectra Sports Biomechanics — Edge Hardware Firmware
 * Microcontroller: ESP32-WROOM-32D
 * Authors: Sharad Singh (@sharad619619)
 * Description: High-speed 200Hz 6-DOF IMU motion ingestion, DMP processing,
 *              sensor-fusion telemetry streaming via WebSockets, and real-time
 *              tactile feedback triggering.
 * ============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include "config.h"

// Hardware Objects
MPU6050 mpu;
WebSocketsClient webSocket;

// FreeRTOS Task Handles
TaskHandle_t TaskIMUHandle;
TaskHandle_t TaskNetworkHandle;
QueueHandle_t telemetryQueue;

// MPU Control & Status Variables
bool dmpReady = false;
uint8_t mpuIntStatus;
uint8_t devStatus;
uint16_t packetSize;
uint16_t fifoCount;
uint8_t fifoBuffer[64];

// Orientation & Motion Containers
Quaternion q;           // [w, x, y, z] quaternion container
VectorInt16 aa;         // [x, y, z] accel sensor measurements
VectorInt16 aaReal;     // [x, y, z] gravity-free accel measurements
VectorInt16 aaWorld;    // [x, y, z] world-frame accel measurements
VectorFloat gravity;    // [x, y, z] gravity vector
float ypr[3];           // [yaw, pitch, roll] in radians

// Struct for inter-task communication
struct TelemetryFrame {
    uint32_t timestamp_ms;
    float yaw;
    float pitch;
    float roll;
    float accel_x_g;
    float accel_y_g;
    float accel_z_g;
    float gyro_x_dps;
    float gyro_y_dps;
    float gyro_z_dps;
    float total_g;
    bool impact_detected;
};

volatile bool mpuInterrupt = false;
void IRAM_ATTR dmpDataReady() {
    mpuInterrupt = true;
}

// ============================================================================
// Haptic Alert Driver
// ============================================================================
void triggerHapticFeedback(uint16_t duration_ms) {
    digitalWrite(PIN_HAPTIC_MOTOR, HIGH);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    digitalWrite(PIN_HAPTIC_MOTOR, LOW);
}

// ============================================================================
// WebSocket Event Handler (Core 1)
// ============================================================================
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            digitalWrite(PIN_LED_NETWORK, LOW);
            Serial.println("[WS] Disconnected from Biomechanics Gateway");
            break;
        case WStype_CONNECTED:
            digitalWrite(PIN_LED_NETWORK, HIGH);
            Serial.printf("[WS] Connected to url: %s\n", payload);
            break;
        case WStype_TEXT: {
            // Parse inbound feedback commands from backend/AI
            StaticJsonDocument<256> doc;
            DeserializationError err = deserializeJson(doc, payload, length);
            if (!err) {
                const char* command = doc["cmd"] | "";
                if (strcmp(command, "HAPTIC_PULSE") == 0) {
                    uint16_t duration = doc["duration_ms"] | HAPTIC_PULSE_WARN_MS;
                    triggerHapticFeedback(duration);
                }
            }
            break;
        }
        case WStype_BIN:
        case WStype_ERROR:
        default:
            break;
    }
}

// ============================================================================
// Task 1: High-Speed IMU Data Ingestion (Pinned to Core 0)
// ============================================================================
void TaskIMU(void * pvParameters) {
    Serial.println("[Core 0] Starting 200Hz IMU Acquisition Task...");
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(5); // 5ms = 200 Hz

    for(;;) {
        if (dmpReady && mpuInterrupt) {
            mpuInterrupt = false;
            mpuIntStatus = mpu.getIntStatus();
            fifoCount = mpu.getFIFOCount();

            if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
                mpu.resetFIFO();
                Serial.println("[IMU] FIFO Overflow detected and reset");
            } else if (mpuIntStatus & 0x02) {
                while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();
                mpu.getFIFOBytes(fifoBuffer, packetSize);
                fifoCount -= packetSize;

                // Decode DMP orientation & gravity vectors
                mpu.dmpGetQuaternion(&q, fifoBuffer);
                mpu.dmpGetGravity(&gravity, &q);
                mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
                mpu.dmpGetAccel(&aa, fifoBuffer);
                mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);

                TelemetryFrame frame;
                frame.timestamp_ms = millis();
                frame.yaw = ypr[0] * 180.0f / M_PI;
                frame.pitch = ypr[1] * 180.0f / M_PI;
                frame.roll = ypr[2] * 180.0f / M_PI;

                // Conversion to G and deg/s
                frame.accel_x_g = (float)aaReal.x / 2048.0f; // Scale factor for +/- 16g
                frame.accel_y_g = (float)aaReal.y / 2048.0f;
                frame.accel_z_g = (float)aaReal.z / 2048.0f;
                frame.total_g = sqrtf(frame.accel_x_g * frame.accel_x_g +
                                      frame.accel_y_g * frame.accel_y_g +
                                      frame.accel_z_g * frame.accel_z_g);

                // Detect bat impact spike or sudden bowling arm release deceleration
                frame.impact_detected = (frame.total_g > 6.5f);

                // Send frame to non-blocking network queue
                xQueueOverwrite(telemetryQueue, &frame);
            }
        }
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// ============================================================================
// Task 2: Network & WebSocket Stream (Pinned to Core 1)
// ============================================================================
void TaskNetwork(void * pvParameters) {
    Serial.println("[Core 1] Starting WebSocket Network Task...");
    
    TelemetryFrame currentFrame;
    StaticJsonDocument<384> doc;
    char jsonBuffer[384];

    for(;;) {
        webSocket.loop();

        if (xQueueReceive(telemetryQueue, &currentFrame, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (webSocket.isConnected()) {
                doc.clear();
                doc["node_id"] = "kinectra-node-alpha";
                doc["ts"] = currentFrame.timestamp_ms;
                
                JsonObject euler = doc.createNestedObject("euler");
                euler["y"] = round(currentFrame.yaw * 10.0f) / 10.0f;
                euler["p"] = round(currentFrame.pitch * 10.0f) / 10.0f;
                euler["r"] = round(currentFrame.roll * 10.0f) / 10.0f;

                JsonObject acc = doc.createNestedObject("acc");
                acc["x"] = round(currentFrame.accel_x_g * 100.0f) / 100.0f;
                acc["y"] = round(currentFrame.accel_y_g * 100.0f) / 100.0f;
                acc["z"] = round(currentFrame.accel_z_g * 100.0f) / 100.0f;
                acc["mag"] = round(currentFrame.total_g * 100.0f) / 100.0f;

                doc["impact"] = currentFrame.impact_detected;

                serializeJson(doc, jsonBuffer);
                webSocket.sendTXT(jsonBuffer);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

// ============================================================================
// Main Initialization & Setup
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=== KINECTRA HARDWARE NODE V1.0 ===");

    // Pin Configurations
    pinMode(PIN_LED_STATUS, OUTPUT);
    pinMode(PIN_LED_NETWORK, OUTPUT);
    pinMode(PIN_HAPTIC_MOTOR, OUTPUT);
    pinMode(PIN_MPU_INT, INPUT);

    digitalWrite(PIN_LED_STATUS, HIGH);
    digitalWrite(PIN_HAPTIC_MOTOR, LOW);

    // Initialize I2C Bus @ 400kHz Fast Mode
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);

    // Initialize MPU-6050
    Serial.println("[Hardware] Initializing MPU-6050...");
    mpu.initialize();
    devStatus = mpu.dmpInitialize();

    // Factory calibration offsets
    mpu.setXAccelOffset(-2850);
    mpu.setYAccelOffset(1420);
    mpu.setZAccelOffset(1780);
    mpu.setXGyroOffset(42);
    mpu.setYGyroOffset(-18);
    mpu.setZGyroOffset(12);

    if (devStatus == 0) {
        mpu.CalibrateAccel(6);
        mpu.CalibrateGyro(6);
        mpu.setDMPEnabled(true);

        attachInterrupt(digitalPinToInterrupt(PIN_MPU_INT), dmpDataReady, RISING);
        mpuIntStatus = mpu.getIntStatus();
        packetSize = mpu.dmpGetFIFOPacketSize();
        dmpReady = true;
        Serial.println("[Hardware] DMP calibrated and armed @ 200 Hz.");
    } else {
        Serial.printf("[Hardware] DMP Initialization failed (code %d)\n", devStatus);
    }

    // Connect to WiFi
    Serial.printf("[Network] Connecting to WiFi SSID: %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
        delay(300);
        digitalWrite(PIN_LED_STATUS, !digitalRead(PIN_LED_STATUS));
        retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[Network] WiFi Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        webSocket.begin(WS_SERVER_HOST, WS_SERVER_PORT, WS_ENDPOINT);
        webSocket.onEvent(webSocketEvent);
        webSocket.setReconnectInterval(2000);
    } else {
        Serial.println("[Network] Running in standalone local offline buffer mode.");
    }

    // Create FreeRTOS Queue for 1-element telemetry frame exchange
    telemetryQueue = xQueueCreate(1, sizeof(TelemetryFrame));

    // Launch Dual-Core Tasks
    xTaskCreatePinnedToCore(TaskIMU, "TaskIMU", 4096, NULL, 3, &TaskIMUHandle, 0);
    xTaskCreatePinnedToCore(TaskNetwork, "TaskNetwork", 4096, NULL, 2, &TaskNetworkHandle, 1);

    // Boot complete haptic chirp
    triggerHapticFeedback(HAPTIC_PULSE_SHORT_MS);
    digitalWrite(PIN_LED_STATUS, HIGH);
}

void loop() {
    // Primary execution managed by FreeRTOS tasks
    vTaskDelay(pdMS_TO_TICKS(1000));
}
