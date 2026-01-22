/*
 * ESP32 + P10 RGB Panel - Portkey Metrics Dashboard
 *
 * 2 rows of 4 panels each (128x32 total, serpentine chain)
 * Top row: Trending models with request counts and change %
 * Bottom row: Real-time metrics (reqs, tokens, spend, rates)
 *
 * Panel: XPS RUME HYP10-2525/2727-32x16-8S (1/8 scan)
 * Board: ESP-WROOM-32D NodeMCU
 */

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ============== WIFI CONFIGURATION ==============
const char* WIFI_SSID = "SSID";
const char* WIFI_PASSWORD = "PASSWORD";

// ============== DATA SOURCE ==============
const char* DATA_URL = "https://portkey.ai/rankings/metrics.json";
#define FETCH_INTERVAL_MS 3600000  // Refresh every 1 hour

// ============== PANEL CONFIGURATION ==============
// Single panel dimensions
#define PANEL_RES_X 32
#define PANEL_RES_Y 16

// Panel arrangement: 4 columns x 2 rows = 8 panels
#define PANEL_COLS 4
#define PANEL_ROWS 2
#define PANEL_CHAIN (PANEL_COLS * PANEL_ROWS)  // 8 panels total

// Total display dimensions
#define TOTAL_WIDTH  (PANEL_RES_X * PANEL_COLS)   // 128 pixels
#define TOTAL_HEIGHT (PANEL_RES_Y * PANEL_ROWS)   // 32 pixels

// ============== PIN MAPPING ==============
#define R1_PIN  25
#define G1_PIN  26
#define B1_PIN  27
#define R2_PIN  14
#define G2_PIN  32  // Avoid GPIO12 (boot pin)
#define B2_PIN  13

#define A_PIN   23
#define B_PIN   19
#define C_PIN   5
#define D_PIN   17
#define E_PIN   -1  // Not used for 1/8 scan

#define CLK_PIN 16
#define LAT_PIN 4
#define OE_PIN  15

// ============== COLORS ==============
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_CYAN      0x07FF
#define COLOR_MAGENTA   0xF81F
#define COLOR_ORANGE    0xFD20
#define COLOR_PINK      0xFC18
#define COLOR_PURPLE    0x8010
#define COLOR_LIME      0x87E0

// ============== GLYPHS ==============
const char* ARROW_UP = "^";
const char* ARROW_DOWN = "v";
const char* BULLET = "*";
const char* SEPARATOR = " | ";

// ============== GLOBALS ==============
MatrixPanel_I2S_DMA *dma_display = nullptr;

// Text buffers for each row
String trendingText = "  Loading trending models...  ";
String metricsText = "  Loading metrics...  ";

// Scroll positions for each row
int scrollPosTrending = 0;
int scrollPosMetrics = 0;

unsigned long lastFetchTime = 0;
unsigned long lastScrollTime = 0;
const int scrollDelay = 50;  // ms between scroll steps

// Color arrays for trending items (cycle through these)
const uint16_t modelColors[] = {
  COLOR_CYAN, COLOR_YELLOW, COLOR_MAGENTA, COLOR_LIME,
  COLOR_ORANGE, COLOR_PINK, COLOR_WHITE, COLOR_PURPLE
};
const int numModelColors = sizeof(modelColors) / sizeof(modelColors[0]);

// Store individual trending items for colored rendering
struct TrendingItem {
  String name;
  String current;
  String change;
  bool isPositive;
};

TrendingItem trendingItems[20];
int numTrendingItems = 0;

// Store metrics
String metrics24hReqs = "";
String metrics24hTokens = "";
String metrics24hSpend = "";
String metricsReqPerSec = "";
String metricsTokenPerSec = "";

// ============== SETUP ==============
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n\n==========================================");
  Serial.println("  Portkey Metrics Dashboard");
  Serial.println("  ESP32 + P10 RGB (4x2 panels)");
  Serial.printf("  Display: %dx%d pixels\n", TOTAL_WIDTH, TOTAL_HEIGHT);
  Serial.println("==========================================\n");

  // Configure HUB75 pins
  HUB75_I2S_CFG::i2s_pins _pins = {
    R1_PIN, G1_PIN, B1_PIN,
    R2_PIN, G2_PIN, B2_PIN,
    A_PIN, B_PIN, C_PIN, D_PIN, E_PIN,
    LAT_PIN, OE_PIN, CLK_PIN
  };

  // Create configuration for 4x2 serpentine layout
  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,   // Width per panel
    PANEL_RES_Y,   // Height per panel
    PANEL_CHAIN    // Total panels in chain
  );

  mxconfig.gpio = _pins;
  mxconfig.driver = HUB75_I2S_CFG::FM6126A;
  mxconfig.double_buff = true;
  mxconfig.clkphase = false;
  mxconfig.min_refresh_rate = 100;
  mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_10M;
  mxconfig.latch_blanking = 4;

  Serial.println("Initializing display...");
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);

  if (!dma_display->begin()) {
    Serial.println("*** Display init failed! ***");
    while (1) delay(1000);
  }

  Serial.println("Display initialized!\n");
  dma_display->setBrightness8(100);
  dma_display->fillScreenRGB888(0, 0, 0);
  dma_display->flipDMABuffer();

  // Show startup message
  showStartupMessage();

  // Connect to WiFi
  connectWiFi();

  // Initial data fetch
  fetchData();
}

// ============== STARTUP MESSAGE ==============
void showStartupMessage() {
  dma_display->fillScreenRGB888(0, 0, 0);
  dma_display->setTextSize(1);
  dma_display->setTextWrap(false);

  // Top row
  dma_display->setTextColor(COLOR_CYAN);
  dma_display->setCursor(20, 4);
  dma_display->print("PORTKEY.AI");

  // Bottom row
  dma_display->setTextColor(COLOR_YELLOW);
  dma_display->setCursor(15, 20);
  dma_display->print("METRICS DASH");

  dma_display->flipDMABuffer();
  delay(2000);
}

// ============== WIFI CONNECTION ==============
void connectWiFi() {
  Serial.printf("Connecting to WiFi: %s", WIFI_SSID);

  dma_display->fillScreenRGB888(0, 0, 0);
  dma_display->setTextSize(1);
  dma_display->setTextWrap(false);

  dma_display->setTextColor(COLOR_YELLOW);
  dma_display->setCursor(2, 4);
  dma_display->print("WiFi...");

  dma_display->setTextColor(COLOR_ORANGE);
  dma_display->setCursor(2, 20);
  dma_display->print("Connecting");

  dma_display->flipDMABuffer();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    dma_display->fillScreenRGB888(0, 0, 0);
    dma_display->setTextColor(COLOR_GREEN);
    dma_display->setCursor(2, 4);
    dma_display->print("WiFi OK!");
    dma_display->setTextColor(COLOR_CYAN);
    dma_display->setCursor(2, 20);
    dma_display->print(WiFi.localIP().toString());
    dma_display->flipDMABuffer();
    delay(1500);
  } else {
    Serial.println(" Failed!");
    trendingText = "  WiFi Failed! Check credentials.  ";
    metricsText = "  No connection  ";
  }
}

// ============== FETCH AND PARSE JSON ==============
void fetchData() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return;
  }

  Serial.printf("Fetching: %s\n", DATA_URL);

  // Show loading
  dma_display->fillScreenRGB888(0, 0, 0);
  dma_display->setTextColor(COLOR_CYAN);
  dma_display->setCursor(2, 4);
  dma_display->print("Fetching...");
  dma_display->setTextColor(COLOR_MAGENTA);
  dma_display->setCursor(2, 20);
  dma_display->print("Please wait");
  dma_display->flipDMABuffer();

  HTTPClient http;
  http.begin(DATA_URL);
  http.setTimeout(15000);

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Data received!");
    parseJSON(payload);
  } else {
    Serial.printf("HTTP error: %d\n", httpCode);
    trendingText = "  Error fetching data (HTTP " + String(httpCode) + ")  ";
    metricsText = "  Check connection  ";
  }

  http.end();
  lastFetchTime = millis();

  // Reset scroll positions after fetch
  scrollPosTrending = 0;
  scrollPosMetrics = 0;
}

// ============== PARSE JSON DATA ==============
void parseJSON(const String& json) {
  // Allocate JSON document
  DynamicJsonDocument doc(8192);

  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    trendingText = "  JSON parse error  ";
    return;
  }

  // Parse trending array
  JsonArray trending = doc["trending"];
  numTrendingItems = 0;
  trendingText = "    ";

  for (JsonObject item : trending) {
    if (numTrendingItems >= 20) break;

    String name = item["name"].as<String>();
    String current = item["current"].as<String>();
    String change = item["change"].as<String>();

    // Store for colored rendering
    trendingItems[numTrendingItems].name = name;
    trendingItems[numTrendingItems].current = current;
    trendingItems[numTrendingItems].change = change;
    trendingItems[numTrendingItems].isPositive = change.startsWith("+");

    // Build scrolling text (fallback)
    trendingText += BULLET;
    trendingText += " ";
    trendingText += name;
    trendingText += " ";
    trendingText += current;
    trendingText += " ";
    trendingText += change;
    trendingText += "   ";

    numTrendingItems++;
  }

  trendingText += "    ";

  // Parse metrics
  JsonObject metrics = doc["metrics"];
  metrics24hReqs = metrics["24h reqs"].as<String>();
  metrics24hTokens = metrics["24h tokens"].as<String>();
  metrics24hSpend = metrics["24h spend"].as<String>();
  metricsReqPerSec = metrics["req/s"].as<String>();
  metricsTokenPerSec = metrics["token/s"].as<String>();

  // Build metrics scrolling text
  metricsText = "    ";
  metricsText += "REQS: " + metrics24hReqs;
  metricsText += SEPARATOR;
  metricsText += "TOKENS: " + metrics24hTokens;
  metricsText += SEPARATOR;
  metricsText += "SPEND: " + metrics24hSpend;
  metricsText += SEPARATOR;
  metricsText += metricsReqPerSec + " req/s";
  metricsText += SEPARATOR;
  metricsText += metricsTokenPerSec + " tok/s";
  metricsText += "    ";

  Serial.printf("Parsed %d trending items\n", numTrendingItems);
  Serial.printf("Trending text length: %d\n", trendingText.length());
  Serial.printf("Metrics text length: %d\n", metricsText.length());
}

// ============== MAIN LOOP ==============
void loop() {
  unsigned long currentTime = millis();

  // Periodic data refresh (every hour)
  if (currentTime - lastFetchTime >= FETCH_INTERVAL_MS) {
    fetchData();
  }

  // Reconnect WiFi if disconnected
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastReconnectAttempt = 0;
    if (currentTime - lastReconnectAttempt > 30000) {
      Serial.println("WiFi lost, reconnecting...");
      WiFi.reconnect();
      lastReconnectAttempt = currentTime;
    }
  }

  // Scroll animation
  if (currentTime - lastScrollTime >= scrollDelay) {
    lastScrollTime = currentTime;
    renderDisplay();
  }
}

// ============== RENDER DISPLAY ==============
void renderDisplay() {
  dma_display->fillScreenRGB888(0, 0, 0);

  // ===== TOP ROW: Trending Models (y = 0-15) =====
  renderTrendingRow();

  dma_display->flipDMABuffer();

  // Update scroll positions
  int trendingWidth = trendingText.length() * 6;
  int metricsWidth = metricsText.length() * 6;

  scrollPosTrending++;
  if (scrollPosTrending > trendingWidth + TOTAL_WIDTH + 50) {
    scrollPosTrending = 0;
  }

  scrollPosMetrics++;
  if (scrollPosMetrics > metricsWidth + TOTAL_WIDTH + 50) {
    scrollPosMetrics = 0;
  }
}

// ============== RENDER TRENDING ROW (with colors) ==============
void renderTrendingRow() {
  int x = TOTAL_WIDTH - scrollPosTrending;
  int y = 4;  // Centered in top 16 pixels

  dma_display->setTextSize(1);
  dma_display->setTextWrap(false);

  // Render each trending item with its own color
  int currentX = x;

  currentX += 24;  // Initial padding (4 spaces * 6 pixels)

  for (int i = 0; i < numTrendingItems; i++) {
    // Bullet - white
    dma_display->setTextColor(COLOR_WHITE);
    dma_display->setCursor(currentX, y);
    dma_display->print(BULLET);
    currentX += 6;

    // Space
    currentX += 6;

    // Model name - colored
    uint16_t nameColor = modelColors[i % numModelColors];
    dma_display->setTextColor(nameColor);
    dma_display->setCursor(currentX, y);
    dma_display->print(trendingItems[i].name);
    currentX += trendingItems[i].name.length() * 6;

    // Space
    currentX += 6;

    // Current value - white
    dma_display->setTextColor(COLOR_WHITE);
    dma_display->setCursor(currentX, y);
    dma_display->print(trendingItems[i].current);
    currentX += trendingItems[i].current.length() * 6;

    // Space
    currentX += 6;

    // Arrow and change - green for positive, red for negative
    uint16_t changeColor = trendingItems[i].isPositive ? COLOR_GREEN : COLOR_RED;
    dma_display->setTextColor(changeColor);
    dma_display->setCursor(currentX, y);

    const char* arrow = trendingItems[i].isPositive ? ARROW_UP : ARROW_DOWN;
    dma_display->print(arrow);
    currentX += 6;

    dma_display->setCursor(currentX, y);
    dma_display->print(trendingItems[i].change);
    currentX += trendingItems[i].change.length() * 6;

    // Spacing between items
    currentX += 18;  // 3 spaces
  }

  dma_display->setTextSize(1);
  dma_display->setTextWrap(false);

  currentX += 24;  // Initial padding

  // REQS label
  dma_display->setTextColor(COLOR_ORANGE);
  dma_display->setCursor(currentX, y);
  dma_display->print("REQS:");
  currentX += 6 * 5;

  // REQS value
  currentX += 6;
  dma_display->setTextColor(COLOR_WHITE);
  dma_display->setCursor(currentX, y);
  dma_display->print(metrics24hReqs);
  currentX += metrics24hReqs.length() * 6;

  // Separator
  dma_display->setTextColor(COLOR_PURPLE);
  dma_display->setCursor(currentX, y);
  dma_display->print(SEPARATOR);
  currentX += strlen(SEPARATOR) * 6;

  // TOKENS label
  dma_display->setTextColor(COLOR_CYAN);
  dma_display->setCursor(currentX, y);
  dma_display->print("TOKENS:");
  currentX += 7 * 6;

  // TOKENS value
  currentX += 6;
  dma_display->setTextColor(COLOR_WHITE);
  dma_display->setCursor(currentX, y);
  dma_display->print(metrics24hTokens);
  currentX += metrics24hTokens.length() * 6;

  // Separator
  dma_display->setTextColor(COLOR_PURPLE);
  dma_display->setCursor(currentX, y);
  dma_display->print(SEPARATOR);
  currentX += strlen(SEPARATOR) * 6;

  // SPEND label
  dma_display->setTextColor(COLOR_GREEN);
  dma_display->setCursor(currentX, y);
  dma_display->print("SPEND:");
  currentX += 6 * 6;

  // SPEND value
  currentX += 6;
  dma_display->setTextColor(COLOR_YELLOW);
  dma_display->setCursor(currentX, y);
  dma_display->print(metrics24hSpend);
  currentX += metrics24hSpend.length() * 6;

  // Separator
  dma_display->setTextColor(COLOR_PURPLE);
  dma_display->setCursor(currentX, y);
  dma_display->print(SEPARATOR);
  currentX += strlen(SEPARATOR) * 6;

  // REQ/S
  dma_display->setTextColor(COLOR_MAGENTA);
  dma_display->setCursor(currentX, y);
  dma_display->print(metricsReqPerSec);
  currentX += metricsReqPerSec.length() * 6;

  dma_display->setTextColor(COLOR_WHITE);
  dma_display->setCursor(currentX, y);
  dma_display->print(" req/s");
  currentX += 6 * 6;

  // Separator
  dma_display->setTextColor(COLOR_PURPLE);
  dma_display->setCursor(currentX, y);
  dma_display->print(SEPARATOR);
  currentX += strlen(SEPARATOR) * 6;

  // TOK/S
  dma_display->setTextColor(COLOR_LIME);
  dma_display->setCursor(currentX, y);
  dma_display->print(metricsTokenPerSec);
  currentX += metricsTokenPerSec.length() * 6;

  dma_display->setTextColor(COLOR_WHITE);
  dma_display->setCursor(currentX, y);
  dma_display->print(" tok/s");
}

/*
 * ============== WIRING FOR 4x2 SERPENTINE ==============
 *
 *   Row 1 (top):    [P4] <-- [P3] <-- [P2] <-- [P1] <-- ESP32
 *                     |
 *                     v (ribbon cable down)
 *   Row 2 (bottom): [P5] --> [P6] --> [P7] --> [P8]
 *
 * Connect ESP32 to Panel 1 INPUT
 * Chain: P1 -> P2 -> P3 -> P4 -> P5 -> P6 -> P7 -> P8
 *
 * ============== REQUIRED LIBRARIES ==============
 *
 * 1. ESP32 HUB75 LED MATRIX PANEL DMA Display (by mrfaptastic)
 * 2. Adafruit GFX Library
 * 3. ArduinoJson (by Benoit Blanchon) - Install via Library Manager!
 *
 * ============== CONFIGURATION ==============
 *
 * WiFi: Update WIFI_SSID and WIFI_PASSWORD
 * Refresh: FETCH_INTERVAL_MS (default 1 hour = 3600000ms)
 * Brightness: dma_display->setBrightness8(80) in setup()
 * Scroll speed: scrollDelay (lower = faster)
 *
 */
