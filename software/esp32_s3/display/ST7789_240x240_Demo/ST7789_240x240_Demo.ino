/**
 * ESP32-S3 + 1.3inch TFT SPI Display Demo
 * Driver  : ST7789   (240x240, no CS pin, 7-pin)
 * Library : LovyanGFX (latest)
 *
 * Pin Assignments
 *  MOSI  GPIO11
 *  SCK   GPIO12
 *  MISO  GPIO13  (connected but unused)
 *  CS    none    (pin_cs = -1)
 *  DC    GPIO41
 *  RESET GPIO42
 *  BL    tie to 3.3V
 */

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// ===== LovyanGFX configuration =====
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel_instance;
  lgfx::Bus_SPI      _bus_instance;

public:
  LGFX() {
    // --- SPI bus ---
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host    = SPI2_HOST;
      cfg.spi_mode    = 3;          // ST7789 works best in mode 3
      cfg.freq_write  = 40000000;   // ST7789 supports up to 62 MHz; 40 MHz is safe
      cfg.freq_read   = 16000000;
      cfg.spi_3wire   = false;
      cfg.use_lock    = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk    = 12;
      cfg.pin_mosi    = 11;
      cfg.pin_miso    = 13;
      cfg.pin_dc      = 41;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    // --- Panel ---
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs           = -1;    // no CS pin on this 7-pin module
      cfg.pin_rst          = 42;
      cfg.pin_busy         = -1;
      cfg.panel_width      = 240;
      cfg.panel_height     = 240;
      // ST7789 240x240: the physical RAM is 240x320, top 80 rows are hidden
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;    // critical for 240x240 modules
      cfg.offset_rotation  = 2;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable         = false;
      cfg.invert           = true;  // most ST7789 240x240 modules need invert
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = false;
      _panel_instance.config(cfg);
    }

    setPanel(&_panel_instance);
  }
};

static LGFX        tft;
static LGFX_Sprite canvas(&tft);   // double-buffer sprite (240x240)

// ===== Demo management =====
static const int      DEMO_COUNT    = 5;
static int            demoIndex     = 0;
static uint32_t       demoTimer     = 0;
static const uint32_t DEMO_DURATION = 6000;

static inline float frand(float lo, float hi) {
  return lo + (float)random(0, 10000) / 10000.0f * (hi - lo);
}

// ===== Demo 0 : Bouncing Balls =====
struct Ball { float x, y, vx, vy, r; uint16_t color; };
static const int BALL_COUNT = 12;
static Ball balls[BALL_COUNT];

void initBalls() {
  for (int i = 0; i < BALL_COUNT; i++) {
    balls[i].r     = frand(8, 20);
    balls[i].x     = frand(balls[i].r, 240 - balls[i].r);
    balls[i].y     = frand(balls[i].r, 240 - balls[i].r);
    balls[i].vx    = frand(1.5f, 4.0f) * (random(2) ? 1 : -1);
    balls[i].vy    = frand(1.5f, 4.0f) * (random(2) ? 1 : -1);
    balls[i].color = (uint16_t)canvas.color888(random(80,255), random(80,255), random(80,255));
  }
}

void demoBalls() {
  canvas.fillScreen(TFT_BLACK);
  for (int i = 0; i < BALL_COUNT; i++) {
    Ball &b = balls[i];
    b.x += b.vx; b.y += b.vy;
    if (b.x - b.r < 0)   { b.x = b.r;       b.vx =  fabsf(b.vx); }
    if (b.x + b.r > 240) { b.x = 240 - b.r; b.vx = -fabsf(b.vx); }
    if (b.y - b.r < 0)   { b.y = b.r;       b.vy =  fabsf(b.vy); }
    if (b.y + b.r > 240) { b.y = 240 - b.r; b.vy = -fabsf(b.vy); }
    canvas.fillCircle((int)b.x, (int)b.y, (int)b.r, b.color);
    canvas.fillCircle((int)b.x - (int)(b.r*0.3f),
                      (int)b.y - (int)(b.r*0.3f),
                      max(2,(int)(b.r*0.25f)), TFT_WHITE);
  }
}

// ===== Demo 1 : Scrolling Sine Waves =====
static float waveOffset = 0;

void demoWave() {
  canvas.fillScreen(TFT_BLACK);
  for (int y = 0; y < 240; y += 40)
    canvas.drawFastHLine(0, y, 240, (uint16_t)canvas.color888(20, 20, 60));
  for (int x = 0; x < 240; x += 40)
    canvas.drawFastVLine(x, 0, 240, (uint16_t)canvas.color888(20, 20, 60));
  for (int layer = 0; layer < 3; layer++) {
    float   amp  = 55.0f - layer * 14.0f;
    float   freq = 0.027f + layer * 0.011f;
    uint8_t r    = (layer == 0) ?   0 : (layer == 1 ?   0 : 100);
    uint8_t g    = (layer == 0) ? 200 : (layer == 1 ? 100 : 200);
    uint8_t b    = (layer == 0) ? 255 : (layer == 1 ? 255 :  50);
    uint16_t col = (uint16_t)canvas.color888(r, g, b);
    int prevY = 120 + (int)(amp * sinf(freq * 0 + waveOffset * (1 + layer * 0.3f)));
    for (int x = 1; x < 240; x++) {
      int curY = 120 + (int)(amp * sinf(freq * x + waveOffset * (1 + layer * 0.3f)));
      canvas.drawLine(x-1, prevY, x, curY, col);
      prevY = curY;
    }
  }
  waveOffset += 0.08f;
}

// ===== Demo 2 : Rotating Gears =====
static float gearAngle = 0;

void drawPolygon(LGFX_Sprite &spr, int cx, int cy, int r, int sides,
                 float angle, uint16_t color) {
  float step = 2.0f * M_PI / sides;
  int px = cx + (int)(r * cosf(angle));
  int py = cy + (int)(r * sinf(angle));
  for (int i = 1; i <= sides; i++) {
    float a = angle + step * i;
    int nx = cx + (int)(r * cosf(a));
    int ny = cy + (int)(r * sinf(a));
    spr.drawLine(px, py, nx, ny, color);
    px = nx; py = ny;
  }
}

void demoGears() {
  canvas.fillScreen((uint16_t)canvas.color888(5, 5, 20));
  struct GearSet { int cx, cy, count; float dir; uint16_t color; };
  GearSet gs[] = {
    {120, 120, 6,  1.0f, (uint16_t)canvas.color888(255, 100,   0)},
    { 55,  55, 8, -0.7f, (uint16_t)canvas.color888(  0, 200, 255)},
    {185, 175, 5,  0.9f, (uint16_t)canvas.color888(200,  50, 200)},
  };
  for (auto &g : gs) {
    for (int r = 10; r <= 48; r += 8)
      drawPolygon(canvas, g.cx, g.cy, r, g.count,
                  gearAngle * g.dir + r * 0.05f, g.color);
    for (int i = 0; i < g.count; i++) {
      float a = gearAngle * g.dir + i * 2.0f * M_PI / g.count;
      canvas.drawLine(g.cx, g.cy,
                      g.cx + (int)(48 * cosf(a)),
                      g.cy + (int)(48 * sinf(a)), g.color);
    }
  }
  gearAngle += 0.04f;
}

// ===== Demo 3 : Lissajous Curve =====
static float    lissPhase = 0;
static const int TRACE_LEN = 300;
static int16_t   traceX[TRACE_LEN], traceY[TRACE_LEN];
static int       traceHead = 0;

void initLissajous() {
  memset(traceX, 0, sizeof(traceX));
  memset(traceY, 0, sizeof(traceY));
  traceHead = 0;
}

void demoLissajous() {
  canvas.fillScreen(TFT_BLACK);
  traceX[traceHead] = 120 + (int)(110 * sinf(3.0f * lissPhase + 0.5f));
  traceY[traceHead] = 120 + (int)(110 * sinf(2.0f * lissPhase));
  traceHead = (traceHead + 1) % TRACE_LEN;
  for (int i = 0; i < TRACE_LEN - 1; i++) {
    int   a     = (traceHead + i)     % TRACE_LEN;
    int   b     = (traceHead + i + 1) % TRACE_LEN;
    float ratio = (float)i / TRACE_LEN;
    uint8_t r  = (uint8_t)(ratio * 255);
    uint8_t g  = (uint8_t)(sinf(ratio * M_PI) * 255);
    uint8_t bl = (uint8_t)((1 - ratio) * 255);
    canvas.drawLine(traceX[a], traceY[a], traceX[b], traceY[b],
                    (uint16_t)canvas.color888(r, g, bl));
  }
  lissPhase += 0.025f;
}

// ===== Demo 4 : Starfield =====
struct Star { float x, y, z; };
static const int STAR_COUNT = 120;
static Star      stars[STAR_COUNT];

void initStars() {
  for (int i = 0; i < STAR_COUNT; i++) {
    stars[i].x = frand(-120, 120);
    stars[i].y = frand(-120, 120);
    stars[i].z = frand(1, 240);
  }
}

void demoStarfield() {
  canvas.fillScreen(TFT_BLACK);
  for (int i = 0; i < STAR_COUNT; i++) {
    Star &s = stars[i];
    s.z -= 3.0f;
    if (s.z <= 0) { s.x = frand(-120,120); s.y = frand(-120,120); s.z = 240; }
    float    sx     = s.x / s.z * 240 + 120;
    float    sy     = s.y / s.z * 240 + 120;
    float    size   = (1.0f - s.z / 240.0f) * 4.0f;
    uint8_t  bright = (uint8_t)((1.0f - s.z / 240.0f) * 255);
    uint16_t col    = (uint16_t)canvas.color888(bright, bright, bright);
    if (sx >= 0 && sx < 240 && sy >= 0 && sy < 240) {
      if (size < 1.5f) canvas.drawPixel((int)sx, (int)sy, col);
      else             canvas.fillCircle((int)sx, (int)sy, (int)size, col);
    }
  }
}

// ===== HUD =====
const char* demoNames[] = {
  "Bouncing Balls", "Sine Waves", "Gears", "Lissajous", "Starfield"
};

void drawHUD() {
  canvas.fillRect(0, 0, 240, 16, (uint16_t)canvas.color888(0, 0, 80));
  canvas.setTextColor(TFT_WHITE);
  canvas.setTextSize(1);
  canvas.setCursor(4, 5);
  canvas.print(demoNames[demoIndex]);
  uint32_t elapsed  = millis() - demoTimer;
  int      progress = (int)(240.0f * elapsed / DEMO_DURATION);
  canvas.drawFastHLine(0, 15, 240,      (uint16_t)canvas.color888( 30,  30,  80));
  canvas.drawFastHLine(0, 15, progress, (uint16_t)canvas.color888(100, 200, 255));
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  Serial.println("ESP32-S3 ST7789 240x240 Demo");

  tft.init();
  tft.setRotation(0);

  // Startup color test
  tft.fillScreen(TFT_RED);   delay(400);
  tft.fillScreen(TFT_GREEN); delay(400);
  tft.fillScreen(TFT_BLUE);  delay(400);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(30, 100);
  tft.print("ST7789 OK");
  tft.setTextSize(1);
  tft.setCursor(50, 130);
  tft.print("Starting...");
  delay(1000);

  // Allocate sprite (double buffer)
  canvas.setColorDepth(16);
  canvas.createSprite(240, 240);
  canvas.setFont(&fonts::Font2);

  initBalls();
  initLissajous();
  initStars();

  demoTimer = millis();
  randomSeed(esp_random());
}

// ===== Main loop =====
void loop() {
  if (millis() - demoTimer >= DEMO_DURATION) {
    demoIndex = (demoIndex + 1) % DEMO_COUNT;
    demoTimer = millis();
    if (demoIndex == 0) initBalls();
    if (demoIndex == 3) initLissajous();
    if (demoIndex == 4) initStars();
  }

  switch (demoIndex) {
    case 0: demoBalls();     break;
    case 1: demoWave();      break;
    case 2: demoGears();     break;
    case 3: demoLissajous(); break;
    case 4: demoStarfield(); break;
  }

  drawHUD();
  canvas.pushSprite(0, 0);
}
