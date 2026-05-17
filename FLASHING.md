# 🔧 Flashing Guide — CYD Spotify Visualizer

## Pre-Flash Checklist

- [ ] **Hardware**: ESP32-2432S028R (CYD) connected to PC via USB-C
- [ ] **Arduino IDE**: Installed and updated
- [ ] **Libraries**: TFT_eSPI, XPT2046_Touchscreen, ArduinoJson installed via Library Manager
- [ ] **Spotify Credentials**: Client ID, Client Secret, Refresh Token (from `get_spotify_token.py`)
- [ ] **WiFi Credentials**: Your home network SSID and password
- [ ] **This repo**: Cloned locally

---

## Step 1: Get Your Spotify Refresh Token

**Run this ONCE on your PC** (in any terminal/PowerShell):

```bash
cd path/to/cyd-spotify-visualizer
python get_spotify_token.py
```

**First, edit the script** with your Spotify credentials:
1. Go to [developer.spotify.com/dashboard](https://developer.spotify.com/dashboard)
2. Create an app (if you haven't already)
3. Set Redirect URI to: `http://localhost:8888/callback`
4. Copy **Client ID** and **Client Secret**
5. Edit `get_spotify_token.py`, fill in these values at the top
6. Save and run the script

The script will:
- Open your browser for authorization
- Ask you to paste the redirect URL from your browser's address bar
- Output your **REFRESH_TOKEN** (keep this safe!)

---

## Step 2: Configure Credentials in Arduino IDE

### Update Spotify Credentials

Open `spotify_viz_wifi.ino` and find the credentials section (around line 30):

```cpp
#define CLIENT_ID      "YOUR_SPOTIFY_CLIENT_ID"
#define CLIENT_SECRET  "YOUR_SPOTIFY_CLIENT_SECRET"
#define REFRESH_TOKEN  "YOUR_REFRESH_TOKEN"
```

Replace with your actual values from Step 1.

### Update WiFi Credentials

Open `settings.h` and find (around line 13):

```cpp
#define DEFAULT_SSID  "YourHomeSSID"
#define DEFAULT_PASS  "YourHomePassword"
```

Replace with your home WiFi network details.

---

## Step 3: Configure TFT_eSPI Library

**This is critical for the display to work!**

1. In Arduino IDE, go to **Sketch → Include Library → Manage Libraries**
2. Search for **TFT_eSPI** by Bodmer
3. Install it (version 2.5.0+ recommended)
4. Locate the library folder:
   - **Windows**: `Documents\Arduino\libraries\TFT_eSPI`
   - **Mac**: `~/Documents/Arduino/libraries/TFT_eSPI`
   - **Linux**: `~/Arduino/libraries/TFT_eSPI`

5. Open `User_Setup.h` in that folder and replace its contents with:

```cpp
#define ILI9341_DRIVER
#define TFT_WIDTH  320
#define TFT_HEIGHT 240

// Pin definitions for CYD
#define TFT_MOSI  13
#define TFT_SCLK  14
#define TFT_CS    15
#define TFT_DC     2
#define TFT_RST   -1
#define TFT_BL    21

// Touch panel
#define TOUCH_CS  33

// SPI frequencies
#define SPI_FREQUENCY        55000000
#define SPI_READ_FREQUENCY   20000000
#define SPI_TOUCH_FREQUENCY   2500000

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
```

6. Save and close

---

## Step 4: Arduino IDE Board Settings

1. Open Arduino IDE
2. **Tools → Board → ESP32 Arduino → ESP32 Dev Module**
3. Configure these settings:

| Setting | Value |
|---------|-------|
| Board | ESP32 Dev Module |
| Partition Scheme | **Default 4MB with spiffs** |
| CPU Frequency | **240 MHz** |
| Upload Speed | **921600** |
| Port | COM3 (or your USB port) |

---

## Step 5: Open & Verify the Sketch

1. In Arduino IDE, **File → Open**
2. Navigate to and select `spotify_viz_wifi.ino`
3. Click **Verify** (checkmark icon) to compile

You should see:
```
Compiling sketch...
Sketch uses 1234567 bytes of program storage space.
Global variables use 56789 bytes...
```

**No errors?** Proceed to Step 6.

---

## Step 6: Flash to CYD

### If Upload Works Directly

1. Connect CYD via USB-C cable to PC
2. Press **Upload** button (arrow icon) in Arduino IDE
3. Wait for upload to complete (1-2 minutes)
4. You should see:
   ```
   Writing at 0x...
   [========] 100%
   Hard resetting via RTS pin...
   ```

### If Upload Fails

1. **Hold BOOT button** on CYD while connecting USB
2. Release BOOT once power LED lights
3. In Arduino IDE, press **Upload**
4. Release BOOT after upload starts
5. Wait for completion

---

## Step 7: First Boot

1. **Power on** the CYD (via USB-C)
2. You should see the **splash screen**: "SPOTIFY VIZ"
3. The device will attempt to connect to WiFi

### If WiFi Connection Fails

1. Tap the **⚙ gear icon** (bottom-right of HUD)
2. Enter your WiFi SSID and password on the on-screen keyboard
3. Press OK to save and connect

### If Spotify Connection Fails

Check the **status indicator** (tiny LED in top-right):
- 🔴 **Red flash** = API error (token invalid or network issue)
- 🟡 **Yellow** = Connected but no track playing
- 🟢 **Green** = Playing music

**Troubleshooting**:
- Verify REFRESH_TOKEN is correct in `spotify_viz_wifi.ino`
- Ensure you're logged into Spotify on your phone/PC
- Start playback, then power on CYD (give it 3-5 seconds)

---

## Step 8: Control Your Device

### Tap Anywhere to Cycle Visualization Modes
7 modes available (indicated by dots in HUD):
1. Waveform
2. Spectrum Analyzer
3. Scatter/Lissajous
4. Radial Burst
5. Matrix Rain
6. Starfield
7. Waterfall

### Menu Button (Bottom-Left) — NEW!
Opens **Control Panel** with:
- **Track Info** — Current song details, BPM, energy, danceability, valence, key
- **Spotify Account** — Token status, reconnect options, setup new account
- **WiFi Settings** — Change network without reflashing

### Gear Icon (Bottom-Right)
Opens **WiFi Settings** — Change network and reconnect

### BOOT Button (Physical Button on CYD)
Cycles visualization mode

---

## ⚠️ Important Security Notes

🔒 **Never commit your credentials to git!**
- `spotify_tokens.txt` is in `.gitignore` (if you generated one)
- Keep your REFRESH_TOKEN and WiFi password secret
- Don't share photos/videos showing your credentials

🔒 **HTTPS Certificate Validation is Enabled**
- This version uses proper certificate pinning (spotify_certs.h)
- Much more secure than the old `setInsecure()` approach

---

## Troubleshooting

### Device Won't Flash

**Error: "Failed to connect to ESP32: Timed out"**
- Try holding BOOT button while connecting USB
- Check USB cable (some cables are power-only, not data)
- Try different USB port

### Display Shows Garbage

**TFT_eSPI configuration issue**:
- Verify `User_Setup.h` matches the config above exactly
- Make sure you edited the **correct** TFT_eSPI library copy
- Arduino IDE → **Sketch → Include Library → TFT_eSPI** should show correct path

### WiFi Won't Connect

- Check SSID/password spelling in `settings.h`
- Verify your WiFi network is 2.4GHz (CYD doesn't support 5GHz)
- Try using the gear icon to re-enter credentials

### Spotify Won't Sync

- Check that Spotify is actually playing on your phone/PC
- Verify REFRESH_TOKEN in `spotify_viz_wifi.ino`
- Look at HUD status indicator (should be green if connected)
- Open Control Panel (menu button) → Spotify Account → Reconnect

### Serial Monitor Debugging

To see detailed logs:
1. Arduino IDE → **Tools → Serial Monitor**
2. Set baud rate to **115200**
3. Restart CYD (power off/on)
4. You'll see connection logs and any errors

---

## Next Steps

- Customize visualization colors by editing the colour system in `spotify_viz_wifi.ino`
- Add more visualization modes by extending the switch statement in `loop()`
- Use Control Panel to monitor your Spotify playback in real-time

**Enjoy your beat-synced visualizer! 🎵**
