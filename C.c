const int sensorPins[5] = {17, 16, 26, 25, 33};

const int MOTOR_L_A = 13;
const int MOTOR_L_B = 12;
const int MOTOR_R_A = 27;
const int MOTOR_R_B = 14;

const int THRESHOLD = 2000;
const int BASE_SPEED = 150;
const int TURN_SPEED = 80;
const int MAX_SPEED = 255;

int lastTurn = 0;

void driveOneMotor(int pinA, int pinB, int speed) {
  speed = constrain(speed, -MAX_SPEED, MAX_SPEED);
  if (speed >= 0) {
    ledcWrite(pinA, speed);
    ledcWrite(pinB, 0);
  } else {
    ledcWrite(pinA, 0);
    ledcWrite(pinB, -speed);
  }
}

void setMotorSpeed(int left, int right) {
  driveOneMotor(MOTOR_L_A, MOTOR_L_B, left);
  driveOneMotor(MOTOR_R_A, MOTOR_R_B, right);
}

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 5; i++) pinMode(sensorPins[i], INPUT);
  ledcAttach(MOTOR_L_A, 5000, 8);
  ledcAttach(MOTOR_L_B, 5000, 8);
  ledcAttach(MOTOR_R_A, 5000, 8);
  ledcAttach(MOTOR_R_B, 5000, 8);
}

void loop() {
  bool s0 = analogRead(sensorPins[0]) > THRESHOLD;
  bool s1 = analogRead(sensorPins[1]) > THRESHOLD;
  bool s2 = analogRead(sensorPins[2]) > THRESHOLD;
  bool s3 = analogRead(sensorPins[3]) > THRESHOLD;
  bool s4 = analogRead(sensorPins[4]) > THRESHOLD;

  if (s2) {
    setMotorSpeed(BASE_SPEED, BASE_SPEED);
    lastTurn = 0;
  } else if (s1) {
    setMotorSpeed(BASE_SPEED - TURN_SPEED, BASE_SPEED);
    lastTurn = -1;
  } else if (s0) {
    setMotorSpeed(0, BASE_SPEED);
    lastTurn = -1;
  } else if (s3) {
    setMotorSpeed(BASE_SPEED, BASE_SPEED - TURN_SPEED);
    lastTurn = 1;
  } else if (s4) {
    setMotorSpeed(BASE_SPEED, 0);
    lastTurn = 1;
  } else if (lastTurn < 0) {
    setMotorSpeed(-BASE_SPEED, BASE_SPEED);
  } else if (lastTurn > 0) {
    setMotorSpeed(BASE_SPEED, -BASE_SPEED);
  } else {
    setMotorSpeed(BASE_SPEED, BASE_SPEED);
  }

  Serial.printf("s=%d%d%d%d%d lastTurn=%d\n", s0, s1, s2, s3, s4, lastTurn);
}
