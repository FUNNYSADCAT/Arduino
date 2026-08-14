class Motor {
  int pinA, pinB;

public:
  Motor(int a, int b) : pinA(a), pinB(b) {}

  void begin() {
    ledcAttach(pinA, 5000, 8);
    ledcAttach(pinB, 5000, 8);
  }

  void setSpeed(int speed) {
    speed = constrain(speed, -255, 255);
    if (speed >= 0) {
      ledcWrite(pinA, speed);
      ledcWrite(pinB, 0);
    } else {
      ledcWrite(pinA, 0);
      ledcWrite(pinB, -speed);
    }
  }
};

const int sensorPins[5] = {17, 16, 26, 25, 33};

Motor motorL(13, 12);
Motor motorR(27, 14);

const int THRESHOLD = 2000;
const int BASE_SPEED = 150;
const int TURN_SPEED = 80;

int lastTurn = 0;

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 5; i++) pinMode(sensorPins[i], INPUT);
  motorL.begin();
  motorR.begin();
}

void loop() {
  bool s0 = analogRead(sensorPins[0]) > THRESHOLD;
  bool s1 = analogRead(sensorPins[1]) > THRESHOLD;
  bool s2 = analogRead(sensorPins[2]) > THRESHOLD;
  bool s3 = analogRead(sensorPins[3]) > THRESHOLD;
  bool s4 = analogRead(sensorPins[4]) > THRESHOLD;

  if (s2) {
    motorL.setSpeed(BASE_SPEED);
    motorR.setSpeed(BASE_SPEED);
    lastTurn = 0;
  } else if (s1) {
    motorL.setSpeed(BASE_SPEED - TURN_SPEED);
    motorR.setSpeed(BASE_SPEED);
    lastTurn = -1;
  } else if (s0) {
    motorL.setSpeed(0);
    motorR.setSpeed(BASE_SPEED);
    lastTurn = -1;
  } else if (s3) {
    motorL.setSpeed(BASE_SPEED);
    motorR.setSpeed(BASE_SPEED - TURN_SPEED);
    lastTurn = 1;
  } else if (s4) {
    motorL.setSpeed(BASE_SPEED);
    motorR.setSpeed(0);
    lastTurn = 1;
  } else if (lastTurn < 0) {
    motorL.setSpeed(-BASE_SPEED);
    motorR.setSpeed(BASE_SPEED);
  } else if (lastTurn > 0) {
    motorL.setSpeed(BASE_SPEED);
    motorR.setSpeed(-BASE_SPEED);
  } else {
    motorL.setSpeed(BASE_SPEED);
    motorR.setSpeed(BASE_SPEED);
  }

  Serial.printf("s=%d%d%d%d%d lastTurn=%d\n", s0, s1, s2, s3, s4, lastTurn);
}
