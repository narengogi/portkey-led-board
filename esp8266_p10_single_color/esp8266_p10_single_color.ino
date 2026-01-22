/*
 * ESP8266 P10 LED Marquee Controller
 * 
 * Fetches text from an HTTPS URL and displays it as scrolling marquee
 * on HUB12 P10 LED panels.
 * 
 * Hardware: Uno+WiFi R3 (ESP8266 portion)
 * Display: P10 HUB12 single-color LED panels
 * 
 * DIP Switch Setting for programming ESP8266:
 * 1=OFF, 2=OFF, 3=OFF, 4=OFF, 5=ON, 6=ON, 7=ON
 * 
 * Library: DMDESP by busel7
 * Install: https://github.com/busel7/DMDESP (download ZIP and add to Arduino)
 */

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <DMDESP.h>
#include <fonts/EMSans8x16.h>

// ============================================================================
// CONFIGURATION - Modify these values for your setup
// ============================================================================

// WiFi credentials
const char* WIFI_SSID = "SSID";
const char* WIFI_PASSWORD = "PASSWORD";

// Text source URL (HTTPS)
const char* TEXT_URL = "https://portkey.ai/robots.txt";

// Display configuration - CHANGE THESE TO MATCH YOUR PANEL SETUP
#define DISPLAYS_WIDE 5   // Number of P10 panels horizontally
#define DISPLAYS_HIGH 1   // Number of P10 panels vertically

// Timing configuration
#define FETCH_INTERVAL_MS 600000   // Fetch new text every 10 minutes
#define SCROLL_SPEED_MS 30         // Scroll delay in ms (lower = faster)
#define WIFI_TIMEOUT_MS 20000      // WiFi connection timeout
#define WIFI_RETRY_DELAY_MS 5000   // Delay between WiFi reconnection attempts

// Display dimensions (P10 panels are 32x16 pixels each)
#define PANEL_WIDTH 32
#define PANEL_HEIGHT 16
#define DISPLAY_WIDTH (PANEL_WIDTH * DISPLAYS_WIDE)
#define DISPLAY_HEIGHT (PANEL_HEIGHT * DISPLAYS_HIGH)

// ============================================================================
// PIN DEFINITIONS - ESP8266 GPIO to P10 HUB12
// ============================================================================
// These are the default pins for DMD2 on ESP8266:
// Pin A    -> GPIO16 (D0)
// Pin B    -> GPIO12 (D6)
// Pin CLK  -> GPIO14 (D5) - Hardware SPI CLK
// Pin LAT  -> GPIO0  (D3)
// Pin DATA -> GPIO13 (D7) - Hardware SPI MOSI
// Pin OE   -> GPIO15 (D8)

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

// DMDESP display object for ESP8266 + P10 HUB12
// Pin assignments (directly in library, can be modified in DMDESP.h if needed):
// P_A    = D0 (GPIO16)
// P_B    = D6 (GPIO12)
// P_CLK  = D5 (GPIO14)
// P_LAT  = D3 (GPIO0)
// P_OE   = D8 (GPIO15)
// P_DATA = D7 (GPIO13)
DMDESP dmd(DISPLAYS_WIDE, DISPLAYS_HIGH);

// Text buffer for fetched content
String displayText = "Connecting to WiFi...";
String newText = "";

// Scrolling state
int scrollPosition = 0;
int textPixelWidth = 0;

// Timing variables
unsigned long lastFetchTime = 0;
unsigned long lastScrollTime = 0;

// State flags
bool wifiConnected = false;
bool fetchInProgress = false;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

void connectToWiFi();
void fetchTextFromURL();
void updateScrollingText();
void calculateTextWidth();
String sanitizeText(String input);

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  delay(100);
  
  Serial.println();
  Serial.println("================================");
  Serial.println("ESP8266 P10 Marquee Controller");
  Serial.println("================================");
  Serial.printf("Display: %d x %d panels (%d x %d pixels)\n", 
                DISPLAYS_WIDE, DISPLAYS_HIGH, DISPLAY_WIDTH, DISPLAY_HEIGHT);
  
  // Initialize DMD display
  Serial.println("Initializing P10 display...");
  dmd.start();               // Initialize the display
  dmd.setBrightness(50);     // Brightness 0-100 (DMDESP uses 0-100 scale)
  dmd.setFont(EMSans8x16);  // Use built-in font from DMDESP
  
  // Show initial message
  dmd.drawText(0, 0, "INIT...");
  dmd.loop();  // Update display
  
  // Connect to WiFi
  connectToWiFi();
  
  // Calculate initial text width
  calculateTextWidth();
  
  // Reset scroll position to start from right edge
  scrollPosition = DISPLAY_WIDTH;
  
  // Fetch text immediately on startup
  if (wifiConnected) {
    fetchTextFromURL();
  }
  
  Serial.println("Setup complete!");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  unsigned long currentTime = millis();
  
  // Handle WiFi reconnection if disconnected
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConnected) {
      Serial.println("WiFi disconnected!");
      wifiConnected = false;
      displayText = "WiFi Lost - Reconnecting...";
      calculateTextWidth();
      scrollPosition = DISPLAY_WIDTH;
    }
    connectToWiFi();
  }
  
  // Periodic text fetch (non-blocking check)
  if (wifiConnected && (currentTime - lastFetchTime >= FETCH_INTERVAL_MS)) {
    fetchTextFromURL();
  }
  
  // Update scrolling animation
  if (currentTime - lastScrollTime >= SCROLL_SPEED_MS) {
    lastScrollTime = currentTime;
    updateScrollingText();
  }
  
  // Small yield for ESP8266 background tasks
  yield();
}

// ============================================================================
// WIFI CONNECTION
// ============================================================================

void connectToWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    return;
  }
  
  Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
  
  // Show connecting message on display
  dmd.clear();
  dmd.drawText(0, 0, "WiFi...");
  dmd.loop();
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startTime = millis();
  int dotCount = 0;
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    dotCount++;
    
    // Update display with progress dots
    if (dotCount % 3 == 0) {
      dmd.clear();
      String dots = "WiFi";
      for (int i = 0; i < (dotCount / 3) % 4; i++) {
        dots += ".";
      }
      dmd.drawText(0, 0, dots);
      dmd.loop();
    }
    
    // Timeout check
    if (millis() - startTime > WIFI_TIMEOUT_MS) {
      Serial.println("\nWiFi connection timeout!");
      displayText = "No WiFi - Check Settings";
      calculateTextWidth();
      scrollPosition = DISPLAY_WIDTH;
      return;
    }
    
    yield();
  }
  
  wifiConnected = true;
  Serial.println();
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());
  
  displayText = "WiFi Connected!";
  calculateTextWidth();
  scrollPosition = DISPLAY_WIDTH;
}

// ============================================================================
// HTTPS TEXT FETCH
// ============================================================================

void fetchTextFromURL() {
  if (!wifiConnected || fetchInProgress) {
    return;
  }
  
  fetchInProgress = true;
  lastFetchTime = millis();
  
  Serial.printf("Fetching text from: %s\n", TEXT_URL);
  
  // Create secure WiFi client
  WiFiClientSecure client;
  
  // Skip certificate verification (for simplicity)
  // In production, you should use proper certificate validation
  client.setInsecure();
  
  HTTPClient http;
  
  if (http.begin(client, TEXT_URL)) {
    // Set timeout
    http.setTimeout(10000);
    
    // Send GET request
    int httpCode = http.GET();
    
    if (httpCode > 0) {
      Serial.printf("HTTP response code: %d\n", httpCode);
      
      if (httpCode == HTTP_CODE_OK) {
        newText = http.getString();
        
        // Sanitize and prepare text for display
        newText = sanitizeText(newText);
        
        if (newText.length() > 0) {
          displayText = newText;
          Serial.printf("Fetched %d characters\n", displayText.length());
          Serial.println("Text preview: " + displayText.substring(0, min(50, (int)displayText.length())) + "...");
        } else {
          displayText = "Empty response from server";
          Serial.println("Warning: Empty response");
        }
      } else {
        displayText = "HTTP Error: " + String(httpCode);
        Serial.printf("HTTP error: %d\n", httpCode);
      }
    } else {
      displayText = "Connection Failed";
      Serial.printf("HTTP request failed: %s\n", http.errorToString(httpCode).c_str());
    }
    
    http.end();
  } else {
    displayText = "Failed to connect";
    Serial.println("Unable to begin HTTP connection");
  }
  
  // Recalculate text width and reset scroll
  calculateTextWidth();
  scrollPosition = DISPLAY_WIDTH;
  
  fetchInProgress = false;
}

// ============================================================================
// TEXT SANITIZATION
// ============================================================================

String sanitizeText(String input) {
  String output = "";
  
  // Replace newlines and carriage returns with spaces
  input.replace("\r\n", " ");
  input.replace("\n", " ");
  input.replace("\r", " ");
  input.replace("\t", " ");
  
  // Remove multiple consecutive spaces
  bool lastWasSpace = false;
  for (unsigned int i = 0; i < input.length(); i++) {
    char c = input.charAt(i);
    
    // Only keep printable ASCII characters
    if (c >= 32 && c <= 126) {
      if (c == ' ') {
        if (!lastWasSpace) {
          output += c;
          lastWasSpace = true;
        }
      } else {
        output += c;
        lastWasSpace = false;
      }
    }
  }
  
  // Trim leading/trailing spaces
  output.trim();
  
  // Add spacing at the end for smooth looping
  output += "     ";
  
  return output;
}

// ============================================================================
// SCROLLING DISPLAY
// ============================================================================

void calculateTextWidth() {
  // Calculate pixel width of the text using current font
  // DMDESP provides textWidth method
  textPixelWidth = dmd.textWidth(displayText);
  Serial.printf("Text width: %d pixels\n", textPixelWidth);
}

void updateScrollingText() {
  // Clear the display
  dmd.clear();
  
  // Draw text at current scroll position
  // Y position: 0 for top alignment (font handles vertical positioning)
  dmd.drawText(scrollPosition, 0, displayText);
  
  // Update the physical display
  dmd.loop();
  
  // Move scroll position left
  scrollPosition--;
  
  // Reset scroll when text has fully scrolled off screen
  if (scrollPosition < -textPixelWidth) {
    scrollPosition = DISPLAY_WIDTH;
  }
}
