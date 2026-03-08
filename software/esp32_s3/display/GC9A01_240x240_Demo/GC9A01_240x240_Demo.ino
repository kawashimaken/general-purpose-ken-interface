/**
 * ESP32-S3 + Round TFT SPI Display Demo
 * Driver  : GC9A01   (240x240, circular display)
 * Library : LovyanGFX (latest)
 *
 * Pin Assignments
 *  MOSI  GPIO11
 *  SCK   GPIO12
 *  MISO  GPIO13  (connected but unused)
 *  CS    GPIO10
 *  DC    GPIO17
 *  RESET GPIO18
 *  BL    tie to 3.3V
 */

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// ===== LovyanGFX configuration =====
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_GC9A01 _panel_instance;
  lgfx::Bus_SPI      _bus_instance;

public:
  LGFX() {
    // --- SPI bus ---
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host    = SPI2_HOST;
      cfg.spi_mode    = 0;
      cfg.freq_write  = 40000000;
      cfg.freq_read   = 16000000;
      cfg.spi_3wire   = false;
      cfg.use_lock    = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk    = 12;
      cfg.pin_mosi    = 11;
      cfg.pin_miso    = 13;
      cfg.pin_dc      = 17;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    // --- Panel ---
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs           = 10;
      cfg.pin_rst          = 18;
      cfg.pin_busy         = -1;
      cfg.panel_width      = 240;
      cfg.panel_height     = 240;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.offset_rotation  = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable         = false;
      cfg.invert           = true;   // GC9A01 needs invert
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = false;
      _panel_instance.config(cfg);
    }

    setPanel(&_panel_instance);
  }
};

static LGFX        tft;
static LGFX_Sprite canvas(&tft);

// circle constants
static const int CX = 120;   // center X
static const int CY = 120;   // center Y
static const int CR = 120;   // circle radius (full screen)

// ===== Demo management =====
static const int      DEMO_COUNT    = 6;
static int            demoIndex     = 0;
static uint32_t       demoTimer     = 0;
static const uint32_t DEMO_DURATION = 6000;

static inline float frand(float lo, float hi) {
  return lo + (float)random(0, 10000) / 10000.0f * (hi - lo);
}

// check if point is inside the circular screen
static inline bool inCircle(int x, int y) {
  int dx = x - CX, dy = y - CY;
  return dx*dx + dy*dy <= CR*CR;
}

// ===== Demo 0 : Radar Sweep =====
static float radarAngle = 0;
static const int TRAIL_STEPS = 60;

void demoRadar() {
  canvas.fillScreen(TFT_BLACK);

  // grid circles
  for (int r = 30; r <= 120; r += 30)
    canvas.drawCircle(CX, CY, r, (uint16_t)canvas.color888(0, 60, 0));

  // crosshair
  canvas.drawFastHLine(0,  CY, 240, (uint16_t)canvas.color888(0, 60, 0));
  canvas.drawFastVLine(CX,  0, 240, (uint16_t)canvas.color888(0, 60, 0));

  // fading sweep trail
  for (int i = 0; i < TRAIL_STEPS; i++) {
    float a      = radarAngle - i * (M_PI / 180.0f) * 2.0f;
    float bright = (float)(TRAIL_STEPS - i) / TRAIL_STEPS;
    uint16_t col = (uint16_t)canvas.color888(0, (uint8_t)(bright * 220), 0);
    int x2 = CX + (int)(CR * cosf(a));
    int y2 = CY + (int)(CR * sinf(a));
    canvas.drawLine(CX, CY, x2, y2, col);
  }

  // random blips
  static int blipX[6] = {80,150,60,170,100,140};
  static int blipY[6] = {70,100,150,160,180,50};
  for (int i = 0; i < 6; i++) {
    float dist  = sqrtf((blipX[i]-CX)*(blipX[i]-CX) + (blipY[i]-CY)*(blipY[i]-CY));
    float blipA = atan2f(blipY[i]-CY, blipX[i]-CX);
    float diff  = fmodf(radarAngle - blipA + 4*M_PI, 2*M_PI);
    if (diff < 0.4f) {
      uint8_t bright = (uint8_t)((1.0f - diff / 0.4f) * 255);
      canvas.fillCircle(blipX[i], blipY[i], 4, (uint16_t)canvas.color888(0, bright, 0));
    }
  }

  radarAngle += 0.04f;
  if (radarAngle > 2 * M_PI) radarAngle -= 2 * M_PI;
}

// ===== Demo 1 : Analog Clock =====
void demoAnalogClock() {
  static uint32_t clockMs = 0;
  static float    sec = 0, min = 0, hr = 0;

  canvas.fillScreen(TFT_BLACK);

  // outer bezel
  canvas.fillCircle(CX, CY, CR, (uint16_t)canvas.color888(20, 20, 40));
  canvas.drawCircle(CX, CY, CR-1, (uint16_t)canvas.color888(80, 80, 180));
  canvas.drawCircle(CX, CY, CR-3, (uint16_t)canvas.color888(40, 40, 100));

  // hour tick marks
  for (int i = 0; i < 60; i++) {
    float a  = i * 2.0f * M_PI / 60.0f - M_PI / 2.0f;
    int   r1 = (i % 5 == 0) ? CR - 14 : CR - 8;
    int   w  = (i % 5 == 0) ? 2 : 1;
    uint16_t col = (i % 5 == 0)
      ? (uint16_t)canvas.color888(200, 200, 255)
      : (uint16_t)canvas.color888(80,  80,  120);
    canvas.drawLine(CX + (int)((r1)*cosf(a)), CY + (int)((r1)*sinf(a)),
                    CX + (int)((CR-4)*cosf(a)), CY + (int)((CR-4)*sinf(a)), col);
  }

  // advance time
  uint32_t now = millis();
  float dt = (now - clockMs) / 1000.0f;
  clockMs = now;
  sec += dt;
  if (sec >= 60) { sec -= 60; min += 1; }
  if (min >= 60) { min -= 60; hr  += 1; }
  if (hr  >= 12) hr  -= 12;

  // hour hand
  float ha = (hr + min/60.0f) / 12.0f * 2*M_PI - M_PI/2;
  canvas.drawLine(CX, CY,
                  CX + (int)(65 * cosf(ha)),
                  CY + (int)(65 * sinf(ha)),
                  (uint16_t)canvas.color888(255,255,255));
  canvas.drawLine(CX, CY,
                  CX + (int)(64 * cosf(ha)),
                  CY + (int)(64 * sinf(ha)),
                  (uint16_t)canvas.color888(255,255,255));

  // minute hand
  float ma = min / 60.0f * 2*M_PI - M_PI/2;
  canvas.drawLine(CX, CY,
                  CX + (int)(90 * cosf(ma)),
                  CY + (int)(90 * sinf(ma)),
                  (uint16_t)canvas.color888(180, 220, 255));

  // second hand
  float sa = sec / 60.0f * 2*M_PI - M_PI/2;
  canvas.drawLine(CX, CY,
                  CX + (int)(100 * cosf(sa)),
                  CY + (int)(100 * sinf(sa)),
                  (uint16_t)canvas.color888(255, 80, 80));
  canvas.drawLine(CX, CY,
                  CX - (int)(25 * cosf(sa)),
                  CY - (int)(25 * sinf(sa)),
                  (uint16_t)canvas.color888(255, 80, 80));

  // center cap
  canvas.fillCircle(CX, CY, 5, TFT_WHITE);
}

// ===== Demo 2 : Bouncing Balls (clipped to circle) =====
struct Ball { float x, y, vx, vy, r; uint16_t color; };
static const int BALL_COUNT = 10;
static Ball balls[BALL_COUNT];

void initBalls() {
  for (int i = 0; i < BALL_COUNT; i++) {
    balls[i].r     = frand(8, 18);
    balls[i].x     = CX + frand(-50, 50);
    balls[i].y     = CY + frand(-50, 50);
    balls[i].vx    = frand(1.5f, 3.5f) * (random(2) ? 1 : -1);
    balls[i].vy    = frand(1.5f, 3.5f) * (random(2) ? 1 : -1);
    balls[i].color = (uint16_t)canvas.color888(random(80,255), random(80,255), random(80,255));
  }
}

void demoBalls() {
  canvas.fillScreen(TFT_BLACK);
  canvas.drawCircle(CX, CY, CR-1, (uint16_t)canvas.color888(40, 40, 80));

  for (int i = 0; i < BALL_COUNT; i++) {
    Ball &b = balls[i];
    b.x += b.vx; b.y += b.vy;
    // circular boundary bounce
    float dx = b.x - CX, dy = b.y - CY;
    float dist = sqrtf(dx*dx + dy*dy);
    if (dist + b.r > CR - 2) {
      float nx = dx / dist, ny = dy / dist;
      float dot = b.vx*nx + b.vy*ny;
      b.vx -= 2*dot*nx; b.vy -= 2*dot*ny;
      b.x = CX + nx * (CR - 2 - b.r);
      b.y = CY + ny * (CR - 2 - b.r);
    }
    canvas.fillCircle((int)b.x, (int)b.y, (int)b.r, b.color);
    canvas.fillCircle((int)b.x - (int)(b.r*0.3f),
                      (int)b.y - (int)(b.r*0.3f),
                      max(2,(int)(b.r*0.25f)), TFT_WHITE);
  }
}

// ===== Demo 3 : Spiral =====
static float spiralAngle = 0;
static const int SPIRAL_TRAIL = 400;
static int16_t spiralX[SPIRAL_TRAIL], spiralY[SPIRAL_TRAIL];
static int spiralHead = 0;

void initSpiral() {
  memset(spiralX, 0, sizeof(spiralX));
  memset(spiralY, 0, sizeof(spiralY));
  spiralHead = 0;
  spiralAngle = 0;
}

void demoSpiral() {
  canvas.fillScreen(TFT_BLACK);

  float r = (sinf(spiralAngle * 0.07f) * 0.5f + 0.5f) * (CR - 10);
  spiralX[spiralHead] = CX + (int)(r * cosf(spiralAngle));
  spiralY[spiralHead] = CY + (int)(r * sinf(spiralAngle));
  spiralHead = (spiralHead + 1) % SPIRAL_TRAIL;

  for (int i = 0; i < SPIRAL_TRAIL - 1; i++) {
    int   a     = (spiralHead + i)     % SPIRAL_TRAIL;
    int   b     = (spiralHead + i + 1) % SPIRAL_TRAIL;
    float ratio = (float)i / SPIRAL_TRAIL;
    uint8_t r_  = (uint8_t)(sinf(ratio * M_PI + 0.0f) * 255);
    uint8_t g_  = (uint8_t)(sinf(ratio * M_PI + 2.1f) * 255);
    uint8_t b_  = (uint8_t)(sinf(ratio * M_PI + 4.2f) * 255);
    canvas.drawLine(spiralX[a], spiralY[a], spiralX[b], spiralY[b],
                    (uint16_t)canvas.color888(r_, g_, b_));
  }
  spiralAngle += 0.07f;
}

// ===== Demo 4 : Rotating Concentric Rings =====
static float ringAngle = 0;

void demoRings() {
  canvas.fillScreen(TFT_BLACK);

  for (int ring = 1; ring <= 7; ring++) {
    int     r    = ring * 16;
    float   offs = ringAngle * (ring % 2 == 0 ? 1 : -1) * (0.5f + ring*0.1f);
    uint8_t rv   = (uint8_t)(sinf(offs + 0.0f) * 127 + 128);
    uint8_t gv   = (uint8_t)(sinf(offs + 2.1f) * 127 + 128);
    uint8_t bv   = (uint8_t)(sinf(offs + 4.2f) * 127 + 128);
    uint16_t col = (uint16_t)canvas.color888(rv, gv, bv);

    // dotted ring
    int dots = ring * 8;
    for (int d = 0; d < dots; d++) {
      float a  = offs + d * 2.0f * M_PI / dots;
      int   px = CX + (int)(r * cosf(a));
      int   py = CY + (int)(r * sinf(a));
      canvas.fillCircle(px, py, 2, col);
    }
  }
  ringAngle += 0.03f;
}

// ===== Demo 5 : Starfield =====
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
    float    sx     = s.x / s.z * 240 + CX;
    float    sy     = s.y / s.z * 240 + CY;
    float    size   = (1.0f - s.z / 240.0f) * 4.0f;
    uint8_t  bright = (uint8_t)((1.0f - s.z / 240.0f) * 255);
    uint16_t col    = (uint16_t)canvas.color888(bright, bright, bright);
    if (inCircle((int)sx, (int)sy)) {
      if (size < 1.5f) canvas.drawPixel((int)sx, (int)sy, col);
      else             canvas.fillCircle((int)sx, (int)sy, (int)size, col);
    }
  }
}

// ===== HUD (arc style) =====
const char* demoNames[] = {
  "Radar", "Clock", "Balls", "Spiral", "Rings", "Starfield"
};

void drawHUD() {
  // small label at top center
  canvas.fillRect(75, 0, 90, 14, (uint16_t)canvas.color888(0, 0, 60));
  canvas.setTextColor(TFT_WHITE);
  canvas.setTextSize(1);
  int tx = 120 - strlen(demoNames[demoIndex]) * 3;
  canvas.setCursor(tx, 3);
  canvas.print(demoNames[demoIndex]);

  // arc progress bar along bottom of circle
  uint32_t elapsed  = millis() - demoTimer;
  float    ratio    = (float)elapsed / DEMO_DURATION;
  int      arcSpan  = (int)(180.0f * ratio); // sweep 180 degrees along bottom
  for (int deg = 0; deg <= arcSpan; deg++) {
    float a  = (deg - 90) * M_PI / 180.0f + M_PI;  // bottom arc
    int   px = CX + (int)((CR-3) * cosf(a));
    int   py = CY + (int)((CR-3) * sinf(a));
    canvas.drawPixel(px, py, (uint16_t)canvas.color888(100, 200, 255));
  }
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  Serial.println("ESP32-S3 GC9A01 240x240 Round Display Demo");

  tft.init();
  tft.setRotation(0);

  // Startup color test
  tft.fillScreen(TFT_RED);   delay(400);
  tft.fillScreen(TFT_GREEN); delay(400);
  tft.fillScreen(TFT_BLUE);  delay(400);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(40, 100);
  tft.print("GC9A01 OK");
  tft.setTextSize(1);
  tft.setCursor(55, 130);
  tft.print("Round 240x240");
  delay(1000);

  // Allocate sprite (double buffer)
  canvas.setColorDepth(16);
  canvas.createSprite(240, 240);
  canvas.setFont(&fonts::Font0);

  initBalls();
  initSpiral();
  initStars();

  demoTimer = millis();
  randomSeed(esp_random());
}

// ===== Main loop =====
void loop() {
  if (millis() - demoTimer >= DEMO_DURATION) {
    demoIndex = (demoIndex + 1) % DEMO_COUNT;
    demoTimer = millis();
    if (demoIndex == 2) initBalls();
    if (demoIndex == 3) initSpiral();
    if (demoIndex == 5) initStars();
  }

  switch (demoIndex) {
    case 0: demoRadar();      break;
    case 1: demoAnalogClock();break;
    case 2: demoBalls();      break;
    case 3: demoSpiral();     break;
    case 4: demoRings();      break;
    case 5: demoStarfield();  break;
  }

  drawHUD();
  canvas.pushSprite(0, 0);
}
