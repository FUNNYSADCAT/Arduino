#include <Arduino.h>

/*
 * ESP32 Digital Line Follower
 * Sensor: Digital 5-channel IR
 * Motor Driver: L9110S / H-Bridge แบบ 2 พินต่อมอเตอร์
 *
 * Sensor:
 * D5 -> GPIO33
 * D4 -> GPIO25
 * D3 -> GPIO26
 * D2 -> GPIO16
 * D1 -> GPIO17
 *
 * Motor 1:
 * A -> GPIO13
 * B -> GPIO12
 *
 * Motor 2:
 * A -> GPIO27
 * B -> GPIO14
 */

// ============================================================
// Pin Definitions
// ============================================================

// Sensor จากซ้าย -> ขวา
const int SENSOR_PINS[5] = {
  33, 25, 26, 16, 17
};

// Motor 1 = ซ้าย
const int LEFT_MOTOR_A  = 13;
const int LEFT_MOTOR_B  = 12;

// Motor 2 = ขวา
const int RIGHT_MOTOR_A = 27;
const int RIGHT_MOTOR_B = 14;


// ============================================================
// Sensor Configuration
// ============================================================

// LOW = เจอเส้นดำ
// ถ้าเซนเซอร์ของคุณกลับกัน ให้เปลี่ยนเป็น HIGH
const int LINE_DETECTED = LOW;


// ============================================================
// PWM Configuration
// ============================================================

const int PWM_FREQUENCY = 20000;
const int PWM_RESOLUTION = 8;

const int LEFT_PWM_CHANNEL_A  = 0;
const int LEFT_PWM_CHANNEL_B  = 1;
const int RIGHT_PWM_CHANNEL_A = 2;
const int RIGHT_PWM_CHANNEL_B = 3;


// ============================================================
// PID Configuration
// ============================================================

const float Kp = 45.0;
const float Ki = 0.0;
const float Kd = 20.0;

// ความเร็วพื้นฐาน
const int BASE_SPEED = 150;

// ความเร็วสูงสุด
const int MAX_SPEED = 255;

// จำกัด integral ป้องกัน integral windup
const float INTEGRAL_LIMIT = 100.0;


// ============================================================
// Global Variables
// ============================================================

float error = 0.0;
float lastError = 0.0;
float integral = 0.0;
float derivative = 0.0;
float pidOutput = 0.0;

// ตำแหน่งล่าสุดที่ตรวจพบเส้น
float lastPosition = 2000.0;

// ใช้ตอนหลุดเส้น
float lastKnownError = 0.0;

// เวลา PID ครั้งล่าสุด
unsigned long lastPIDTime = 0;

// Debug
unsigned long lastDebugTime = 0;
const unsigned long DEBUG_INTERVAL = 100;


// ============================================================
// Setup
// ============================================================

void setup() {
  Serial.begin(115200);

  // ตั้ง sensor เป็น input
  for (int i = 0; i < 5; i++) {
    pinMode(SENSOR_PINS[i], INPUT);
  }

  // ตั้ง PWM สำหรับมอเตอร์
  ledcSetup(LEFT_PWM_CHANNEL_A, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcSetup(LEFT_PWM_CHANNEL_B, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcSetup(RIGHT_PWM_CHANNEL_A, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcSetup(RIGHT_PWM_CHANNEL_B, PWM_FREQUENCY, PWM_RESOLUTION);

  ledcAttachPin(LEFT_MOTOR_A, LEFT_PWM_CHANNEL_A);
  ledcAttachPin(LEFT_MOTOR_B, LEFT_PWM_CHANNEL_B);

  ledcAttachPin(RIGHT_MOTOR_A, RIGHT_PWM_CHANNEL_A);
  ledcAttachPin(RIGHT_MOTOR_B, RIGHT_PWM_CHANNEL_B);

  setMotorSpeed(0, 0);

  Serial.println("Digital Line Follower Starting...");

  // Digital sensor ไม่สามารถ calibrate min/max แบบ analog ได้
  calibrateSensors();

  lastPIDTime = millis();
}


// ============================================================
// Calibration
// ============================================================

// Digital sensor ใช้ calibration เพื่อให้เซนเซอร์ปรับตัวก่อนเริ่ม
void calibrateSensors() {
  Serial.println("Calibrating sensors...");

  // อ่าน sensor หลายครั้งเพื่อให้วงจรนิ่ง
  for (int i = 0; i < 50; i++) {
    for (int j = 0; j < 5; j++) {
      digitalRead(SENSOR_PINS[j]);
    }

    delay(10);
  }

  Serial.println("Calibration complete.");
}


// ============================================================
// Sensor Reading
// ============================================================

// อ่าน sensor ทั้ง 5 ตัว และคืนตำแหน่ง 0-4000
// ซ้ายสุด = 0
// ซ้าย = 1000
// กลาง = 2000
// ขวา = 3000
// ขวาสุด = 4000
float readSensors() {

  const int weights[5] = {
    0, 1000, 2000, 3000, 4000
  };

  long weightedSum = 0;
  int activeCount = 0;

  for (int i = 0; i < 5; i++) {

    int value = digitalRead(SENSOR_PINS[i]);

    if (value == LINE_DETECTED) {
      weightedSum += weights[i];
      activeCount++;
    }
  }

  // ไม่มี sensor ตัวไหนเจอเส้น
  if (activeCount == 0) {
    return -1.0;
  }

  return (float)weightedSum / activeCount;
}


// ============================================================
// Compute Error
// ============================================================

// คำนวณ error จากตำแหน่งของเส้นเทียบกับจุดกลาง 2000
float computeError(float position) {

  // หลุดเส้น
  if (position < 0) {

    // ใช้ error ล่าสุดแทน
    return lastKnownError;
  }

  float currentError = position - 2000.0;

  lastKnownError = currentError;
  lastPosition = position;

  return currentError;
}


// ============================================================
// PID Controller
// ============================================================

// คำนวณ PID และคืนค่า correction
float computePID(float currentError) {

  unsigned long now = millis();

  float dt = (now - lastPIDTime) / 1000.0;

  // ป้องกัน dt เป็น 0
  if (dt <= 0.0) {
    dt = 0.001;
  }

  lastPIDTime = now;

  // Integral
  integral += currentError * dt;

  // Anti-windup
  if (integral > INTEGRAL_LIMIT) {
    integral = INTEGRAL_LIMIT;
  }

  if (integral < -INTEGRAL_LIMIT) {
    integral = -INTEGRAL_LIMIT;
  }

  // Derivative
  derivative = (currentError - lastError) / dt;

  // PID
  float output =
    (Kp * currentError) +
    (Ki * integral) +
    (Kd * derivative);

  lastError = currentError;

  return output;
}


// ============================================================
// Motor Control
// ============================================================

// ควบคุมมอเตอร์ซ้าย/ขวา
// รองรับค่าติดลบเพื่อหมุนย้อนกลับ
void setSingleMotor(
  int speed,
  int channelA,
  int channelB
) {

  speed = constrain(speed, -255, 255);

  if (speed > 0) {

    // เดินหน้า
    ledcWrite(channelA, speed);
    ledcWrite(channelB, 0);

  } else if (speed < 0) {

    // ถอยหลัง
    ledcWrite(channelA, 0);
    ledcWrite(channelB, -speed);

  } else {

    // หยุด
    ledcWrite(channelA, 0);
    ledcWrite(channelB, 0);
  }
}


// ควบคุมมอเตอร์ทั้งสองข้าง
void setMotorSpeed(int left, int right) {

  left = constrain(left, -MAX_SPEED, MAX_SPEED);
  right = constrain(right, -MAX_SPEED, MAX_SPEED);

  setSingleMotor(
    left,
    LEFT_PWM_CHANNEL_A,
    LEFT_PWM_CHANNEL_B
  );

  setSingleMotor(
    right,
    RIGHT_PWM_CHANNEL_A,
    RIGHT_PWM_CHANNEL_B
  );
}


// ============================================================
// Intersection Detection
// ============================================================

// ตรวจว่าทั้ง 5 sensor เจอเส้นพร้อมกันหรือไม่
bool isIntersection() {

  for (int i = 0; i < 5; i++) {

    if (digitalRead(SENSOR_PINS[i]) != LINE_DETECTED) {
      return false;
    }
  }

  return true;
}


// ============================================================
// Debug
// ============================================================

// แสดงค่าทาง Serial ทุก 100 ms
void debugOutput(
  float position,
  float currentError,
  float output
) {

  unsigned long now = millis();

  if (now - lastDebugTime < DEBUG_INTERVAL) {
    return;
  }

  lastDebugTime = now;

  Serial.print("Position: ");
  Serial.print(position);

  Serial.print(" | Error: ");
  Serial.print(currentError);

  Serial.print(" | PID: ");
  Serial.print(output);

  Serial.print(" | Sensors: ");

  for (int i = 0; i < 5; i++) {
    Serial.print(digitalRead(SENSOR_PINS[i]));

    if (i < 4) {
      Serial.print(",");
    }
  }

  Serial.println();
}


// ============================================================
// Main Loop
// ============================================================

void loop() {

  // อ่านตำแหน่งเส้น
  float position = readSensors();

  // คำนวณ error
  error = computeError(position);

  // ตรวจทางแยก / เส้นตัด
  if (isIntersection()) {

    // TODO: intersection logic

  }

  // คำนวณ PID
  pidOutput = computePID(error);

  // Differential Drive
  //
  // error < 0 = เส้นอยู่ทางซ้าย
  // error > 0 = เส้นอยู่ทางขวา
  //
  // ถ้าหุ่นเลี้ยวผิดด้าน ให้สลับ + กับ - ตรงนี้

  int leftSpeed =
    BASE_SPEED + (int)pidOutput;

  int rightSpeed =
    BASE_SPEED - (int)pidOutput;

  // จำกัดความเร็ว
  leftSpeed = constrain(
    leftSpeed,
    -MAX_SPEED,
    MAX_SPEED
  );

  rightSpeed = constrain(
    rightSpeed,
    -MAX_SPEED,
    MAX_SPEED
  );

  // ส่งความเร็วไปมอเตอร์
  setMotorSpeed(leftSpeed, rightSpeed);

  // Debug
  debugOutput(position, error, pidOutput);
}
