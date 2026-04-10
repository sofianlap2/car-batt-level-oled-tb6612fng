#include <SPI.h>
#include <RF24.h>

RF24 radio(9, 10);

const byte txAddr[6] = "NODE1";
const byte rxAddr[6] = "NODE2";

#define BAT_PIN A3

// DATA
struct TXData {
  int speed;
  int turn;
  bool lights;
  bool horn; 
};

struct RXData {
  float battery;
  int signal;
};

TXData txData;
RXData rxData;

// MOTOR
const int AIN1 = 7;
const int AIN2 = 8;
const int PWMA = 6;

const int BIN1 = 5;
const int BIN2 = 4;
const int PWMB = 3;

// LEDS
const int FRONT_LED = A5;
const int BACK_LED = A2;

// BUZZER
const int BUZZER = A4;

unsigned long lastReceiveTime = 0;

// BATTERY
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
  pinMode(BUZZER, OUTPUT);

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

    int leftMotor = txData.speed + txData.turn;
    int rightMotor = txData.speed - txData.turn;

    controlMotor(AIN1, AIN2, PWMA, leftMotor);
    controlMotor(BIN1, BIN2, PWMB, rightMotor);

    digitalWrite(FRONT_LED, txData.lights);
    digitalWrite(BACK_LED, txData.lights);

    if (txData.horn) {
      tone(BUZZER, 2000);
    } else {
      noTone(BUZZER);
    }

    rxData.battery = readBattery();
    rxData.signal = 100;

    radio.writeAckPayload(1, &rxData, sizeof(rxData));
  }

  // FAILSAFE
  if (millis() - lastReceiveTime > 500) {
    stopMotor(AIN1, AIN2, PWMA);
    stopMotor(BIN1, BIN2, PWMB);

    digitalWrite(FRONT_LED, LOW);
    digitalWrite(BACK_LED, LOW);
    noTone(BUZZER);

    rxData.signal = 0;
  }
}

// 🔥 NEW CONTROL (NO PWM)
void controlMotor(int in1, int in2, int pwmPin, int value) {

  if (value > 100) {  // FORWARD threshold
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    digitalWrite(pwmPin, HIGH);
  } 
  else if (value < -100) { // BACKWARD threshold
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    digitalWrite(pwmPin, HIGH);
  } 
  else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    digitalWrite(pwmPin, LOW);
  }
}

void stopMotor(int in1, int in2, int pwmPin) {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(pwmPin, LOW);
}
