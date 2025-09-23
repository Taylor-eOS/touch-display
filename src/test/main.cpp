#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

static const int TFT_MOSI_PIN = 23;
static const int TFT_MISO_PIN = 19;
static const int TFT_SCLK_PIN = 18;
static const int TFT_CS_PIN   = 5;
static const int TFT_DC_PIN   = 16;
static const int TFT_RST_PIN  = 15;
static const int TFT_LED_PIN  = 4;
static const int TOUCH_MOSI_PIN = TFT_MOSI_PIN;
static const int TOUCH_MISO_PIN = TFT_MISO_PIN;
static const int TOUCH_CLK_PIN  = TFT_SCLK_PIN;
static const int TOUCH_CS_PIN   = 21;
static const int TOUCH_IRQ_PIN  = 22;
static const uint16_t SCREEN_W = 320;
static const uint16_t SCREEN_H = 240;
static const int TS_MINX = 200;
static const int TS_MAXX = 3900;
static const int TS_MINY = 200;
static const int TS_MAXY = 3900;

SPIClass spi;
Adafruit_ILI9341 tft(&spi, TFT_DC_PIN, TFT_CS_PIN, TFT_RST_PIN);
XPT2046_Touchscreen ts(TOUCH_CS_PIN, TOUCH_IRQ_PIN);

unsigned long lastTouchMs = 0;
int lastX = -1;
int lastY = -1;

void drawTestPattern() {
  tft.fillScreen(ILI9341_BLACK);
  for (int i = 0; i < 8; ++i) {
    uint16_t c = tft.color565((i & 1) ? 255 : 0, (i & 2) ? 255 : 0, (i & 4) ? 255 : 0);
    int x0 = (SCREEN_W / 8) * i;
    tft.fillRect(x0, 0, SCREEN_W / 8, SCREEN_H / 2, c);
  }
  tft.setRotation(1);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(6, SCREEN_H / 2 + 6);
  tft.print("ILI9341 + XPT2046 test");
  tft.setCursor(6, SCREEN_H / 2 + 30);
  tft.print("Touch to see coords");
}

void showTouchCross(int x, int y) {
  static const uint16_t crossCol = ILI9341_YELLOW;
  static const uint16_t bgCol = ILI9341_BLACK;
  static int prevX = -1;
  static int prevY = -1;
  if (prevX != -1) {
    tft.drawLine(prevX - 10, prevY, prevX + 10, prevY, bgCol);
    tft.drawLine(prevX, prevY - 10, prevX, prevY + 10, bgCol);
    tft.fillRect(0, SCREEN_H - 40, SCREEN_W, 40, bgCol);
  }
  tft.drawLine(x - 10, y, x + 10, y, crossCol);
  tft.drawLine(x, y - 10, x, y + 10, crossCol);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(6, SCREEN_H - 34);
  tft.print("raw:");
  tft.print(lastX);
  tft.print(",");
  tft.print(lastY);
  prevX = x;
  prevY = y;
}

void setup() {
  Serial.begin(115200);
  spi.begin(TFT_SCLK_PIN, TFT_MISO_PIN, TFT_MOSI_PIN, TFT_CS_PIN);
  tft.begin();
  pinMode(TFT_LED_PIN, OUTPUT);
  digitalWrite(TFT_LED_PIN, HIGH);
  ts.begin(spi);
  ts.setRotation(1);
  drawTestPattern();
}

void loop() {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    lastX = p.x;
    lastY = p.y;
    int tx = map(p.x, TS_MINX, TS_MAXX, 0, SCREEN_W);
    int ty = map(p.y, TS_MINY, TS_MAXY, 0, SCREEN_H);
    tx = constrain(tx, 0, SCREEN_W - 1);
    ty = constrain(ty, 0, SCREEN_H - 1);
    unsigned long now = millis();
    if (now - lastTouchMs > 80) {
      Serial.print("RAW X=");
      Serial.print(p.x);
      Serial.print(" Y=");
      Serial.print(p.y);
      Serial.print(" -> X=");
      Serial.print(tx);
      Serial.print(" Y=");
      Serial.println(ty);
      showTouchCross(tx, ty);
      lastTouchMs = now;
    }
  } else {
    if (lastX != -1) {
      lastX = -1;
      lastY = -1;
    }
  }
  delay(10);
}

