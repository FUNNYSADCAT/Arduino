#include <Arduino.h>

// ============================================================
// PIN DEFINITIONS
// ============================================================

// Sensor: ซ้าย -> ขวา
const int SENSOR_PINS[5] = {
  33, 25, 26, 16, 17
};

// Motor ซ้าย
const int LEFT_MOTOR_A = 13;
const int LEFT_MOTOR_B = 12;

// Motor ขวา
const int RIGHT_MOTOR_A = 27;
const int RIGHT_MOTOR_B = 14;


// ============================================================
// SENSOR CONFIG
// ============================================================

// LOW = เจอเส้นดำ
// ถ้าเซนเซอร์กลับด้าน ให้เปลี่ยนเป็น HIGH
const int LINE_DETECTED = LOW;


// ============================================================
// GLOBAL VARIABLES
// ============================================================

// จำทิศทางล่าสุดที่เจอเส้น
int lastDirection = 0;

// -1 = ซ้าย
//  0 = กลาง
//  1 = ขวา

unsigned long lastDebugTime = 0;


// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);


  // ตั้ง sensor
  for (int i = 0; i < 5; i++) {
    pinMode(SENSOR_PINS[i], INPUT);
  }


  // ตั้ง motor
  pinMode(LEFT_MOTOR_A, OUTPUT);
  pinMode(LEFT_MOTOR_B, OUTPUT);

  pinMode(RIGHT_MOTOR_A, OUTPUT);
  pinMode(RIGHT_MOTOR_B, OUTPUT);


  // หยุดมอเตอร์
  stopMotors();


  Serial.println("ESP32 Digital Line Follower");
  Serial.println("No LEDC / No PWM");


  calibrateSensors();
}


// ============================================================
// CALIBRATION
// ============================================================

// Digital sensor ไม่มี min/max analog
// จึงใช้ calibration เพื่อรอให้ sensor พร้อม
void calibrateSensors() {

  Serial.println("Calibrating...");

  for (int i = 0; i < 100; i++) {

    for (int j = 0; j < 5; j++) {
      digitalRead(SENSOR_PINS[j]);
    }

    delay(10);
  }

  Serial.println("Calibration complete.");
}


// ============================================================
// READ SENSORS
// ============================================================

// อ่าน sensor ทั้ง 5 ตัว
//
// Sensor:
// D5 = ซ้ายสุด
// D4
// D3 = กลาง
// D2
// D1 = ขวาสุด
//
// คืนค่า:
// -2 = ซ้ายสุด
// -1 = ซ้าย
//  0 = กลาง
//  1 = ขวา
//  2 = ขวาสุด
// 99 = ไม่เจอเส้น
int readSensors() {

  bool s0 =
    digitalRead(SENSOR_PINS[0])
    == LINE_DETECTED;

  bool s1 =
    digitalRead(SENSOR_PINS[1])
    == LINE_DETECTED;

  bool s2 =
    digitalRead(SENSOR_PINS[2])
    == LINE_DETECTED;

  bool s3 =
    digitalRead(SENSOR_PINS[3])
    == LINE_DETECTED;

  bool s4 =
    digitalRead(SENSOR_PINS[4])
    == LINE_DETECTED;


  // ==========================================================
  // กลาง
  // ==========================================================

  if (s2 && !s0 && !s1 && !s3 && !s4) {
    return 0;
  }


  // ==========================================================
  // ซ้าย
  // ==========================================================

  if (s1 || s0) {

    lastDirection = -1;

    if (s0) {
      return -2;
    }

    return -1;
  }


  // ==========================================================
  // ขวา
  // ==========================================================

  if (s3 || s4) {

    lastDirection = 1;

    if (s4) {
      return 2;
    }

    return 1;
  }


  // ==========================================================
  // ไม่เจอเส้น
  // ==========================================================

  return 99;
}


// ============================================================
// MOTOR CONTROL
// ============================================================

// เดินหน้า
void forward() {

  digitalWrite(LEFT_MOTOR_A, HIGH);
  digitalWrite(LEFT_MOTOR_B, LOW);

  digitalWrite(RIGHT_MOTOR_A, HIGH);
  digitalWrite(RIGHT_MOTOR_B, LOW);
}


// ============================================================
// TURN LEFT
// ============================================================

void turnLeft() {

  // ล้อซ้ายถอย
  digitalWrite(LEFT_MOTOR_A, LOW);
  digitalWrite(LEFT_MOTOR_B, HIGH);

  // ล้อขวาเดินหน้า
  digitalWrite(RIGHT_MOTOR_A, HIGH);
  digitalWrite(RIGHT_MOTOR_B, LOW);
}


// ============================================================
// TURN RIGHT
// ============================================================

void turnRight() {

  // ล้อซ้ายเดินหน้า
  digitalWrite(LEFT_MOTOR_A, HIGH);
  digitalWrite(LEFT_MOTOR_B, LOW);

  // ล้อขวาถอย
  digitalWrite(RIGHT_MOTOR_A, LOW);
  digitalWrite(RIGHT_MOTOR_B, HIGH);
}


// ============================================================
// STOP
// ============================================================

void stopMotors() {

  digitalWrite(LEFT_MOTOR_A, LOW);
  digitalWrite(LEFT_MOTOR_B, LOW);

  digitalWrite(RIGHT_MOTOR_A, LOW);
  digitalWrite(RIGHT_MOTOR_B, LOW);
}


// ============================================================
// DEBUG
// ============================================================

void debugSensors() {

  unsigned long now = millis();

  if (now - lastDebugTime < 100) {
    return;
  }

  lastDebugTime = now;


  Serial.print("Sensors: ");

  for (int i = 0; i < 5; i++) {

    Serial.print(
      digitalRead(SENSOR_PINS[i])
    );

    if (i < 4) {
      Serial.print(",");
    }
  }

  Serial.println();
}


// ============================================================
// LOOP
// ============================================================

void loop() {

  int position = readSensors();


  // ==========================================================
  // กลางเจอเส้น
  // ==========================================================

  if (position == 0) {

    forward();
  }


  // ==========================================================
  // เส้นอยู่ซ้าย
  // ==========================================================

  else if (position == -1) {

    turnLeft();
  }


  // ==========================================================
  // เส้นอยู่ซ้ายสุด
  // ==========================================================

  else if (position == -2) {

    turnLeft();
  }


  // ==========================================================
  // เส้นอยู่ขวา
  // ==========================================================

  else if (position == 1) {

    turnRight();
  }


  // ==========================================================
  // เส้นอยู่ขวาสุด
  // ==========================================================

  else if (position == 2) {

    turnRight();
  }


  // ==========================================================
  // ไม่เจอเส้น
  // ==========================================================

  else if (position == 99) {

    // ใช้ทิศทางล่าสุดในการหาเส้น

    if (lastDirection < 0) {

      turnLeft();

    } else if (lastDirection > 0) {

      turnRight();

    } else {

      stopMotors();
    }
  }


  // ==========================================================
  // Intersection
  // ==========================================================

  // TODO: intersection logic


  debugSensors();
}
float lastKnownError = 0.0;
float lastPosition = 2000.0;

unsigned long lastPIDTime = 0;
unsigned long lastDebugTime = 0;


// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  // -------------------------
  // Sensor
  // -------------------------

  for (int i = 0; i < 5; i++) {
    pinMode(SENSOR_PINS[i], INPUT);
  }


  // -------------------------
  // Motor PWM
  // -------------------------

  // Arduino-ESP32 Core 3.x
  // channel จะถูกเลือกให้อัตโนมัติ
  ledcAttach(
    LEFT_MOTOR_A,
    PWM_FREQUENCY,
    PWM_RESOLUTION
  );

  ledcAttach(
    LEFT_MOTOR_B,
    PWM_FREQUENCY,
    PWM_RESOLUTION
  );

  ledcAttach(
    RIGHT_MOTOR_A,
    PWM_FREQUENCY,
    PWM_RESOLUTION
  );

  ledcAttach(
    RIGHT_MOTOR_B,
    PWM_FREQUENCY,
    PWM_RESOLUTION
  );


  // หยุดมอเตอร์ก่อน
  setMotorSpeed(0, 0);


  Serial.println();
  Serial.println("================================");
  Serial.println("ESP32 LINE FOLLOWER");
  Serial.println("Digital Sensor");
  Serial.println("Arduino-ESP32 Core 3.x");
  Serial.println("================================");


  // Calibration
  calibrateSensors();

  lastPIDTime = millis();
}


// ============================================================
// CALIBRATION
// ============================================================

// Digital sensor ไม่มีค่า analog min/max
// จึงใช้ช่วงนี้อ่าน sensor เพื่อให้วงจรนิ่งก่อนเริ่มทำงาน
void calibrateSensors() {

  Serial.println("Calibrating sensors...");

  for (int i = 0; i < 100; i++) {

    for (int j = 0; j < 5; j++) {
      digitalRead(SENSOR_PINS[j]);
    }

    delay(10);
  }

  Serial.println("Calibration complete.");
}


// ============================================================
// READ SENSORS
// ============================================================

// อ่าน sensor ทั้ง 5 ตัว
//
// Position:
// 0    = ซ้ายสุด
// 1000 = ซ้าย
// 2000 = กลาง
// 3000 = ขวา
// 4000 = ขวาสุด
//
// คืนค่า -1 ถ้าไม่มี sensor ตัวไหนเจอเส้น
float readSensors() {

  const int weights[5] = {
    0,
    1000,
    2000,
    3000,
    4000
  };

  long weightedSum = 0;
  int activeCount = 0;


  for (int i = 0; i < 5; i++) {

    int sensorValue = digitalRead(SENSOR_PINS[i]);

    if (sensorValue == LINE_DETECTED) {

      weightedSum += weights[i];
      activeCount++;
    }
  }


  // ไม่เจอเส้น
  if (activeCount == 0) {
    return -1.0;
  }


  // ถ้ามีหลาย sensor เจอเส้น
  // ใช้ค่าเฉลี่ยตำแหน่ง
  return (float)weightedSum / activeCount;
}


// ============================================================
// COMPUTE ERROR
// ============================================================

// จุดกลาง = 2000
//
// error < 0 = เส้นอยู่ทางซ้าย
// error > 0 = เส้นอยู่ทางขวา
float computeError(float position) {

  // หลุดเส้น
  if (position < 0) {

    // ใช้ทิศทางล่าสุด
    return lastKnownError;
  }


  float currentError = position - 2000.0;


  lastKnownError = currentError;
  lastPosition = position;


  return currentError;
}


// ============================================================
// PID CONTROLLER
// ============================================================

// คำนวณ PID
float computePID(float currentError) {

  unsigned long now = millis();

  float dt =
    (now - lastPIDTime) / 1000.0;


  if (dt <= 0.0) {
    dt = 0.001;
  }


  lastPIDTime = now;


  // -------------------------
  // Integral
  // -------------------------

  integral += currentError * dt;


  // Anti-windup
  if (integral > INTEGRAL_LIMIT) {
    integral = INTEGRAL_LIMIT;
  }

  if (integral < -INTEGRAL_LIMIT) {
    integral = -INTEGRAL_LIMIT;
  }


  // -------------------------
  // Derivative
  // -------------------------

  derivative =
    (currentError - lastError) / dt;


  // -------------------------
  // PID
  // -------------------------

  float output =
      (Kp * currentError)
    + (Ki * integral)
    + (Kd * derivative);


  lastError = currentError;


  return output;
}


// ============================================================
// SINGLE MOTOR CONTROL
// ============================================================

// ควบคุมมอเตอร์ 1 ตัว
//
// speed > 0 = เดินหน้า
// speed < 0 = ถอยหลัง
// speed = 0 = หยุด
void setSingleMotor(
  int speed,
  int pinA,
  int pinB
) {

  speed = constrain(
    speed,
    -MAX_SPEED,
    MAX_SPEED
  );


  if (speed > 0) {

    // เดินหน้า
    ledcWrite(pinA, speed);
    ledcWrite(pinB, 0);

  }

  else if (speed < 0) {

    // ถอยหลัง
    ledcWrite(pinA, 0);
    ledcWrite(pinB, -speed);

  }

  else {

    // หยุด
    ledcWrite(pinA, 0);
    ledcWrite(pinB, 0);
  }
}


// ============================================================
// MOTOR CONTROL
// ============================================================

// ควบคุมมอเตอร์ซ้ายและขวา
void setMotorSpeed(
  int left,
  int right
) {

  left = constrain(
    left,
    -MAX_SPEED,
    MAX_SPEED
  );

  right = constrain(
    right,
    -MAX_SPEED,
    MAX_SPEED
  );


  setSingleMotor(
    left,
    LEFT_MOTOR_A,
    LEFT_MOTOR_B
  );


  setSingleMotor(
    right,
    RIGHT_MOTOR_A,
    RIGHT_MOTOR_B
  );
}


// ============================================================
// INTERSECTION DETECTION
// ============================================================

// ตรวจว่าทั้ง 5 sensor เจอเส้นพร้อมกันหรือไม่
bool isIntersection() {

  for (int i = 0; i < 5; i++) {

    if (
      digitalRead(SENSOR_PINS[i])
      != LINE_DETECTED
    ) {

      return false;
    }
  }


  return true;
}


// ============================================================
// DEBUG
// ============================================================

// แสดงข้อมูลทุก 100 ms
// ไม่ print ทุก loop เพื่อไม่ให้ Serial หน่วงระบบ
void debugOutput(
  float position,
  float error,
  float pidOutput
) {

  unsigned long now = millis();


  if (
    now - lastDebugTime
    < 100
  ) {

    return;
  }


  lastDebugTime = now;


  Serial.print("Sensors: ");


  for (int i = 0; i < 5; i++) {

    Serial.print(
      digitalRead(SENSOR_PINS[i])
    );


    if (i < 4) {
      Serial.print(",");
    }
  }


  Serial.print(" | Position: ");
  Serial.print(position);


  Serial.print(" | Error: ");
  Serial.print(error);


  Serial.print(" | PID: ");
  Serial.print(pidOutput);


  Serial.println();
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop() {

  // -------------------------
  // อ่าน sensor
  // -------------------------

  float position =
    readSensors();


  // -------------------------
  // คำนวณ error
  // -------------------------

  float error =
    computeError(position);


  // -------------------------
  // Intersection
  // -------------------------

  if (isIntersection()) {

    // TODO: intersection logic

  }


  // -------------------------
  // PID
  // -------------------------

  float pidOutput =
    computePID(error);


  // -------------------------
  // Differential Drive
  // -------------------------

  /*
    PID เป็นบวก:
      เส้นอยู่ทางขวา

      ลดความเร็วล้อขวา
      เพิ่มความเร็วล้อซ้าย

    PID เป็นลบ:
      เส้นอยู่ทางซ้าย

      ลดความเร็วล้อซ้าย
      เพิ่มความเร็วล้อขวา
  */

  int leftSpeed =
    BASE_SPEED + (int)pidOutput;

  int rightSpeed =
    BASE_SPEED - (int)pidOutput;


  // -------------------------
  // จำกัดความเร็ว
  // -------------------------

  leftSpeed =
    constrain(
      leftSpeed,
      -MAX_SPEED,
      MAX_SPEED
    );


  rightSpeed =
    constrain(
      rightSpeed,
      -MAX_SPEED,
      MAX_SPEED
    );


  // -------------------------
  // สั่งมอเตอร์
  // -------------------------

  setMotorSpeed(
    leftSpeed,
    rightSpeed
  );


  // -------------------------
  // Debug
  // -------------------------

  debugOutput(
    position,
    error,
    pidOutput
  );
}
