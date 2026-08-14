const int sensorPins[5] = {17, 16, 26, 25, 33};

const int MOTOR_L_A = 13;
const int MOTOR_L_B = 12;
const int MOTOR_R_A = 27;
const int MOTOR_R_B = 14;

const int SENSOR_COUNT = 5;
int sensorMin[SENSOR_COUNT];
int sensorMax[SENSOR_COUNT];
bool INVERT_SENSOR = false;

const int PWM_FREQ = 5000;
const int PWM_RES = 8;

const int BASE_SPEED = 150;
const int MAX_SPEED = 255;

const double Kp = 0.06;
const double Ki = 0.0006;
const double Kd = 0.8;
const long INTEGRAL_LIMIT = 2000;

long lastError = 0;
double integral = 0;
unsigned long lastPidTime = 0;

unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL_MS = 200;

const int LINE_LOST_THRESHOLD = 250;
const int INTERSECTION_THRESHOLD = 700;

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < SENSOR_COUNT; i++) {
    pinMode(sensorPins[i], INPUT);
  }

  ledcAttach(MOTOR_L_A, PWM_FREQ, PWM_RES);
  ledcAttach(MOTOR_L_B, PWM_FREQ, PWM_RES);
  ledcAttach(MOTOR_R_A, PWM_FREQ, PWM_RES);
  ledcAttach(MOTOR_R_B, PWM_FREQ, PWM_RES);

  setMotorSpeed(0, 0);
  calibrateSensors();

  lastPidTime = millis();
}

void calibrateSensors() {
  for (int i = 0; i < SENSOR_COUNT; i++) {
    sensorMin[i] = 4095;
    sensorMax[i] = 0;
  }

  unsigned long start = millis();
  while (millis() - start < 3000) {
    for (int i = 0; i < SENSOR_COUNT; i++) {
      int raw = analogRead(sensorPins[i]);
      if (raw < sensorMin[i]) sensorMin[i] = raw;
      if (raw > sensorMax[i]) sensorMax[i] = raw;
    }
    delay(10);
  }
}

void readSensors(int out[SENSOR_COUNT]) {
  for (int i = 0; i < SENSOR_COUNT; i++) {
    int raw = analogRead(sensorPins[i]);
    int norm = map(raw, sensorMin[i], sensorMax[i], 0, 1000);
    norm = constrain(norm, 0, 1000);
    out[i] = INVERT_SENSOR ? (1000 - norm) : norm;
  }
}

bool computeError(int sensorValues[SENSOR_COUNT], long &error) {
  long weightedSum = 0;
  long total = 0;

  for (int i = 0; i < SENSOR_COUNT; i++) {
    weightedSum += (long)sensorValues[i] * (i * 1000L);
    total += sensorValues[i];
  }

  if (total < LINE_LOST_THRESHOLD) {
    error = lastError;
    return false;
  }

  long position = weightedSum / total;
  error = position - 2000;
  return true;
}

double computePID(long error, unsigned long dt_ms) {
  double dt = dt_ms / 1000.0;
  if (dt <= 0) dt = 0.001;

  integral += error * dt;
  integral = constrain(integral, -INTEGRAL_LIMIT, INTEGRAL_LIMIT);

  double derivative = (error - lastError) / dt;
  double output = Kp * error + Ki * integral + Kd * derivative;

  lastError = error;
  return output;
}

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

void loop() {
  int sensorValues[SENSOR_COUNT];
  readSensors(sensorValues);

  bool allOnLine = true;
  for (int i = 0; i < SENSOR_COUNT; i++) {
    if (sensorValues[i] < INTERSECTION_THRESHOLD) {
      allOnLine = false;
      break;
    }
  }
  if (allOnLine) {
  }

  long error;
  bool onLine = computeError(sensorValues, error);

  unsigned long now = millis();
  unsigned long dt = now - lastPidTime;
  lastPidTime = now;

  double output = computePID(error, dt);

  int left = BASE_SPEED + (int)output;
  int right = BASE_SPEED - (int)output;
  left = constrain(left, -MAX_SPEED, MAX_SPEED);
  right = constrain(right, -MAX_SPEED, MAX_SPEED);

  setMotorSpeed(left, right);

  if (now - lastPrintTime >= PRINT_INTERVAL_MS) {
    lastPrintTime = now;
    Serial.printf("error=%ld onLine=%d output=%.2f L=%d R=%d\n",
                  error, onLine, output, left, right);
  }
}

