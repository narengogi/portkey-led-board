# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32 Arduino project that drives a P10 RGB HUB75 LED panel display (128x32 pixels from 8 panels in 4x2 serpentine configuration). Displays a scrolling "Portkey Metrics Dashboard" with trending model data and real-time metrics fetched from a JSON API.

## Build & Upload (Arduino IDE)

1. **Board Setup**: Tools → Board → "ESP32 Dev Module"
2. **Required Libraries** (install via Library Manager):
   - Adafruit GFX Library
   - ESP32 HUB75 LED MATRIX PANEL DMA Display (by mrfaptastic)
   - ArduinoJson (by Benoit Blanchon)
3. **Upload Settings**: 921600 baud, 80MHz flash, QIO mode, Default 4MB partition
4. **Upload Issues**: Hold BOOT button during upload if it fails

## Code Architecture

Single-file Arduino sketch (`esp32_p10_rgb.ino`) with these main components:

- **Configuration Section** (lines 17-75): WiFi credentials, data URL, panel dimensions, GPIO pin mapping, color definitions
- **setup()**: Initializes HUB75 display with DMA, shows startup message, connects WiFi, fetches initial data
- **loop()**: Handles hourly data refresh, WiFi reconnection, scroll animation (35ms intervals)
- **fetchData()**: HTTP GET to JSON API with loading state display
- **parseJSON()**: Parses trending array and metrics object into global state
- **renderDisplay()**: Main render loop calling `renderTrendingRow()` and `renderMetricsRow()`
- **shortenModelName()**: Maps verbose model names to display-friendly versions (e.g., "claude-sonnet-4-5-20250929" → "Claude 4.5")

**Key Data Structures**:
- `TrendingItem` struct: name, current value, change percentage, positive/negative flag
- Separate scroll positions for top row (trending) and bottom row (metrics)

## Hardware Configuration

**Panel Layout**: 4x2 serpentine chain (Panel 1 at top-right, snake pattern down)

**GPIO Pin Mapping** (important - GPIO12 avoided due to boot issues):
- RGB upper: R1=25, G1=26, B1=27
- RGB lower: R2=14, G2=32, B2=13
- Row select: A=23, B=19, C=5, D=17
- Control: CLK=16, LAT=4, OE=15

## Key Configuration Values

```cpp
#define FETCH_INTERVAL_MS 3600000  // Data refresh interval (1 hour)
scrollDelay = 35                   // Scroll speed in ms
dma_display->setBrightness8(80)    // Brightness 0-255
```

## Debugging

Open Serial Monitor at 115200 baud for:
- Panel configuration info
- WiFi connection status
- HTTP fetch results and JSON parsing status
