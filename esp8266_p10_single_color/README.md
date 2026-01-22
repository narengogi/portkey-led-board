# ESP8266 P10 LED Marquee Controller

A WiFi-enabled scrolling marquee display that fetches text from an HTTPS URL and displays it on P10 HUB12 LED panels.

## Hardware Requirements

- **Board:** Uno+WiFi R3 ATmega328p+NodeMCU ESP8266 (using ESP8266 only)
- **Display:** P10 HUB12 single-color LED panels (32x16 pixels each)
- **Power:** 5V power supply rated for your panel count (~2A per panel)

## Wiring Diagram

Connect the P10 HUB12 connector to the ESP8266 side of your board:

```
P10 HUB12 Pin    ESP8266 GPIO    NodeMCU Label
─────────────────────────────────────────────
     A              GPIO16           D0
     B              GPIO12           D6
    CLK             GPIO14           D5
  LAT/SCLK          GPIO0            D3
  DATA/R            GPIO13           D7
    OE              GPIO15           D8
    GND              GND            GND
```

### P10 HUB12 Connector Pinout (16-pin)

```
         ┌─────────────────┐
    OE  1│ ○             ○ │2  GND
     A  3│ ○             ○ │4  B
    GND 5│ ○             ○ │6  GND
   CLK  7│ ○             ○ │8  SCLK/LAT
  DATA  9│ ○             ○ │10 GND
    NC 11│ ○             ○ │12 GND
    NC 13│ ○             ○ │14 GND
    NC 15│ ○             ○ │16 GND
         └─────────────────┘
```

### Daisy-Chaining Multiple Panels

Connect panels in series using ribbon cables:
- Panel 1 OUT → Panel 2 IN
- Panel 2 OUT → Panel 3 IN
- (and so on...)

Each panel needs its own 5V power connection.

## DIP Switch Configuration

Your Uno+WiFi board has a 7-position DIP switch to select the communication mode.

### For Programming ESP8266 (uploading sketch):

```
Switch:  1    2    3    4    5    6    7
State:  OFF  OFF  OFF  OFF  ON   ON   ON
```

### For Normal Operation (after upload):

Same as above, or set all switches OFF for independent operation:

```
Switch:  1    2    3    4    5    6    7
State:  OFF  OFF  OFF  OFF  OFF  OFF  OFF
```

### All DIP Switch Modes Reference:

| Mode                          | 1   | 2   | 3   | 4   | 5   | 6   | 7   |
|-------------------------------|-----|-----|-----|-----|-----|-----|-----|
| ATmega328 ↔ ESP8266           | ON  | ON  | OFF | OFF | OFF | OFF | OFF |
| USB ↔ ATmega328               | OFF | OFF | ON  | ON  | OFF | OFF | OFF |
| USB ↔ ESP8266 (upload)        | OFF | OFF | OFF | OFF | ON  | ON  | ON  |
| USB ↔ ESP8266 (serial monitor)| OFF | OFF | OFF | OFF | ON  | ON  | OFF |
| All Independent               | OFF | OFF | OFF | OFF | OFF | OFF | OFF |

## Software Setup

### 1. Install Arduino IDE

Download from [arduino.cc](https://www.arduino.cc/en/software)

### 2. Add ESP8266 Board Support

1. Open Arduino IDE
2. Go to **File → Preferences**
3. In "Additional Board Manager URLs", add:
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
4. Go to **Tools → Board → Boards Manager**
5. Search for "esp8266" and install **ESP8266 by ESP8266 Community**

### 3. Install Required Library (DMDESP)

The **DMDESP** library by busel7 is required. Install manually:

1. Go to: https://github.com/busel7/DMDESP
2. Click **Code → Download ZIP**
3. In Arduino IDE: **Sketch → Include Library → Add .ZIP Library...**
4. Select the downloaded `DMDESP-master.zip` file

### 4. Configure the Board

In Arduino IDE, set:

- **Tools → Board:** "NodeMCU 1.0 (ESP-12E Module)" or "Generic ESP8266 Module"
- **Tools → Flash Size:** "4MB (FS:2MB OTA:~1019KB)" or similar 4MB option
- **Tools → Upload Speed:** 115200
- **Tools → Port:** Select your COM port (with USB cable connected)

### 5. Configure the Sketch

Open `sketch_jan10a.ino` and modify these values:

```cpp
// WiFi credentials - CHANGE THESE
const char* WIFI_SSID = "YourSSID";
const char* WIFI_PASSWORD = "YourPassword";

// Text source URL - change to your text file URL
const char* TEXT_URL = "https://portkey.ai/robots.txt";

// Display configuration - match your physical setup
#define DISPLAYS_WIDE 5   // Number of panels horizontally
#define DISPLAYS_HIGH 1   // Number of panels vertically
```

### 6. Upload

1. Set DIP switches to upload mode (5=ON, 6=ON, 7=ON, others OFF)
2. Connect USB cable
3. Click **Upload** in Arduino IDE
4. Wait for "Done uploading" message

## Power Requirements

**Important:** P10 panels draw significant current. Do NOT power them from USB.

| Panel Count | Minimum PSU Rating |
|-------------|--------------------|
| 1 panel     | 5V 2A              |
| 2 panels    | 5V 4A              |
| 3 panels    | 5V 6A              |
| 4 panels    | 5V 8A              |
| 5 panels    | 5V 10A             |

Use a quality 5V switching power supply. Connect:
- PSU 5V → All panel 5V inputs (parallel)
- PSU GND → All panel GND + ESP8266 GND

## Configuration Options

### Adjusting Panel Count

To use a different number of panels, change these defines:

```cpp
#define DISPLAYS_WIDE 5   // Horizontal panels (e.g., 3 for 3 panels)
#define DISPLAYS_HIGH 1   // Vertical panels (usually 1)
```

### Adjusting Scroll Speed

Lower values = faster scrolling:

```cpp
#define SCROLL_SPEED_MS 30   // Try 20 for faster, 50 for slower
```

### Adjusting Fetch Interval

How often to check for new text (in milliseconds):

```cpp
#define FETCH_INTERVAL_MS 600000   // 600000 = 10 minutes
```

Common values:
- 30000 = 30 seconds
- 60000 = 1 minute
- 300000 = 5 minutes
- 600000 = 10 minutes

## Troubleshooting

### Library compilation errors

If you get errors about `SoftDMD` or `DMD2` not found:
- Make sure you installed **DMDESP** (not DMD2)
- The DMDESP library is specifically for ESP8266 + P10 HUB12

### Display shows nothing

1. Check power supply connections
2. Verify wiring matches the pinout table
3. Check that panels are daisy-chained correctly
4. Try reducing brightness in code: `dmd.setBrightness(128);`

### "No WiFi" message scrolling

1. Double-check SSID and password (case-sensitive)
2. Ensure your WiFi is 2.4GHz (ESP8266 doesn't support 5GHz)
3. Move board closer to router for initial testing

### Upload fails

1. Verify DIP switches are set correctly (5=ON, 6=ON, 7=ON)
2. Try pressing the reset button on the board before uploading
3. Try a different USB cable (some are charge-only)
4. Check that the correct COM port is selected

### Garbled display / flickering

1. Check for loose ribbon cable connections
2. Ensure adequate power supply
3. Try reducing DISPLAYS_WIDE if using fewer panels

### HTTPS fetch fails

1. The sketch uses insecure mode (no certificate verification) for simplicity
2. Ensure the URL is accessible and returns plain text
3. Check Serial Monitor (115200 baud) for error messages

## Serial Monitor

Connect via USB and open Serial Monitor at 115200 baud to see:
- WiFi connection status
- Fetch progress and results
- Text preview
- Error messages

## License

MIT License - Feel free to modify and use for your projects.
