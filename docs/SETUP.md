# CYD Spotify Live Visualizer — Wi-Fi Edition
## Setup & Architecture Guide :: Ellis Edition

---

## How It Works

```
Spotify App (phone/PC)
        │
        │  playing: "Blinding Lights" by The Weeknd
        ▼
  Spotify Cloud API
        │
        │  HTTPS every 3 seconds
        ▼
   CYD (ESP32)  ──── Wi-Fi ────►  api.spotify.com
        │                          /v1/me/player/currently-playing
        │                          /v1/audio-features/{id}
        ▼
  Beat Engine (BPM sync)
  Spectrum Synthesizer
  Colour System (key + valence)
        │
        ▼
  ILI9341 TFT  →  7 Live Visualization Modes
```

**Key insight:** Spotify gives you *metadata* — not audio waveforms.
The CYD synthesises a musically-correct spectrum from:
- `tempo` → beat clock (perfectly synced to track position)
- `energy` → overall amplitude
- `danceability` → mid-frequency movement
- `valence` → treble shimmer intensity + colour warmth
- `loudness` → dynamic range scaling
- `key` → base hue (circle-of-fifths colour mapping)
- `mode` → saturation (minor = vivid, major = softer)

---

## One-Time Setup

### 1. Create a Spotify Developer App

1. Go to **https://developer.spotify.com/dashboard**
2. Log in → **Create App**
3. Name: anything (e.g. "CYD Visualizer")
4. Redirect URI: `http://localhost:8888/callback`
5. APIs: check "Web API"
6. Copy **Client ID** and **Client Secret**

### 2. Get Your Refresh Token (PC, run once)

```bash
pip install requests   # only if requests isn't installed
python get_spotify_token.py
```

- Edit the script first with your Client ID + Secret
- It opens a browser → log in → authorize
- Paste the redirect URL back → get your `REFRESH_TOKEN`
- The refresh token **doesn't expire** unless you revoke access

### 3. Fill In Your Credentials

Open `spotify_viz_wifi.ino` and fill in the top section:

```cpp
#define WIFI_SSID        "YourNetworkName"
#define WIFI_PASS        "YourPassword"
#define CLIENT_ID        "abc123..."
#define CLIENT_SECRET    "xyz789..."
#define REFRESH_TOKEN    "AQD..."
```

---

## Libraries Required

Install via Arduino Library Manager:

| Library | Author | Notes |
|---------|--------|-------|
| TFT_eSPI | Bodmer | Configure for ILI9341 |
| XPT2046_Touchscreen | Paul Stoffregen | |
| ArduinoJson | Benoit Blanchon | Version 6.x or 7.x |

WiFiClientSecure and HTTPClient are **built into** the ESP32 Arduino core.

---

## Visualisation Modes — What Drives Each

| # | Mode | BPM | Energy | Dance | Valence | Key |
|---|------|:---:|:------:|:-----:|:-------:|:---:|
| 0 | Waveform | ✓ beat flash | ✓ amplitude | | ✓ shimmer | ✓ hue |
| 1 | Spectrum | ✓ pulse | ✓ bar height | ✓ mid-range | | ✓ colour |
| 2 | Scatter | ✓ burst rate | ✓ particle count | ✓ spread | ✓ tone | ✓ hue |
| 3 | Radial | ✓ ring pulse | ✓ spoke length | ✓ rotation speed | | ✓ hue |
| 4 | Matrix | ✓ drop speed | ✓ brightness | ✓ column density | | ✓ tint |
| 5 | Starfield | ✓ shockwave | ✓ zoom speed | | ✓ colour | ✓ hue |
| 6 | Waterfall | ✓ scroll speed | ✓ intensity | | | ✓ palette |

---

## The Colour System

Musical key maps to the **circle of fifths** as hue:

```
C=Red  G=Yellow  D=Yellow-Green  A=Green  E=Cyan  B=Blue
F=Magenta  Bb=Purple  Eb=Violet  Ab=Indigo  Db=Sky  F#=Cyan
```

**Valence** (sadness→happiness) sweeps the hue toward warm tones.
**Mode** (minor/major) controls saturation — minor tracks are more vivid.

So a sad song in D minor = vivid yellow-green.
A happy song in G major = warm, softer gold.

---

## HUD Elements

```
● — Beat dot (top-left, pulses on every beat)
[mode dots] — 7 dots top-right, current mode lit
───────────────────────── thin line
♪ Track Name  —  Artist Name  (scrolls at BPM speed)
═══════════════════ progress bar (bottom edge)
```

---

## API Rate Limits & Behaviour

- **Currently Playing** polls every 3 seconds — well within Spotify's limits
- **Audio Features** loads once per track — very lightweight
- **Token refresh** happens automatically ~60 seconds before expiry
- If no track is playing (HTTP 204), visualizer runs on last-known parameters
- Wi-Fi disconnect: visualizer keeps running with cached values

---

## Troubleshooting

| Issue | Fix |
|-------|-----|
| "Token refresh failed" | Check CLIENT_ID/SECRET, re-run token script |
| No track showing | Start playback on Spotify first, then power on CYD |
| Very slow beat | Your track's BPM wasn't detected well — Spotify limitation |
| Colours look wrong | Check key mapping, try a different viz mode |
| Wi-Fi won't connect | Ensure 2.4GHz network (ESP32 doesn't do 5GHz) |
| HTTP errors after hours | Token expired — reboot CYD or add auto-reconnect |

---

## Possible Future Upgrades

- [ ] **Album art** — fetch JPEG via Spotify image URL, decode on CYD (tight on RAM but doable)
- [ ] **Audio Analysis endpoint** — Spotify provides beat/bar/section timestamps per track for perfect sync
- [ ] **NTP time sync** — improve long-session progress drift
- [ ] **mDNS web config** — change Wi-Fi credentials without reflashing
- [ ] **MQTT bridge** — Pi reads audio analysis, pushes beat events to CYD over local network for sample-accurate sync

---

*Connecting Ellis's room to the music. The Far Edge, Fort Collins CO.*
