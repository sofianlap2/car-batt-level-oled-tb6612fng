#include <SPI.h>
#include <RF24.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===== OLED =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ===== RF =====
RF24 radio(9, 10);
const byte txAddr[6] = "NODE1";
const byte rxAddr[6] = "NODE2";

// ===== DATA =====
struct TXData {
  int speed;
  int turn;
};

struct RXData {
  float battery;
  int signal;
};

TXData txData;
RXData rxData;

// ===== JOYSTICK =====
const int joyX = A1;
const int joyY = A0;

unsigned long lastReceiveTime = 0;

void setup() {
  radio.begin();
  radio.setChannel(108);
  radio.setDataRate(RF24_250KBPS);

  radio.setPALevel(RF24_PA_LOW);
  radio.setRetries(5, 15);
  radio.enableAckPayload();
  radio.enableDynamicPayloads();

  radio.openWritingPipe(txAddr);
  radio.openReadingPipe(1, rxAddr);

  radio.stopListening();

  // OLED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
}

void loop() {
  int x = analogRead(joyX);
  int y = analogRead(joyY);

  static int lastX = 512, lastY = 512;

  x = (x + lastX) / 2;
  y = (y + lastY) / 2;

  lastX = x;
  lastY = y;

  // Dead zone
  if (lastX > 480 && lastX < 540) lastX = 512;
  if (lastY > 480 && lastY < 540) lastY = 512;

  txData.speed = map(lastY, 0, 1023, -255, 255);
  txData.turn  = map(lastX, 0, 1023, -255, 255);

  // SEND
  bool ok = radio.write(&txData, sizeof(txData));

  // RECEIVE ACK PAYLOAD (IMPORTANT)
  if (radio.available()) {
    radio.read(&rxData, sizeof(rxData));
    lastReceiveTime = millis();
  }

  bool signalLost = (millis() - lastReceiveTime > 500);

  // DISPLAY
  display.clearDisplay();

  display.setCursor(0, 0);
  display.print("Battery: ");
  display.print(rxData.battery, 2);
  display.println(" V");

  display.setCursor(0, 20);
  display.print("Signal: ");

  if (ok && !signalLost) {
    display.print(rxData.signal);
    display.println("%");
  } else {
    display.println("Lost");
  }

  display.display();

  delay(50);
}