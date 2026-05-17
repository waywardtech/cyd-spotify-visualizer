# Quick Flash Reference Card

## Files You'll Need

✅ All in repo: `d:\GitHub\cyd-spotify-visualizer\`

- `spotify_viz_wifi.ino` — Main sketch (edit credentials here)
- `settings.h` — WiFi config (edit your SSID/password here)
- `control_panel.h` — Track control UI
- `spotify_certs.h` — HTTPS certificate validation
- `get_spotify_token.py` — Generate Spotify token (run once on PC)
- `FLASHING.md` — Detailed setup guide

## 3-Minute Quick Start

### 1. Get Spotify Token
```bash
python get_spotify_token.py
# (edit script with Client ID/Secret first)
# Copy the REFRESH_TOKEN output
```

### 2. Edit `spotify_viz_wifi.ino` (lines ~30)
```cpp
#define CLIENT_ID      "your_client_id_here"
#define CLIENT_SECRET  "your_client_secret_here"
#define REFRESH_TOKEN  "your_refresh_token_here"
```

### 3. Edit `settings.h` (lines ~13)
```cpp
#define DEFAULT_SSID  "YourWiFiName"
#define DEFAULT_PASS  "YourWiFiPassword"
```

### 4. Configure TFT_eSPI
Path: `Documents/Arduino/libraries/TFT_eSPI/User_Setup.h`

Key settings:
```cpp
#define ILI9341_DRIVER
#define TFT_MOSI  13
#define TFT_SCLK  14
#define TFT_CS    15
#define TFT_DC     2
#define TFT_BL    21
#define TOUCH_CS  33
#define SPI_FREQUENCY 55000000
```

### 5. Arduino IDE Settings
- Board: **ESP32 Dev Module**
- Partition: **Default 4MB with spiffs**
- CPU: **240 MHz**
- Speed: **921600**

### 6. Upload to CYD
- Connect via USB-C
- Click Upload
- (If stuck: hold BOOT while connecting, then upload)

### 7. Done! 🎉
- WiFi auto-connects
- Spotify syncs within 3 seconds of playback
- Tap screen to cycle visualizations
- Tap menu button (bottom-left) for control panel

---

## Status Indicators (HUD Top-Right)

| Indicator | Meaning |
|-----------|---------|
| 🟢 Green + bars | WiFi connected, playing music |
| 🟡 Yellow + bars | WiFi connected, no track playing |
| 🔴 Red dot | API error or connection lost |
| ❌ Red "!" | No WiFi connection |

---

## Default Button Actions

| Control | Action |
|---------|--------|
| **Tap anywhere** | Cycle visualization mode |
| **Menu button** (☰ bottom-left) | Open Control Panel |
| **Gear icon** (⚙ bottom-right) | WiFi settings |
| **BOOT button** | Cycle mode |

---

## If Something Goes Wrong

**Display shows garbage?**
→ Fix TFT_eSPI User_Setup.h, make sure all pins match

**Won't flash?**
→ Hold BOOT button while connecting USB, then upload

**No Spotify sync?**
→ Check REFRESH_TOKEN is correct, start Spotify playback first

**WiFi won't connect?**
→ Use gear icon to enter SSID/password via on-screen keyboard

**See red error LED?**
→ Open Control Panel → Spotify Account → Reconnect

---

## Full Setup Guide

See **FLASHING.md** in repo for complete troubleshooting & detailed steps.
