const int sensorPins[5] = {17, 16, 26, 25, 33};

const int MOTOR_L_A = 13;
const int MOTOR_L_B = 12;
const int MOTOR_R_A = 27;
const int MOTOR_R_B = 14;

const int THRESHOLD = 2000;
const int BASE_SPEED = 150;
const int MAX_SPEED = 255;

const double Kp = 0.05;
const double Kd = 0.5;

long lastError = 0;

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
  int weights[5] = {-2, -1, 0, 1, 2};
  long weightedSum = 0;
  int count = 0;

  for (int i = 0; i < 5; i++) {
    int val = analogRead(sensorPins[i]);
    if (val > THRESHOLD) {
      weightedSum += weights[i];
      count++;
    }
  }

  long error;
  if (count == 0) {
    error = lastError;
  } else {
    error = weightedSum;
  }

  double output = Kp * error + Kd * (error - lastError);
  lastError = error;

  int left = BASE_SPEED + (int)output;
  int right = BASE_SPEED - (int)output;
  left = constrain(left, -MAX_SPEED, MAX_SPEED);
  right = constrain(right, -MAX_SPEED, MAX_SPEED);

  setMotorSpeed(left, right);

  Serial.printf("error=%ld output=%.2f L=%d R=%d\n", error, output, left, right);
}
