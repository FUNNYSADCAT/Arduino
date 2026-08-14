/*
  หุ่นยนต์เดินตามเส้น (Line Follower)
  บอร์ด: ESP32
  เซนเซอร์: IR 5 ตัว (digital output, ยิ่งดำยิ่งอ่านค่าตามลักษณะเซนเซอร์ที่ใช้)

  พินเซนเซอร์ (ตามที่ให้มา):
  D5 = GPIO33  (ขวาสุด)
  D4 = GPIO25
  D3 = GPIO26  (กลาง)
  D2 = GPIO16
  D1 = GPIO17  (ซ้ายสุด)

  พินมอเตอร์ (2 พินต่อข้าง, PWM ทั้งคู่, ไม่มีพิน enable แยก):
  motorL(13, 12)  ล้อซ้าย
  motorR(27, 14)  ล้อขวา
*/

// ---------- พินเซนเซอร์ ----------
const int sensorPin[5] = {17, 16, 26, 25, 33}; // ซ้ายสุด -> ขวาสุด (D1..D5)

// ---------- คลาสควบคุมมอเตอร์ (2 พิน PWM ต่อข้าง) ----------
class Motor {
  public:
    Motor(int p1, int p2) : pin1(p1), pin2(p2) {}

    void begin() {
      pinMode(pin1, OUTPUT);
      pinMode(pin2, OUTPUT);
    }

    // speed: -255 ถึง 255, บวก = เดินหน้า, ลบ = ถอยหลัง
    void setSpeed(int speed) {
      speed = constrain(speed, -255, 255);
      if (speed >= 0) {
        analogWrite(pin1, speed);
        analogWrite(pin2, 0);
      } else {
        analogWrite(pin1, 0);
        analogWrite(pin2, -speed);
      }
    }

    void stop() {
      analogWrite(pin1, 0);
      analogWrite(pin2, 0);
    }

  private:
    int pin1, pin2;
};

Motor motorL(13, 12);
Motor motorR(27, 14);

// ---------- ค่ากำหนดการทำงาน ----------
const bool SENSOR_ACTIVE_LOW = true; // true ถ้าเซนเซอร์ให้ LOW เมื่อเจอเส้นดำ, false ถ้าตรงกันข้าม
const int BASE_SPEED = 150;  // ความเร็วพื้นฐาน (0-255)
const int TURN_SPEED = 200;  // ความเร็วตอนเลี้ยวแรง (เจอทางแยก/เส้นหลุด)

int sensorValue[5];

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 5; i++) {
    pinMode(sensorPin[i], INPUT);
  }

  motorL.begin();
  motorR.begin();

  stopMotors();
}

void loop() {
  readSensors();
  followLine();
}

void readSensors() {
  for (int i = 0; i < 5; i++) {
    int raw = digitalRead(sensorPin[i]);
    sensorValue[i] = SENSOR_ACTIVE_LOW ? !raw : raw; // แปลงให้ 1 = เจอเส้นดำเสมอ
  }
}

void followLine() {
  // sensorValue[0..4] = ซ้ายสุด, ซ้าย, กลาง, ขวา, ขวาสุด
  int s0 = sensorValue[0];
  int s1 = sensorValue[1];
  int s2 = sensorValue[2];
  int s3 = sensorValue[3];
  int s4 = sensorValue[4];

  if (s2 == 1 && s1 == 0 && s3 == 0) {
    // เส้นอยู่ตรงกลาง วิ่งตรง
    moveMotors(BASE_SPEED, BASE_SPEED);
  }
  else if (s1 == 1 && s0 == 0) {
    // เส้นเบี้ยวไปทางซ้ายเล็กน้อย เลี้ยวซ้ายเบาๆ
    moveMotors(BASE_SPEED - 60, BASE_SPEED);
  }
  else if (s3 == 1 && s4 == 0) {
    // เส้นเบี้ยวไปทางขวาเล็กน้อย เลี้ยวขวาเบาๆ
    moveMotors(BASE_SPEED, BASE_SPEED - 60);
  }
  else if (s0 == 1) {
    // เส้นอยู่ซ้ายสุด เลี้ยวซ้ายแรง
    moveMotors(-TURN_SPEED, TURN_SPEED);
  }
  else if (s4 == 1) {
    // เส้นอยู่ขวาสุด เลี้ยวขวาแรง
    moveMotors(TURN_SPEED, -TURN_SPEED);
  }
  else {
    // ไม่เจอเส้นเลย (หลุดเส้น) หยุดรอ
    stopMotors();
  }
}

// speedLeft, speedRight: ค่าบวก = เดินหน้า, ค่าลบ = ถอยหลัง, ช่วง -255 ถึง 255
void moveMotors(int speedLeft, int speedRight) {
  motorL.setSpeed(speedLeft);
  motorR.setSpeed(speedRight);
}

void stopMotors() {
  motorL.stop();
  motorR.stop();
}
