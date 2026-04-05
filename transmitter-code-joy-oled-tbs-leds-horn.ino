#include <SPI.h>
#include <RF24.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// RF
RF24 radio(9, 10);
const byte txAddr[6] = "NODE1";
const byte rxAddr[6] = "NODE2";

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

// JOYSTICK
const int joyX = A1;
const int joyY = A0;
const int joySW = 3;

// SWITCH
const int switchPin = 2;

unsigned long lastReceiveTime = 0;

void setup() {
  pinMode(switchPin, INPUT_PULLUP);
  pinMode(joySW, INPUT_PULLUP);

  Wire.begin();
  Wire.setClock(100000);

  // OLED INIT
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true)
      ;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("INIT OK");
  display.display();
  delay(500);

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
}

void loop() {
  int x = analogRead(joyX);
  int y = analogRead(joyY);

  static int lastX = 512, lastY = 512;

  x = (x + lastX) / 2;
  y = (y + lastY) / 2;

  lastX = x;
  lastY = y;

  if (lastX > 480 && lastX < 540) lastX = 512;
  if (lastY > 480 && lastY < 540) lastY = 512;

  txData.speed = map(lastY, 0, 1023, -255, 255);
  txData.turn = map(lastX, 0, 1023, -255, 255);

  txData.lights = (digitalRead(switchPin) == LOW);
  txData.horn = (digitalRead(joySW) == LOW);

  bool ok = radio.write(&txData, sizeof(txData));
  delay(5);

  if (ok) {
    if (radio.isAckPayloadAvailable()) {
      radio.read(&rxData, sizeof(rxData));
      lastReceiveTime = millis();
    }
  }

  bool signalLost = (millis() - lastReceiveTime > 500);

  display.clearDisplay();

  display.setCursor(0, 0);
  display.print("Battery: ");
  display.print(rxData.battery, 2);
  display.println(" V");

  display.setCursor(0, 20);
  display.print("Signal: ");
  display.println((ok && !signalLost) ? String(rxData.signal) + "%" : "Lost");

  display.setCursor(0, 40);
  display.print("Horn: ");
  display.println(txData.horn ? "ON" : "OFF");

  display.display();

  delay(50);
}
