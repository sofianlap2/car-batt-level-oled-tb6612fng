#include <SPI.h>
#include <RF24.h>

RF24 radio(9, 10);

const byte txAddr[6] = "NODE1";
const byte rxAddr[6] = "NODE2";

#define BAT_PIN A2

// ===== DATA =====
struct TXData {
  int speed;
  int turn;
  bool lights;
};

struct RXData {
  float battery;
  int signal;
};

TXData txData;
RXData rxData;

// ===== MOTOR PINS =====
const int AIN1 = 5;
const int AIN2 = 4;
const int PWMA = 3;

const int BIN1 = 6;
const int BIN2 = 7;
const int PWMB = 8;

// ===== LEDS =====
const int FRONT_LED = 2;
const int BACK_LED  = A3;

unsigned long lastReceiveTime = 0;

// ===== BATTERY =====
float readBattery() {
  static float v = 0;

  int raw = analogRead(BAT_PIN);
  float newV = raw * (4.9 / 1023.0) * 2.0;

  v = (v * 0.8) + (newV * 0.2);

  return v;
}

void setup() {
  pinMode(FRONT_LED, OUTPUT);
  pinMode(BACK_LED, OUTPUT);

  digitalWrite(FRONT_LED, LOW);
  digitalWrite(BACK_LED, LOW);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);

  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(PWMB, OUTPUT);

  radio.begin();
  radio.setChannel(108);
  radio.setDataRate(RF24_250KBPS);

  radio.setPALevel(RF24_PA_LOW);
  radio.setRetries(5, 15);
  radio.enableAckPayload();
  radio.enableDynamicPayloads();

  radio.openReadingPipe(1, txAddr);
  radio.openWritingPipe(rxAddr);

  radio.startListening();
}

void loop() {
  if (radio.available()) {
    radio.read(&txData, sizeof(txData));
    lastReceiveTime = millis();

    // ===== MOTOR =====
    int leftMotor  = txData.speed + txData.turn;
    int rightMotor = txData.speed - txData.turn;

    leftMotor  = constrain(leftMotor, -255, 255);
    rightMotor = constrain(rightMotor, -255, 255);

    controlMotor(AIN1, AIN2, PWMA, leftMotor);
    controlMotor(BIN1, BIN2, PWMB, rightMotor);

    // ===== LIGHTS =====
    digitalWrite(FRONT_LED, txData.lights ? HIGH : LOW);
    digitalWrite(BACK_LED,  txData.lights ? HIGH : LOW);

    // ===== RESPONSE =====
    rxData.battery = readBattery();

    static int successCount = 0;
    successCount++;
    if (successCount > 100) successCount = 100;

    rxData.signal = successCount;

    radio.writeAckPayload(1, &rxData, sizeof(rxData));
  }

  // ===== FAILSAFE =====
  if (millis() - lastReceiveTime > 500) {
    controlMotor(AIN1, AIN2, PWMA, 0);
    controlMotor(BIN1, BIN2, PWMB, 0);

    digitalWrite(FRONT_LED, LOW);
    digitalWrite(BACK_LED, LOW);

    rxData.battery = readBattery();
    rxData.signal = 0;

    radio.writeAckPayload(1, &rxData, sizeof(rxData));
  }
}

// ===== MOTOR =====
void controlMotor(int in1, int in2, int pwmPin, int value) {
  int speed = abs(value);

  if (value > 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  } else if (value < 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  } else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }

  analogWrite(pwmPin, speed);
}
