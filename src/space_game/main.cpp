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

struct Star {
  float x, y, z;
  uint16_t color;
};

struct GameObject {
  float x, y, z;
  uint8_t type;
  bool active;
  bool hasCollided;
};

struct Player {
  float x, y;
  float vx, vy;
  int fuel;
  int score;
  bool thrusting;
};

static const int MAX_STARS = 80;
static const int MAX_OBJECTS = 12;
Star stars[MAX_STARS];
GameObject objects[MAX_OBJECTS];
Player player;
unsigned long lastUpdate = 0;
unsigned long lastTouchMs = 0;
int touchX = -1, touchY = -1;
bool gameRunning = true;
float gameSpeed = 0.7f;

void initStars() {
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].x = random(-1000, 1000);
    stars[i].y = random(-1000, 1000);
    stars[i].z = random(1, 1000);
    int brightness = random(100, 255);
    stars[i].color = tft.color565(brightness, brightness, brightness);
  }
}

void initObjects() {
  for (int i = 0; i < MAX_OBJECTS; i++) {
    objects[i].active = false;
    objects[i].hasCollided = false;
  }
}

void initPlayer() {
  player.x = SCREEN_W / 2;
  player.y = SCREEN_H / 2;
  player.vx = 0;
  player.vy = 0;
  player.fuel = 2000;
  player.score = 0;
  player.thrusting = false;
}

void drawPlayerExplosion() {
  tft.fillCircle(player.x, player.y, 25, ILI9341_ORANGE);
  tft.fillCircle(player.x, player.y, 15, ILI9341_YELLOW);
  tft.fillCircle(player.x, player.y, 8, ILI9341_WHITE);
  for (int i = 0; i < 12; i++) {
    float angle = i * PI / 6;
    int len = random(15, 35);
    int x2 = player.x + cos(angle) * len;
    int y2 = player.y + sin(angle) * len;
    tft.drawLine(player.x, player.y, x2, y2, ILI9341_ORANGE);
  }
}

void spawnObject() {
  for (int i = 0; i < MAX_OBJECTS; i++) {
    if (!objects[i].active) {
      objects[i].x = random(-200, 200);
      objects[i].y = random(-200, 200);
      objects[i].z = random(500, 800);
      objects[i].type = random(0, 3);
      objects[i].active = true;
      objects[i].hasCollided = false;
      break;
    }
  }
}

void updateStars(float deltaTime) {
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].z -= gameSpeed * 200 * deltaTime;
    if (stars[i].z <= 0) {
      stars[i].x = random(-1000, 1000);
      stars[i].y = random(-1000, 1000);
      stars[i].z = random(800, 1000);
    }
  }
}

void updateObjects(float deltaTime) {
  for (int i = 0; i < MAX_OBJECTS; i++) {
    if (objects[i].active) {
      objects[i].z -= gameSpeed * 80 * deltaTime;
      if (objects[i].z <= 0) {
        objects[i].active = false;
      }
    }
  }
  if (random(0, 100) < 5) {
    spawnObject();
  }
}

void updatePlayer(float deltaTime) {
  if (touchX >= 0 && touchY >= 0) {
    float dx = touchX - player.x;
    float dy = touchY - player.y;
    float distance = sqrt(dx * dx + dy * dy);
    if (distance > 5) {
      player.x += dx * 4.0f * deltaTime;
      player.y += dy * 4.0f * deltaTime;
      player.thrusting = true;
      player.fuel -= (int)(20 * deltaTime);
    }
  } else {
    player.thrusting = false;
  }
  player.x = constrain(player.x, 10, SCREEN_W - 10);
  player.y = constrain(player.y, 10, SCREEN_H - 10);
  if (player.fuel <= 0) {
    gameRunning = false;
  }
}

void checkCollisions() {
  for (int i = 0; i < MAX_OBJECTS; i++) {
    if (!objects[i].active || objects[i].hasCollided) continue;
    int screenX = (objects[i].x * 100 / objects[i].z) + SCREEN_W / 2;
    int screenY = (objects[i].y * 100 / objects[i].z) + SCREEN_H / 2;
    int size = constrain(20 - (int)(objects[i].z / 40), 3, 15);
    if (size >= 10) {
      if (abs(screenX - (int)player.x) < 25 && abs(screenY - (int)player.y) < 25) {
        objects[i].hasCollided = true;
        objects[i].active = false;
        tft.fillCircle(player.x, player.y, 20, ILI9341_ORANGE);
        tft.fillCircle(player.x, player.y, 12, ILI9341_YELLOW);
        delay(150);
        if (objects[i].type == 0) {
          player.fuel += 300;
          player.score += 10;
        } else if (objects[i].type == 1) {
          player.score += 50;
        } else {
          player.fuel -= 50;
          if (player.fuel < 0) player.fuel = 0;
        }
      }
    }
  }
}

void drawStars() {
  for (int i = 0; i < MAX_STARS; i++) {
    if (stars[i].z > 0) {
      int screenX = (stars[i].x * 100 / stars[i].z) + SCREEN_W / 2;
      int screenY = (stars[i].y * 100 / stars[i].z) + SCREEN_H / 2;
      if (screenX >= 0 && screenX < SCREEN_W && screenY >= 0 && screenY < SCREEN_H) {
        int size = constrain(3 - (int)(stars[i].z / 300), 1, 3);
        if (size == 1) {
          tft.drawPixel(screenX, screenY, stars[i].color);
        } else {
          tft.fillRect(screenX - size/2, screenY - size/2, size, size, stars[i].color);
        }
      }
    }
  }
}

void drawObjects() {
  for (int i = 0; i < MAX_OBJECTS; i++) {
    if (!objects[i].active) continue;
    int screenX = (objects[i].x * 100 / objects[i].z) + SCREEN_W / 2;
    int screenY = (objects[i].y * 100 / objects[i].z) + SCREEN_H / 2;
    if (screenX >= -20 && screenX < SCREEN_W + 20 && screenY >= -20 && screenY < SCREEN_H + 20) {
      int size = constrain(20 - (int)(objects[i].z / 40), 3, 15);
      uint16_t color;
      if (objects[i].type == 0) {
        color = ILI9341_GREEN;
        tft.fillCircle(screenX, screenY, size, color);
      } else if (objects[i].type == 1) {
        color = ILI9341_CYAN;
        tft.fillRect(screenX - size, screenY - size, size * 2, size * 2, color);
      } else {
        color = ILI9341_RED;
        for (int j = 0; j < 6; j++) {
          float angle = j * PI / 3;
          int x1 = screenX + cos(angle) * size;
          int y1 = screenY + sin(angle) * size;
          int x2 = screenX + cos(angle + PI/3) * size;
          int y2 = screenY + sin(angle + PI/3) * size;
          tft.drawLine(screenX, screenY, x1, y1, color);
          tft.drawLine(x1, y1, x2, y2, color);
        }
      }
    }
  }
}

void drawPlayer() {
  uint16_t shipColor = ILI9341_YELLOW;
  if (player.thrusting) {
    shipColor = ILI9341_ORANGE;
  }
  tft.fillTriangle(
    player.x, player.y - 8,
    player.x - 6, player.y + 6,
    player.x + 6, player.y + 6,
    shipColor
  );
  if (player.thrusting) {
    tft.fillTriangle(
      player.x - 3, player.y + 6,
      player.x + 3, player.y + 6,
      player.x, player.y + 12,
      ILI9341_RED
    );
  }
}

void drawHUD() {
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(5, 5);
  tft.print("Fuel: ");
  tft.print(player.fuel);
  tft.setCursor(5, 15);
  tft.print("Score: ");
  tft.print(player.score);
  int barWidth = 80;
  int fuelWidth = (player.fuel * barWidth) / 2000;
  tft.drawRect(SCREEN_W - barWidth - 10, 5, barWidth, 8, ILI9341_WHITE);
  if (fuelWidth > 0) {
    uint16_t fuelColor = player.fuel > 400 ? ILI9341_GREEN : ILI9341_RED;
    tft.fillRect(SCREEN_W - barWidth - 9, 6, fuelWidth - 2, 6, fuelColor);
  }
}

void drawGameOver() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(3);
  tft.setTextColor(ILI9341_RED);
  tft.setCursor(80, 80);
  tft.print("GAME OVER");
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(90, 120);
  tft.print("Final Score:");
  tft.setCursor(120, 140);
  tft.print(player.score);
  tft.setTextSize(1);
  tft.setCursor(70, 180);
  tft.print("Touch screen to restart");
}

void handleTouch() {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    int tx = map(p.x, TS_MINX, TS_MAXX, 0, SCREEN_W);
    int ty = map(p.y, TS_MINY, TS_MAXY, 0, SCREEN_H);
    tx = constrain(tx, 0, SCREEN_W - 1);
    ty = constrain(ty, 0, SCREEN_H - 1);
    unsigned long now = millis();
    if (now - lastTouchMs > 80) {
      if (gameRunning) {
        touchX = tx;
        touchY = ty;
      } else {
        gameRunning = true;
        initPlayer();
        initObjects();
      }
      lastTouchMs = now;
    }
  } else {
    touchX = -1;
    touchY = -1;
  }
}

void setup() {
  Serial.begin(115200);
  spi.begin(TFT_SCLK_PIN, TFT_MISO_PIN, TFT_MOSI_PIN, TFT_CS_PIN);
  tft.begin();
  pinMode(TFT_LED_PIN, OUTPUT);
  digitalWrite(TFT_LED_PIN, HIGH);
  ts.begin(spi);
  ts.setRotation(1);
  tft.setRotation(1);
  randomSeed(analogRead(0));
  initStars();
  initObjects();
  initPlayer();
}

void loop() {
  unsigned long now = millis();
  float deltaTime = (now - lastUpdate) / 1000.0f;
  lastUpdate = now;
  handleTouch();
  if (gameRunning) {
    updateStars(deltaTime);
    updateObjects(deltaTime);
    updatePlayer(deltaTime);
    checkCollisions();
    tft.fillScreen(ILI9341_BLACK);
    drawStars();
    drawObjects();
    drawPlayer();
    drawHUD();
    gameSpeed += deltaTime * 0.04f;
  } else {
    drawGameOver();
  }
  delay(10);
}

