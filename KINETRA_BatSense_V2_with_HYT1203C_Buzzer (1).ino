/*
  KINETRA BATSENSE V2
  ESP32 + MPU6050 + SSD1306 OLED

  I2C:
    SDA = GPIO 21
    SCL = GPIO 22

  LEDs:
    Green  = GPIO 25
    Blue   = GPIO 26
    Red    = GPIO 27
    Yellow = GPIO 32

  Buttons:
    Start/Stop      = GPIO 18
    Activity Select = GPIO 19

  I2C Addresses:
    MPU6050 = 0x68
    OLED    = 0x3C

  Required Libraries:
    Adafruit GFX Library
    Adafruit SSD1306
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// =====================================================
// DISPLAY CONFIGURATION
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// =====================================================
// PIN CONFIGURATION
// =====================================================

const int SDA_PIN = 21;
const int SCL_PIN = 22;

const int LED_GREEN  = 25;
const int LED_BLUE   = 26;
const int LED_RED    = 27;
const int LED_YELLOW = 32;

const int BUTTON_START = 18;
const int BUTTON_ACTIVITY = 19;
const int BUZZER_PIN = 23;  // Drives 2N2222 base through 1k resistor

// =====================================================
// MPU6050 REGISTERS
// =====================================================

const uint8_t MPU_ADDRESS = 0x68;

const uint8_t REG_SMPLRT_DIV   = 0x19;
const uint8_t REG_CONFIG       = 0x1A;
const uint8_t REG_GYRO_CONFIG  = 0x1B;
const uint8_t REG_ACCEL_CONFIG = 0x1C;
const uint8_t REG_ACCEL_XOUT_H = 0x3B;
const uint8_t REG_PWR_MGMT_1   = 0x6B;
const uint8_t REG_WHO_AM_I     = 0x75;

// =====================================================
// APPLICATION STATES
// =====================================================

enum AppState {
  STATE_READY,
  STATE_RECORDING,
  STATE_COMPLETE,
  STATE_ERROR
};

AppState appState = STATE_READY;

// =====================================================
// CRICKET ACTIVITIES
// =====================================================

const char* activities[] = {
  "GENERAL",
  "DRIVE",
  "PULL",
  "CUT",
  "DEFENSIVE"
};

const int ACTIVITY_COUNT = 5;

int activityIndex = 0;

// =====================================================
// LIVE SENSOR VALUES
// =====================================================

float accelX = 0;
float accelY = 0;
float accelZ = 0;

float gyroX = 0;
float gyroY = 0;
float gyroZ = 0;

float accelMagnitude = 0;
float dynamicAcceleration = 0;
float gyroMagnitude = 0;

// =====================================================
// SESSION ANALYTICS
// =====================================================

float peakGyro = 0;
float peakDynamicAcceleration = 0;
float gyroMagnitudeSum = 0;

unsigned long sensorSamples = 0;

unsigned long sessionStartTime = 0;
unsigned long sessionEndTime = 0;

unsigned long lastSensorRead = 0;
unsigned long lastSerialPrint = 0;
unsigned long lastOLEDUpdate = 0;
unsigned long lastMotionEvent = 0;

int swingCount = 0;

bool motionArmed = true;

// =====================================================
// TIMING AND MOTION THRESHOLDS
// =====================================================

const unsigned long SENSOR_INTERVAL_MS = 20;
const unsigned long SERIAL_INTERVAL_MS = 200;
const unsigned long OLED_INTERVAL_MS = 150;

const unsigned long BUTTON_DEBOUNCE_MS = 45;
const unsigned long MOTION_COOLDOWN_MS = 350;

const float SWING_TRIGGER_GYRO_DPS = 45.0;
const float SWING_REARM_GYRO_DPS = 25.0;

// =====================================================
// BUTTON STRUCTURE
// =====================================================

struct Button {
  int pin;
  bool lastReading;
  bool stableState;
  unsigned long lastChange;
};

Button startButton = {
  BUTTON_START, HIGH, HIGH, 0
};

Button activityButton = {
  BUTTON_ACTIVITY, HIGH, HIGH, 0
};

// =====================================================
// LED CONTROL
// =====================================================

void setLEDs(
  bool green,
  bool blue,
  bool red,
  bool yellow
) {
  digitalWrite(
    LED_GREEN,
    green ? HIGH : LOW
  );

  digitalWrite(
    LED_BLUE,
    blue ? HIGH : LOW
  );

  digitalWrite(
    LED_RED,
    red ? HIGH : LOW
  );

  digitalWrite(
    LED_YELLOW,
    yellow ? HIGH : LOW
  );
}

void showReadyLEDs() {
  setLEDs(false, false, false, true);
}

void showRecordingLEDs() {
  setLEDs(true, false, false, false);
}

void showCompleteLEDs() {
  setLEDs(false, true, false, false);
}

void showErrorLEDs() {
  setLEDs(false, false, true, false);
}


// =====================================================
// ACTIVE BUZZER CONTROL (HYT-1203C via 2N2222)
// =====================================================
// GPIO23 drives the transistor base through a 1k resistor.
// This is an ACTIVE buzzer: we create rhythms by switching it
// on/off; the buzzer itself determines the pitch.
// Durations alternate ON, OFF, ON, OFF, starting with ON.

const uint16_t BUZZER_STARTUP_PATTERN[] = {
  800, 150, 120, 120, 120, 250, 800, 640
}; // Total: 3000 ms

const uint16_t BUZZER_SELECTION_PATTERN[] = {
  90, 1
};

const uint16_t BUZZER_RECORD_START_PATTERN[] = {
  90, 90, 90, 1
}; // Two quick beeps

const uint16_t BUZZER_STOP_COMPLETE_PATTERN[] = {
  90, 90, 90, 250, 90, 90, 90, 90, 90, 1
}; // Two stop beeps, pause, three completion beeps

const uint16_t* buzzerPattern = nullptr;
uint8_t buzzerPatternLength = 0;
uint8_t buzzerPatternIndex = 0;
unsigned long buzzerNextStepAt = 0;
bool buzzerRunning = false;

void stopBuzzer() {
  digitalWrite(BUZZER_PIN, LOW);
  buzzerRunning = false;
  buzzerPattern = nullptr;
}

void startBuzzerPattern(const uint16_t* pattern, uint8_t length) {
  digitalWrite(BUZZER_PIN, LOW);
  buzzerRunning = false;

  if (pattern == nullptr || length == 0) return;

  buzzerPattern = pattern;
  buzzerPatternLength = length;
  buzzerPatternIndex = 0;
  buzzerRunning = true;

  digitalWrite(BUZZER_PIN, HIGH);
  buzzerNextStepAt = millis() + buzzerPattern[0];
}

void updateBuzzer() {
  if (!buzzerRunning || buzzerPattern == nullptr) return;
  if ((long)(millis() - buzzerNextStepAt) < 0) return;

  buzzerPatternIndex++;
  if (buzzerPatternIndex >= buzzerPatternLength) {
    stopBuzzer();
    return;
  }

  // Pattern index 0,2,4... are ON; 1,3,5... are OFF.
  digitalWrite(BUZZER_PIN,
               (buzzerPatternIndex % 2 == 0) ? HIGH : LOW);
  buzzerNextStepAt = millis() + buzzerPattern[buzzerPatternIndex];
}

void beepShotSelection() {
  startBuzzerPattern(BUZZER_SELECTION_PATTERN,
    sizeof(BUZZER_SELECTION_PATTERN) / sizeof(BUZZER_SELECTION_PATTERN[0]));
}

void beepRecordingStart() {
  startBuzzerPattern(BUZZER_RECORD_START_PATTERN,
    sizeof(BUZZER_RECORD_START_PATTERN) / sizeof(BUZZER_RECORD_START_PATTERN[0]));
}

void beepStopAndSessionComplete() {
  startBuzzerPattern(BUZZER_STOP_COMPLETE_PATTERN,
    sizeof(BUZZER_STOP_COMPLETE_PATTERN) / sizeof(BUZZER_STOP_COMPLETE_PATTERN[0]));
}

// =====================================================
// MPU6050 I2C FUNCTIONS
// =====================================================

bool writeMPURegister(
  uint8_t reg,
  uint8_t value
) {
  Wire.beginTransmission(MPU_ADDRESS);

  Wire.write(reg);
  Wire.write(value);

  return Wire.endTransmission() == 0;
}

bool readMPURegisters(
  uint8_t startRegister,
  uint8_t count,
  uint8_t* buffer
) {
  Wire.beginTransmission(MPU_ADDRESS);

  Wire.write(startRegister);

  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  uint8_t received = Wire.requestFrom(
    (uint8_t)MPU_ADDRESS,
    count,
    (bool)true
  );

  if (received != count) {
    while (Wire.available()) {
      Wire.read();
    }

    return false;
  }

  for (uint8_t i = 0; i < count; i++) {
    buffer[i] = Wire.read();
  }

  return true;
}

int16_t combineBytes(
  uint8_t highByte,
  uint8_t lowByte
) {
  return (int16_t)(
    ((uint16_t)highByte << 8) |
    lowByte
  );
}

// =====================================================
// INITIALIZE MPU6050
// =====================================================

bool initializeMPU6050() {

  Serial.println("Initializing MPU6050...");

  uint8_t whoAmI = 0;

  if (
    readMPURegisters(
      REG_WHO_AM_I,
      1,
      &whoAmI
    )
  ) {
    Serial.print("MPU6050 WHO_AM_I: 0x");

    Serial.println(
      whoAmI,
      HEX
    );
  }

  if (
    !writeMPURegister(
      REG_PWR_MGMT_1,
      0x00
    )
  ) {
    return false;
  }

  delay(100);

  if (
    !writeMPURegister(
      REG_SMPLRT_DIV,
      0x04
    )
  ) {
    return false;
  }

  if (
    !writeMPURegister(
      REG_CONFIG,
      0x03
    )
  ) {
    return false;
  }

  // Gyroscope range: +/-250 degrees/second

  if (
    !writeMPURegister(
      REG_GYRO_CONFIG,
      0x00
    )
  ) {
    return false;
  }

  // Accelerometer range: +/-2g

  if (
    !writeMPURegister(
      REG_ACCEL_CONFIG,
      0x00
    )
  ) {
    return false;
  }

  delay(100);

  uint8_t raw[14];

  if (
    !readMPURegisters(
      REG_ACCEL_XOUT_H,
      14,
      raw
    )
  ) {
    return false;
  }

  Serial.println(
    "MPU6050 initialized successfully."
  );

  return true;
}

// =====================================================
// READ ACCELEROMETER AND GYROSCOPE
// =====================================================

bool readMotionSensor() {

  uint8_t raw[14];

  if (
    !readMPURegisters(
      REG_ACCEL_XOUT_H,
      14,
      raw
    )
  ) {
    return false;
  }

  int16_t ax = combineBytes(
    raw[0],
    raw[1]
  );

  int16_t ay = combineBytes(
    raw[2],
    raw[3]
  );

  int16_t az = combineBytes(
    raw[4],
    raw[5]
  );

  int16_t gx = combineBytes(
    raw[8],
    raw[9]
  );

  int16_t gy = combineBytes(
    raw[10],
    raw[11]
  );

  int16_t gz = combineBytes(
    raw[12],
    raw[13]
  );

  // Convert accelerometer values into g.

  accelX = ax / 16384.0f;
  accelY = ay / 16384.0f;
  accelZ = az / 16384.0f;

  // Convert gyroscope values into degrees/second.

  gyroX = gx / 131.0f;
  gyroY = gy / 131.0f;
  gyroZ = gz / 131.0f;

  accelMagnitude = sqrtf(
    accelX * accelX +
    accelY * accelY +
    accelZ * accelZ
  );

  // Approximate acceleration beyond gravity.

  dynamicAcceleration = fabsf(
    accelMagnitude - 1.0f
  );

  gyroMagnitude = sqrtf(
    gyroX * gyroX +
    gyroY * gyroY +
    gyroZ * gyroZ
  );

  return true;
}

// =====================================================
// OLED TITLE
// =====================================================

// For a typical split-colour OLED, keep the title
// above the yellow/blue boundary and body text below it.

void drawTitle(const char* title) {

  display.setTextSize(1);

  display.setTextColor(
    SSD1306_WHITE
  );

  display.setCursor(0, 2);

  display.print(title);

  display.drawLine(
    0, 14,
    127, 14,
    SSD1306_WHITE
  );
}

// =====================================================
// STARTUP ANIMATION
// =====================================================

void drawStartupFrame(int frame) {

  display.clearDisplay();

  drawTitle("KINETRA BATSENSE");

  int shift = (frame % 4) * 2;

  // Batsman's head.

  display.drawCircle(
    23, 29,
    4,
    SSD1306_WHITE
  );

  // Body.

  display.drawLine(
    23, 33,
    27, 43,
    SSD1306_WHITE
  );

  // Arms.

  display.drawLine(
    25, 37,
    15 + shift, 32,
    SSD1306_WHITE
  );

  display.drawLine(
    26, 37,
    35, 30 - shift,
    SSD1306_WHITE
  );

  // Legs.

  display.drawLine(
    27, 43,
    19, 52,
    SSD1306_WHITE
  );

  display.drawLine(
    27, 43,
    35, 51,
    SSD1306_WHITE
  );

  // Bat swing.

  display.drawLine(
    35, 30 - shift,
    48, 19 + shift,
    SSD1306_WHITE
  );

  display.drawLine(
    45, 22 + shift,
    51, 16 + shift,
    SSD1306_WHITE
  );

  // Moving cricket ball.

  int ballX = 103 - (frame % 8) * 5;
  int ballY = 27 + (frame % 4) * 3;

  display.drawPixel(
    88, 22,
    SSD1306_WHITE
  );

  display.drawPixel(
    93, 24,
    SSD1306_WHITE
  );

  display.drawPixel(
    98, 26,
    SSD1306_WHITE
  );

  display.fillCircle(
    ballX,
    ballY,
    3,
    SSD1306_WHITE
  );

  display.drawLine(
    ballX - 2,
    ballY - 2,
    ballX + 2,
    ballY + 2,
    SSD1306_BLACK
  );

  display.setCursor(8, 56);

  display.print(
    "BAT MOTION ANALYTICS"
  );

  display.display();
}

void runStartupAnimation() {

  Serial.println();

  Serial.println(
    "KINETRA BATSENSE V2 - STARTING"
  );

  // Green, blue and yellow illuminate together.
  // Red remains reserved for error conditions.

  setLEDs(
    true,
    true,
    false,
    true
  );

  startBuzzerPattern(BUZZER_STARTUP_PATTERN,
    sizeof(BUZZER_STARTUP_PATTERN) / sizeof(BUZZER_STARTUP_PATTERN[0]));

  unsigned long started = millis();
  unsigned long lastFrame = 0;

  int frame = 0;

  while (
    millis() - started < 3000UL
  ) {

    updateBuzzer();
    unsigned long now = millis();

    if (
      now - lastFrame >= 100UL
    ) {
      lastFrame = now;

      drawStartupFrame(
        frame++
      );
    }

    delay(1);
  }

  stopBuzzer();

  setLEDs(
    false,
    false,
    false,
    false
  );

  display.clearDisplay();
  display.display();

  Serial.println(
    "Startup animation complete."
  );
}

// =====================================================
// BUTTON DEBOUNCING
// =====================================================

bool buttonPressed(Button &button) {

  bool reading = digitalRead(
    button.pin
  );

  if (
    reading != button.lastReading
  ) {
    button.lastChange = millis();

    button.lastReading = reading;
  }

  if (
    millis() - button.lastChange >=
    BUTTON_DEBOUNCE_MS
  ) {

    if (
      reading != button.stableState
    ) {

      button.stableState = reading;

      if (
        button.stableState == LOW
      ) {
        return true;
      }
    }
  }

  return false;
}

// =====================================================
// READY SCREEN
// =====================================================

void drawReadyScreen() {

  display.clearDisplay();

  drawTitle(
    "KINETRA BATSENSE"
  );

  display.setCursor(0, 19);

  display.print(
    "STATUS: READY"
  );

  display.setCursor(0, 28);

  display.print(
    "ACTIVITY: "
  );

  display.print(
    activities[activityIndex]
  );

  display.setCursor(0, 37);

  display.print(
    "SWINGS: "
  );

  display.print(
    swingCount
  );

  display.setCursor(0, 46);

  display.print(
    "SELECT: NEXT SHOT"
  );

  display.setCursor(0, 55);

  display.print(
    "PRESS START"
  );

  display.display();
}

// =====================================================
// RECORDING SCREEN
// =====================================================

void drawRecordingScreen() {

  display.clearDisplay();

  drawTitle(
    "BATSENSE: RECORD"
  );

  display.setCursor(0, 19);

  display.print(
    "SHOT: "
  );

  display.print(
    activities[activityIndex]
  );

  display.setCursor(0, 28);

  display.print(
    "SWINGS: "
  );

  display.print(
    swingCount
  );

  display.setCursor(0, 37);

  display.print(
    "GYRO: "
  );

  display.print(
    gyroMagnitude,
    1
  );

  display.print(
    " dps"
  );

  display.setCursor(0, 46);

  display.print(
    "ACCEL: "
  );

  display.print(
    dynamicAcceleration,
    2
  );

  display.print(
    " g"
  );

  display.setCursor(0, 55);

  display.print(
    "PRESS TO STOP"
  );

  display.display();
}

// =====================================================
// SESSION COMPLETE SCREEN
// =====================================================

void drawCompleteScreen() {

  display.clearDisplay();

  drawTitle(
    "SESSION COMPLETE"
  );

  display.setCursor(0, 19);

  display.print(
    "SHOT: "
  );

  display.print(
    activities[activityIndex]
  );

  display.setCursor(0, 28);

  display.print(
    "SWINGS: "
  );

  display.print(
    swingCount
  );

  display.setCursor(0, 37);

  display.print(
    "PEAK GYRO: "
  );

  display.print(
    peakGyro,
    1
  );

  display.print(
    " dps"
  );

  display.setCursor(0, 46);

  display.print(
    "PEAK ACC: "
  );

  display.print(
    peakDynamicAcceleration,
    2
  );

  display.print(
    "g"
  );

  display.setCursor(0, 55);

  display.print(
    "PRESS START"
  );

  display.display();
}

// =====================================================
// ERROR SCREEN
// =====================================================

void drawErrorScreen() {

  display.clearDisplay();

  drawTitle(
    "BATSENSE ERROR"
  );

  display.setCursor(0, 22);

  display.print(
    "MPU6050 NOT RESPONDING"
  );

  display.setCursor(0, 36);

  display.print(
    "CHECK I2C CONNECTION"
  );

  display.setCursor(0, 52);

  display.print(
    "RESET TO RETRY"
  );

  display.display();
}

// =====================================================
// RESET SESSION STATISTICS
// =====================================================

void resetSessionStatistics() {

  swingCount = 0;

  peakGyro = 0;

  peakDynamicAcceleration = 0;

  gyroMagnitudeSum = 0;

  sensorSamples = 0;

  lastMotionEvent = 0;

  motionArmed = true;
}

// =====================================================
// START SESSION
// =====================================================

void startSession() {

  resetSessionStatistics();

  sessionStartTime = millis();

  lastSensorRead = 0;
  lastSerialPrint = 0;
  lastOLEDUpdate = 0;

  appState = STATE_RECORDING;

  showRecordingLEDs();
  beepRecordingStart();

  Serial.println();

  Serial.println(
    "========== SESSION STARTED =========="
  );

  Serial.print(
    "Activity: "
  );

  Serial.println(
    activities[activityIndex]
  );

  Serial.println(
    "Live sensor output enabled."
  );
}

// =====================================================
// UPDATE MOTION STATISTICS
// =====================================================

void updateMotionStatistics() {

  if (
    gyroMagnitude > peakGyro
  ) {
    peakGyro = gyroMagnitude;
  }

  if (
    dynamicAcceleration >
    peakDynamicAcceleration
  ) {
    peakDynamicAcceleration =
      dynamicAcceleration;
  }

  gyroMagnitudeSum += gyroMagnitude;

  sensorSamples++;

  unsigned long now = millis();

  if (
    gyroMagnitude <
    SWING_REARM_GYRO_DPS
  ) {
    motionArmed = true;
  }

  if (
    motionArmed &&
    gyroMagnitude >= SWING_TRIGGER_GYRO_DPS &&
    (
      lastMotionEvent == 0 ||
      now - lastMotionEvent >=
      MOTION_COOLDOWN_MS
    )
  ) {

    swingCount++;

    lastMotionEvent = now;

    motionArmed = false;

    Serial.print(
      "[MOTION EVENT] Count: "
    );

    Serial.println(
      swingCount
    );
  }
}

// =====================================================
// LIVE SERIAL MONITOR OUTPUT
// =====================================================

void printLiveSensorData() {

  Serial.print(
    "[REC] Activity="
  );

  Serial.print(
    activities[activityIndex]
  );

  Serial.print(
    " | AX="
  );

  Serial.print(
    accelX,
    3
  );

  Serial.print(
    "g AY="
  );

  Serial.print(
    accelY,
    3
  );

  Serial.print(
    "g AZ="
  );

  Serial.print(
    accelZ,
    3
  );

  Serial.print(
    "g | GX="
  );

  Serial.print(
    gyroX,
    2
  );

  Serial.print(
    "dps GY="
  );

  Serial.print(
    gyroY,
    2
  );

  Serial.print(
    "dps GZ="
  );

  Serial.print(
    gyroZ,
    2
  );

  Serial.print(
    "dps | GYRO="
  );

  Serial.print(
    gyroMagnitude,
    2
  );

  Serial.print(
    "dps | DYN_ACC="
  );

  Serial.print(
    dynamicAcceleration,
    3
  );

  Serial.print(
    "g | EVENTS="
  );

  Serial.print(
    swingCount
  );

  Serial.print(
    " | PEAK_GYRO="
  );

  Serial.print(
    peakGyro,
    2
  );

  Serial.print(
    "dps | PEAK_DYN_ACC="
  );

  Serial.print(
    peakDynamicAcceleration,
    3
  );

  Serial.println(
    "g"
  );
}

// =====================================================
// SESSION REPORT
// =====================================================

void printSessionReport() {

  float duration =
    (sessionEndTime - sessionStartTime) /
    1000.0f;

  float averageGyro =
    sensorSamples > 0
    ? gyroMagnitudeSum / sensorSamples
    : 0.0f;

  float eventsPerMinute =
    duration > 0
    ? (swingCount * 60.0f) / duration
    : 0.0f;

  Serial.println();

  Serial.println(
    "========================================"
  );

  Serial.println(
    "           SESSION COMPLETE"
  );

  Serial.println(
    "========================================"
  );

  Serial.print(
    "Activity: "
  );

  Serial.println(
    activities[activityIndex]
  );

  Serial.print(
    "Duration: "
  );

  Serial.print(
    duration,
    2
  );

  Serial.println(
    " seconds"
  );

  Serial.print(
    "Detected motion events: "
  );

  Serial.println(
    swingCount
  );

  Serial.print(
    "Peak gyro magnitude: "
  );

  Serial.print(
    peakGyro,
    2
  );

  Serial.println(
    " degrees/second"
  );

  Serial.print(
    "Peak dynamic acceleration: "
  );

  Serial.print(
    peakDynamicAcceleration,
    3
  );

  Serial.println(
    " g"
  );

  Serial.print(
    "Average gyro magnitude: "
  );

  Serial.print(
    averageGyro,
    2
  );

  Serial.println(
    " degrees/second"
  );

  Serial.print(
    "Events per minute: "
  );

  Serial.println(
    eventsPerMinute,
    2
  );

  Serial.println(
    "========================================"
  );

  Serial.println(
    "Press START for a new session."
  );
}

// =====================================================
// STOP SESSION
// =====================================================

void stopSession() {

  sessionEndTime = millis();

  appState = STATE_COMPLETE;

  showCompleteLEDs();
  beepStopAndSessionComplete();

  printSessionReport();

  drawCompleteScreen();
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(300);

  pinMode(
    LED_GREEN,
    OUTPUT
  );

  pinMode(
    LED_BLUE,
    OUTPUT
  );

  pinMode(
    LED_RED,
    OUTPUT
  );

  pinMode(
    LED_YELLOW,
    OUTPUT
  );

  setLEDs(
    false,
    false,
    false,
    false
  );

  pinMode(
    BUTTON_START,
    INPUT_PULLUP
  );

  pinMode(
    BUTTON_ACTIVITY,
    INPUT_PULLUP
  );

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Wire.begin(
    SDA_PIN,
    SCL_PIN
  );

  Wire.setClock(100000);

  Serial.println();

  Serial.println(
    "KINETRA BatSense V2 booting..."
  );

  if (
    !display.begin(
      SSD1306_SWITCHCAPVCC,
      OLED_ADDRESS
    )
  ) {

    Serial.println(
      "OLED initialization failed."
    );

    showErrorLEDs();

    while (true) {
      delay(100);
    }
  }

  display.clearDisplay();

  display.setTextSize(1);

  display.setTextColor(
    SSD1306_WHITE
  );

  display.display();

  // Three-second cricket startup animation.

  runStartupAnimation();

  if (
    !initializeMPU6050()
  ) {

    appState = STATE_ERROR;

    showErrorLEDs();

    drawErrorScreen();

    Serial.println(
      "MPU6050 initialization failed. Reset to retry."
    );

    while (true) {
      delay(100);
    }
  }

  startButton.lastReading =
    digitalRead(BUTTON_START);

  startButton.stableState =
    startButton.lastReading;

  activityButton.lastReading =
    digitalRead(BUTTON_ACTIVITY);

  activityButton.stableState =
    activityButton.lastReading;

  appState = STATE_READY;

  showReadyLEDs();

  drawReadyScreen();

  Serial.println(
    "KINETRA BatSense is READY."
  );

  Serial.println(
    "Select activity, then press START."
  );
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  updateBuzzer();

  if (
    appState == STATE_ERROR
  ) {
    return;
  }

  // ---------------------------------------------------
  // ACTIVITY SELECTION
  // Works on READY and SESSION COMPLETE screens.
  // ---------------------------------------------------

  if (
    (
      appState == STATE_READY ||
      appState == STATE_COMPLETE
    ) &&
    buttonPressed(activityButton)
  ) {

    activityIndex =
      (activityIndex + 1) %
      ACTIVITY_COUNT;

    appState = STATE_READY;

    showReadyLEDs();
    beepShotSelection();

    drawReadyScreen();

    Serial.print(
      "Selected activity: "
    );

    Serial.println(
      activities[activityIndex]
    );
  }

  // ---------------------------------------------------
  // START / STOP BUTTON
  // ---------------------------------------------------

  if (
    buttonPressed(startButton)
  ) {

    if (
      appState == STATE_READY ||
      appState == STATE_COMPLETE
    ) {

      startSession();

      drawRecordingScreen();

    } else if (
      appState == STATE_RECORDING
    ) {

      stopSession();
    }
  }

  // ---------------------------------------------------
  // LIVE RECORDING
  // ---------------------------------------------------

  if (
    appState == STATE_RECORDING
  ) {

    unsigned long now = millis();

    // Read sensor at approximately 50 Hz.

    if (
      now - lastSensorRead >=
      SENSOR_INTERVAL_MS
    ) {

      lastSensorRead = now;

      if (
        !readMotionSensor()
      ) {

        Serial.println(
          "ERROR: MPU6050 read failed."
        );

        appState = STATE_ERROR;

        showErrorLEDs();

        drawErrorScreen();

        return;
      }

      updateMotionStatistics();
    }

    // Print live sensor data to Serial Monitor.

    if (
      now - lastSerialPrint >=
      SERIAL_INTERVAL_MS
    ) {

      lastSerialPrint = now;

      printLiveSensorData();
    }

    // Update OLED while recording.

    if (
      now - lastOLEDUpdate >=
      OLED_INTERVAL_MS
    ) {

      lastOLEDUpdate = now;

      drawRecordingScreen();
    }
  }
}