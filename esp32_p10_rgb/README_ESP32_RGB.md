# ESP32 RGB P10 LED Marquee Controller

A WiFi-enabled scrolling marquee display using ESP32 and RGB P10 HUB75 LED panels.

## Hardware Requirements

- **Board:** ESP-WROOM-32D ESP32 NodeMCU WiFi + BLE
- **Display:** P10 HUB75 RGB LED panels (32x16 pixels each)
- **Power:** 5V power supply rated for your panel count (see power section)

## Default Configuration: 4x2 Panels (128x32 pixels)

The sketch is pre-configured for **8 panels** arranged in 2 rows of 4:

```
┌─────────┬─────────┬─────────┬─────────┐
│ Panel 4 │ Panel 3 │ Panel 2 │ Panel 1 │  ◀── Row 1 (top)
│   32x16 │   32x16 │   32x16 │   32x16 │
└────┬────┴─────────┴─────────┴────┬────┘
     │                             │
     │  ◀── Ribbon cable ──────────┘
     ▼
┌────┴────┬─────────┬─────────┬─────────┐
│ Panel 5 │ Panel 6 │ Panel 7 │ Panel 8 │  ◀── Row 2 (bottom)
│   32x16 │   32x16 │   32x16 │   32x16 │
└─────────┴─────────┴─────────┴────┬────┘
                                   │
                              ESP32 IN

Total Display: 128 x 32 pixels
```

## Wiring Diagram

### ESP32 NodeMCU to HUB75 Pin Connections

Use the **labels printed on your board** (IO25, IO26, etc.) to find the correct pins:

```
┌──────────────────────────────────────────────────────────────────┐
│                      ESP32 NodeMCU DevKit                        │
│                         [USB PORT]                               │
│                                                                  │
│   LEFT SIDE                              RIGHT SIDE              │
│   ─────────                              ──────────              │
│   3V3  [ ]                               [ ] GND ◄── HUB75 GND   │
│   EN   [ ]                               [ ] IO23 ◄── A          │
│   VP   [ ]                               [ ] IO22                │
│   VN   [ ]                               [ ] TX                  │
│   IO34 [ ]                               [ ] RX                  │
│   IO35 [ ]                               [ ] IO21                │
│   IO32 [ ] ◄── G2                        [ ] GND                 │
│   IO33 [ ]                               [ ] IO19 ◄── B          │
│   IO25 [ ] ◄── R1                        [ ] IO18                │
│   IO26 [ ] ◄── G1                        [ ] IO5  ◄── C          │
│   IO27 [ ] ◄── B1                        [ ] IO17 ◄── D          │
│   IO14 [ ] ◄── R2                        [ ] IO16 ◄── CLK        │
│   IO12 [ ] ✗ DO NOT USE                  [ ] IO4  ◄── LAT        │
│   GND  [ ]                               [ ] IO0                 │
│   IO13 [ ] ◄── B2                        [ ] IO2                 │
│   D2   [ ]                               [ ] IO15 ◄── OE         │
│   D3   [ ]                               [ ] D1                  │
│   CMD  [ ]                               [ ] D0                  │
│   5V   [ ]                               [ ] CLK                 │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

### Complete Pin Mapping Table (ESP32-WROOM-32D Official)

**IMPORTANT:** GPIO12 is a boot strapping pin - we use GPIO32 for G2 instead!

| HUB75 Pin | Function        | GPIO   | Module Pin # | Datasheet Name |
|-----------|-----------------|--------|--------------|----------------|
| R1        | Red (upper)     | GPIO25 | Pin 10       | IO25           |
| G1        | Green (upper)   | GPIO26 | Pin 11       | IO26           |
| B1        | Blue (upper)    | GPIO27 | Pin 12       | IO27           |
| R2        | Red (lower)     | GPIO14 | Pin 13       | IO14           |
| G2        | Green (lower)   | **GPIO32** | **Pin 8** | **IO32** |
| B2        | Blue (lower)    | GPIO13 | Pin 16       | IO13           |
| A         | Row select A    | GPIO23 | Pin 37       | IO23           |
| B         | Row select B    | GPIO19 | Pin 31       | IO19           |
| C         | Row select C    | GPIO5  | Pin 29       | IO5            |
| D         | Row select D    | GPIO17 | Pin 28       | IO17           |
| E         | Row select E    | -1     | -            | Not used       |
| CLK       | Pixel clock     | GPIO16 | Pin 27       | IO16           |
| LAT       | Data latch      | GPIO4  | Pin 26       | IO4            |
| OE        | Output enable   | GPIO15 | Pin 23       | IO15           |
| GND       | Ground          | GND    | Pin 1,15,38  | GND            |

### ESP32 NodeMCU DevKit Board Pinout

This is the pinout for the ESP32 NodeMCU development board (not the raw module):

```
                    ESP32 NodeMCU DevKit
                    ┌──────────────────┐
                    │    [USB PORT]    │
                    └────────┬─────────┘
                             │
        3V3  [ ]─────────────┼─────────────[ ] GND
        EN   [ ]─────────────┼─────────────[ ] IO23 ◄── A
        VP   [ ]─────────────┼─────────────[ ] IO22
        VN   [ ]─────────────┼─────────────[ ] TX
        IO34 [ ]─────────────┼─────────────[ ] RX
        IO35 [ ]─────────────┼─────────────[ ] IO21
 G2 ──► IO32 [ ]─────────────┼─────────────[ ] GND
        IO33 [ ]─────────────┼─────────────[ ] IO19 ◄── B
 R1 ──► IO25 [ ]─────────────┼─────────────[ ] IO18
 G1 ──► IO26 [ ]─────────────┼─────────────[ ] IO5  ◄── C
 B1 ──► IO27 [ ]─────────────┼─────────────[ ] IO17 ◄── D
 R2 ──► IO14 [ ]─────────────┼─────────────[ ] IO16 ◄── CLK
        IO12 [ ] DO NOT USE! ┼─────────────[ ] IO4  ◄── LAT
        GND  [ ]─────────────┼─────────────[ ] IO0
 B2 ──► IO13 [ ]─────────────┼─────────────[ ] IO2
        D2   [ ]─────────────┼─────────────[ ] IO15 ◄── OE
        D3   [ ]─────────────┼─────────────[ ] D1
        CMD  [ ]─────────────┼─────────────[ ] D0
        5V   [ ]─────────────┼─────────────[ ] CLK
                             │
                    └────────┴─────────┘
```

**WARNING:** Do NOT use IO12 - it's a boot strapping pin that causes "invalid header" errors!

### HUB75 Connector Pinout (16-pin IDC)

```
Looking at the INPUT connector on the panel:

            ┌───────────────────┐
       R1  1│ ●               ● │2  G1
       B1  3│ ●               ● │4  GND
       R2  5│ ●               ● │6  G2
       B2  7│ ●               ● │8  GND
        A  9│ ●               ● │10 B
        C 11│ ●               ● │12 D
      CLK 13│ ●               ● │14 LAT
       OE 15│ ●               ● │16 GND
            └───────────────────┘
```

## Panel Daisy-Chain Wiring (Serpentine Pattern)

For 4x2 layout, wire panels in a **serpentine/snake pattern**:

```
                    DATA FLOW DIRECTION
                    ══════════════════

    ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐
    │ Panel 4 │◀───│ Panel 3 │◀───│ Panel 2 │◀───│ Panel 1 │◀── ESP32
    │  OUT IN │    │  OUT IN │    │  OUT IN │    │  OUT IN │
    └────┬────┘    └─────────┘    └─────────┘    └─────────┘
         │
         │ Ribbon cable goes DOWN to row 2
         ▼
    ┌────┴────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐
    │ Panel 5 │───▶│ Panel 6 │───▶│ Panel 7 │───▶│ Panel 8 │
    │  IN OUT │    │  IN OUT │    │  IN OUT │    │  IN OUT │
    └─────────┘    └─────────┘    └─────────┘    └─────────┘

    Row 2 direction is REVERSED (serpentine pattern)
```

### Wiring Steps:

1. **ESP32** → **Panel 1 INPUT** (ribbon cable from ESP32)
2. **Panel 1 OUTPUT** → **Panel 2 INPUT**
3. **Panel 2 OUTPUT** → **Panel 3 INPUT**
4. **Panel 3 OUTPUT** → **Panel 4 INPUT**
5. **Panel 4 OUTPUT** → **Panel 5 INPUT** (cable goes down to row 2)
6. **Panel 5 OUTPUT** → **Panel 6 INPUT**
7. **Panel 6 OUTPUT** → **Panel 7 INPUT**
8. **Panel 7 OUTPUT** → **Panel 8 INPUT**

## Power Requirements

**RGB panels draw significant current!**

| Config | Panels | @ 50% Bright | @ 100% Bright | Recommended PSU |
|--------|--------|--------------|---------------|-----------------|
| 4x1    | 4      | 8A           | 16A           | 5V 20A          |
| 4x2    | 8      | 16A          | 32A           | 5V 40A          |
| 5x1    | 5      | 10A          | 20A           | 5V 25A          |
| 5x2    | 10     | 20A          | 40A           | 5V 50A          |

### Power Distribution Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                    5V Power Supply (40A+ for 4x2)                   │
│                        [+5V]            [GND]                       │
└────────────────────────────┬──────────────┬─────────────────────────┘
                             │              │
         ┌───────────────────┼──────────────┼───────────────────┐
         │                   │              │                   │
      ┌──┴──┐    ┌─────┐  ┌──┴──┐    ┌─────┐──┴──┐    ┌─────┐
      │ P1  │    │ P2  │  │ P3  │    │ P4  │     │    │     │
      │ RGB │    │ RGB │  │ RGB │    │ RGB │     │    │     │
      └──┬──┘    └──┬──┘  └──┬──┘    └──┬──┘     │    │     │
         │          │        │          │        │    │     │
      ┌──┴──┐    ┌──┴──┐  ┌──┴──┐    ┌──┴──┐    │    │     │
      │ P5  │    │ P6  │  │ P7  │    │ P8  │    │    │     │
      │ RGB │    │ RGB │  │ RGB │    │ RGB │    │    │     │
      └──┬──┘    └──┬──┘  └──┬──┘    └──┬──┘    │    │     │
         │          │        │          │        │    │     │
         └──────────┴────────┴──────────┴────────┴────┴─────┘
                             │              │
                         All GND        ESP32 GND
                         connected      connected here
```

### Power Wiring Tips:

1. **Use thick wires:** 14-16 AWG for main power rails
2. **Multiple injection points:** Don't rely on panel-to-panel power transfer
3. **Common ground:** ESP32 GND MUST connect to panel GND
4. **Capacitors:** Add 1000µF capacitor near power input to reduce noise

## Software Setup

### 1. Install Arduino IDE

Download from [arduino.cc](https://www.arduino.cc/en/software)

### 2. Add ESP32 Board Support

1. Open Arduino IDE
2. Go to **File → Preferences**
3. In "Additional Board Manager URLs", add:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Go to **Tools → Board → Boards Manager**
5. Search for "esp32" and install **ESP32 by Espressif Systems**

### 3. Install Required Libraries

Go to **Sketch → Include Library → Manage Libraries** and install these (in order):

1. **Adafruit GFX Library** by Adafruit
   (Search: "Adafruit GFX" - install this FIRST, it's a dependency)

2. **Adafruit BusIO** by Adafruit
   (Search: "Adafruit BusIO" - may be auto-installed with GFX)

3. **ESP32 HUB75 LED MATRIX PANEL DMA Display** by mrfaptastic
   (Search: "ESP32 HUB75 Matrix")

### 4. Configure the Board

In Arduino IDE, set:

- **Tools → Board:** "ESP32 Dev Module" or "NodeMCU-32S"
- **Tools → Upload Speed:** 921600
- **Tools → Flash Frequency:** 80MHz
- **Tools → Flash Mode:** QIO
- **Tools → Partition Scheme:** Default 4MB with spiffs
- **Tools → Port:** Select your COM port

### 5. Configure the Sketch

Open `esp32_p10_rgb.ino` and modify:

```cpp
// WiFi credentials
const char* WIFI_SSID = "SSID";
const char* WIFI_PASSWORD = "Password";

// Text source URL
const char* TEXT_URL = "https://portkey.ai/robots.txt";

// Panel arrangement (for 4x2)
#define PANEL_COLS 4          // Panels per row
#define PANEL_ROWS 2          // Number of rows

// Text color (RGB values 0-255)
#define TEXT_COLOR_R 255
#define TEXT_COLOR_G 255
#define TEXT_COLOR_B 0    // Yellow
```

### 6. Upload

1. Connect ESP32 via USB
2. Click **Upload** in Arduino IDE
3. If upload fails, hold the **BOOT** button while uploading

## Configuration Examples

### Single Row (5 panels = 160x16)

```cpp
#define PANEL_COLS 5
#define PANEL_ROWS 1
```

### Two Rows (4x2 = 128x32) - Default

```cpp
#define PANEL_COLS 4
#define PANEL_ROWS 2
```

### Two Rows (5x2 = 160x32)

```cpp
#define PANEL_COLS 5
#define PANEL_ROWS 2
```

### Three Rows (4x3 = 128x48)

```cpp
#define PANEL_COLS 4
#define PANEL_ROWS 3
```

## Color Options

```cpp
// Yellow (default)
#define TEXT_COLOR_R 255
#define TEXT_COLOR_G 255
#define TEXT_COLOR_B 0

// Red
#define TEXT_COLOR_R 255
#define TEXT_COLOR_G 0
#define TEXT_COLOR_B 0

// Green
#define TEXT_COLOR_R 0
#define TEXT_COLOR_G 255
#define TEXT_COLOR_B 0

// Blue
#define TEXT_COLOR_R 0
#define TEXT_COLOR_G 0
#define TEXT_COLOR_B 255

// White
#define TEXT_COLOR_R 255
#define TEXT_COLOR_G 255
#define TEXT_COLOR_B 255

// Cyan
#define TEXT_COLOR_R 0
#define TEXT_COLOR_G 255
#define TEXT_COLOR_B 255

// Magenta
#define TEXT_COLOR_R 255
#define TEXT_COLOR_G 0
#define TEXT_COLOR_B 255

// Orange
#define TEXT_COLOR_R 255
#define TEXT_COLOR_G 165
#define TEXT_COLOR_B 0
```

## Adjusting Settings

### Brightness

In `setup()`, change:
```cpp
dma_display->setBrightness8(128);  // 0-255 (128 = 50%)
```

**Warning:** Higher brightness = more power and heat!

### Scroll Speed

```cpp
#define SCROLL_SPEED_MS 30   // Lower = faster (try 20-50)
```

### Fetch Interval

```cpp
#define FETCH_INTERVAL_MS 600000   // Milliseconds
// 60000 = 1 minute
// 300000 = 5 minutes
// 600000 = 10 minutes
```

## Troubleshooting

### Display shows nothing

1. Check all wiring connections (especially GND!)
2. Verify power supply is providing stable 5V
3. Try with just 1 panel first
4. Check Serial Monitor for error messages

### Colors are wrong/swapped

1. Verify R1/G1/B1 and R2/G2/B2 wiring matches table
2. Some panels have different pinouts - check your panel's datasheet

### Image appears scrambled or shifted

1. Check panel chain order matches serpentine pattern
2. Verify PANEL_COLS and PANEL_ROWS settings match physical layout
3. Try different chain direction in code:
   ```cpp
   CHAIN_TOP_LEFT_DOWN
   CHAIN_TOP_RIGHT_DOWN
   CHAIN_BOTTOM_LEFT_UP
   CHAIN_BOTTOM_RIGHT_UP
   ```

### Upload fails

1. Hold BOOT button while clicking Upload
2. Try lower upload speed (115200)
3. Use a different USB cable

### WiFi won't connect

1. ESP32 only supports 2.4GHz WiFi (not 5GHz)
2. Check SSID/password (case-sensitive)
3. Move closer to router

### Display flickers

1. Add capacitors (100µF + 0.1µF) near power inputs
2. Use shorter wires
3. Ensure power supply can handle the load
4. Reduce brightness

### "Brownout detector triggered"

1. Power supply is insufficient or unstable
2. Add more capacitance
3. Use a higher-rated power supply
4. Reduce brightness

## Serial Monitor

Open Serial Monitor at **115200 baud** to see:
- Panel configuration info
- WiFi connection status
- HTTP fetch results
- Debug messages

## Avoiding GPIO Conflicts (ESP32-WROOM-32D)

**Do NOT use these pins:**

| Pins | Module Pin # | Reason |
|------|--------------|--------|
| GPIO0 | Pin 25 (IO0) | Boot strapping - must be HIGH for normal boot |
| GPIO2 | Pin 24 (IO2) | Boot strapping - must be LOW/floating at boot |
| GPIO6-11 | Pin 17-22 | Connected to internal SPI flash (SD2, SD3, CMD, CLK, SD0, SD1) |
| GPIO34-39 | Pin 4-7 | Input only (SENSOR_VP, SENSOR_VN, IO34, IO35) |

**Safe pins used in this project:**
- IO25, IO26, IO27 (Pin 10-12) - RGB upper half
- IO14, IO12, IO13 (Pin 13-14, 16) - RGB lower half  
- IO23, IO19, IO5, IO17 (Pin 37, 31, 29, 28) - Row address
- IO16, IO4, IO15 (Pin 27, 26, 23) - Control signals

## License

MIT License - Feel free to modify and use for your projects.
