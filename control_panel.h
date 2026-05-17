/*
 * control_panel.h — Track Control & Spotify Account Screen
 * Access via menu button on HUD — allows track info, reconnect, account switching
 * Ellis Edition :: The Far Edge
 */

#pragma once

// ═════════════════════════════════════════════════════════════
//  CONTROL PANEL STATE
// ═════════════════════════════════════════════════════════════

enum ControlPanelState {
  CPANEL_MAIN_MENU,
  CPANEL_TRACK_INFO,
  CPANEL_SPOTIFY_SETTINGS,
  CPANEL_CONFIRM_RECONNECT
};

// ═════════════════════════════════════════════════════════════
//  MAIN MENU SCREEN
// ═════════════════════════════════════════════════════════════

void drawControlPanelMenu(TFT_eSprite &canvas) {
  canvas.fillSprite(rgb16(8, 10, 18));

  canvas.setTextColor(0xFFFF);
  canvas.setTextSize(2);
  canvas.drawString("Control Panel", 30, 12);

  canvas.setTextSize(1);
  canvas.setTextColor(rgb16(120, 120, 120));
  canvas.drawFastHLine(0, 32, W, rgb16(50, 60, 80));

  // Current Spotify Status
  canvas.setTextColor(rgb16(100, 200, 100));
  canvas.drawString("Spotify Status:", 10, 42);
  canvas.setTextColor(0xFFFF);
  
  if(WiFi.status() != WL_CONNECTED) {
    canvas.setTextColor(rgb16(200, 60, 0));
    canvas.drawString("No Wi-Fi connection", 10, 54);
  } else if(millis() - lastSpotifyError < 5000) {
    canvas.setTextColor(rgb16(200, 60, 0));
    canvas.drawString("API Error - check token", 10, 54);
  } else if(apiConnected && track.isPlaying) {
    canvas.setTextColor(rgb16(0, 200, 0));
    canvas.drawString("Playing: " + track.trackName.substring(0, 20), 10, 54);
  } else if(apiConnected) {
    canvas.setTextColor(rgb16(200, 180, 0));
    canvas.drawString("Ready (no track playing)", 10, 54);
  } else {
    canvas.setTextColor(rgb16(120, 120, 120));
    canvas.drawString("Initializing...", 10, 54);
  }

  // Menu buttons
  int btnY = 90;

  // [Current Track Info]
  canvas.fillRoundRect(10, btnY, 145, 32, 5, rgb16(20, 60, 90));
  canvas.drawRoundRect(10, btnY, 145, 32, 5, themeColor(0.6f, 0));
  canvas.setTextColor(0xFFFF);
  canvas.setTextSize(1);
  canvas.drawString("Track Details", 32, btnY + 10);

  // [Spotify Account]
  canvas.fillRoundRect(165, btnY, 145, 32, 5, rgb16(60, 30, 20));
  canvas.drawRoundRect(165, btnY, 145, 32, 5, rgb16(180, 100, 40));
  canvas.setTextColor(0xFFFF);
  canvas.drawString("Account Settings", 176, btnY + 10);

  // [Wi-Fi Settings]
  canvas.fillRoundRect(10, btnY + 45, 145, 32, 5, rgb16(30, 50, 40));
  canvas.drawRoundRect(10, btnY + 45, 145, 32, 5, rgb16(60, 120, 80));
  canvas.setTextColor(0xFFFF);
  canvas.drawString("Wi-Fi Settings", 32, btnY + 55);

  // [Back to Viz]
  canvas.fillRoundRect(165, btnY + 45, 145, 32, 5, rgb16(25, 25, 40));
  canvas.drawRoundRect(165, btnY + 45, 145, 32, 5, themeColor(0.5f, 0));
  canvas.setTextColor(0xFFFF);
  canvas.drawString("Back to Visualizer", 176, btnY + 55);

  // Legend at bottom
  canvas.setTextSize(1);
  canvas.setTextColor(rgb16(80, 80, 80));
  canvas.drawString("Status: ", 10, 200);
  canvas.fillCircle(60, 201, 2, rgb16(0, 200, 0));
  canvas.drawString("OK  ", 68, 200);
  canvas.fillCircle(105, 201, 2, rgb16(200, 180, 0));
  canvas.drawString("Ready  ", 113, 200);
  canvas.fillCircle(165, 201, 2, rgb16(200, 60, 0));
  canvas.drawString("Error", 173, 200);
}

// ═════════════════════════════════════════════════════════════
//  TRACK INFO SCREEN
// ═════════════════════════════════════════════════════════════

void drawTrackInfoScreen(TFT_eSprite &canvas) {
  canvas.fillSprite(rgb16(8, 10, 18));

  canvas.setTextColor(0xFFFF);
  canvas.setTextSize(2);
  canvas.drawString("Now Playing", 50, 12);

  canvas.setTextSize(1);
  canvas.setTextColor(rgb16(120, 120, 120));
  canvas.drawFastHLine(0, 32, W, rgb16(50, 60, 80));

  if(!track.isPlaying && track.trackName.length() == 0) {
    canvas.setTextColor(rgb16(120, 120, 120));
    canvas.drawString("No track playing", 80, 100);
    canvas.drawString("Start playback on Spotify", 60, 115);
  } else {
    // Track name (large, wrapped)
    canvas.setTextColor(0xFFFF);
    canvas.setTextSize(2);
    String tname = track.trackName.length() > 0 ? track.trackName : "Unknown Track";
    if(tname.length() > 18) {
      canvas.drawString(tname.substring(0, 18), 5, 40);
      canvas.drawString(tname.substring(18, min((int)tname.length(), 36)), 5, 58);
    } else {
      canvas.drawString(tname, 5, 45);
    }

    // Artist name
    canvas.setTextSize(1);
    canvas.setTextColor(rgb16(150, 150, 150));
    String aname = track.artistName.length() > 0 ? track.artistName : "Unknown Artist";
    if(aname.length() > 38) aname = aname.substring(0, 35) + "...";
    canvas.drawString("by: " + aname, 5, 75);

    // Audio features as bars
    canvas.setTextColor(rgb16(100, 100, 100));
    canvas.drawString("Energy:", 5, 95);
    int ew = (int)(track.energy * 100);
    canvas.fillRect(70, 95, ew / 3, 6, themeColor(track.energy, 0.3f));
    canvas.drawRect(70, 95, 33, 6, rgb16(60, 60, 60));

    canvas.drawString("Dance:", 5, 110);
    int dw = (int)(track.danceability * 100);
    canvas.fillRect(70, 110, dw / 3, 6, themeColor(track.danceability, 0.3f));
    canvas.drawRect(70, 110, 33, 6, rgb16(60, 60, 60));

    canvas.drawString("Valence:", 5, 125);
    int vw = (int)(track.valence * 100);
    canvas.fillRect(70, 125, vw / 3, 6, themeColor(track.valence, 0.3f));
    canvas.drawRect(70, 125, 33, 6, rgb16(60, 60, 60));

    // Tempo
    canvas.drawString("BPM:", 150, 95);
    canvas.setTextColor(0xFFFF);
    canvas.drawString(String((int)track.tempo), 195, 95);

    // Duration/progress
    canvas.setTextColor(rgb16(100, 100, 100));
    canvas.drawString("Progress:", 150, 110);
    canvas.setTextColor(0xFFFF);
    int mins = localProgressMs / 60000;
    int secs = (localProgressMs % 60000) / 1000;
    int dmins = track.durationMs / 60000;
    int dsecs = (track.durationMs % 60000) / 1000;
    canvas.drawString(String(mins) + ":" + (secs < 10 ? "0" : "") + String(secs) +
                      " / " + String(dmins) + ":" + (dsecs < 10 ? "0" : "") + String(dsecs), 
                      195, 110);

    // Key
    const char *keyNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    canvas.setTextColor(rgb16(100, 100, 100));
    canvas.drawString("Key:", 150, 125);
    canvas.setTextColor(0xFFFF);
    canvas.drawString(keyNames[track.key % 12] + String(track.musicalMode == 1 ? "major" : "minor"), 
                      195, 125);
  }

  // Back button
  canvas.fillRoundRect(100, 180, 120, 30, 5, rgb16(25, 25, 40));
  canvas.drawRoundRect(100, 180, 120, 30, 5, themeColor(0.5f, 0));
  canvas.setTextColor(0xFFFF);
  canvas.setTextSize(1);
  canvas.drawString("Back to Menu", 120, 192);
}

// ═════════════════════════════════════════════════════════════
//  SPOTIFY ACCOUNT SETTINGS SCREEN
// ═════════════════════════════════════════════════════════════

void drawSpotifySettingsScreen(TFT_eSprite &canvas) {
  canvas.fillSprite(rgb16(8, 10, 18));

  canvas.setTextColor(0xFFFF);
  canvas.setTextSize(2);
  canvas.drawString("Spotify Settings", 25, 12);

  canvas.setTextSize(1);
  canvas.setTextColor(rgb16(120, 120, 120));
  canvas.drawFastHLine(0, 32, W, rgb16(50, 60, 80));

  // Current token status
  canvas.setTextColor(rgb16(100, 200, 100));
  canvas.drawString("Authentication Status:", 10, 42);
  canvas.setTextColor(0xFFFF);

  if(accessToken.length() > 0) {
    canvas.setTextColor(rgb16(0, 200, 0));
    canvas.drawString("Token Valid", 10, 54);
    
    unsigned long timeLeft = tokenExpiry > millis() ? (tokenExpiry - millis()) / 1000 : 0;
    int mins = timeLeft / 60;
    canvas.setTextColor(rgb16(100, 100, 100));
    canvas.drawString("Expires in: " + String(mins) + " minutes", 10, 66);
  } else {
    canvas.setTextColor(rgb16(200, 60, 0));
    canvas.drawString("No Token - Please Configure", 10, 54);
  }

  // Options
  int btnY = 95;

  // [Reconnect Current Account]
  canvas.fillRoundRect(10, btnY, 145, 32, 5, rgb16(20, 60, 90));
  canvas.drawRoundRect(10, btnY, 145, 32, 5, themeColor(0.6f, 0));
  canvas.setTextColor(0xFFFF);
  canvas.drawString("Reconnect Account", 20, btnY + 10);

  // [Re-authenticate New Account]
  canvas.fillRoundRect(165, btnY, 145, 32, 5, rgb16(60, 30, 20));
  canvas.drawRoundRect(165, btnY, 145, 32, 5, rgb16(180, 80, 40));
  canvas.setTextColor(0xFFFF);
  canvas.drawString("Setup New Account", 175, btnY + 10);

  // Instructions
  canvas.setTextColor(rgb16(100, 100, 100));
  canvas.setTextSize(1);
  canvas.drawString("To setup a new account:", 10, 145);
  canvas.drawString("1. Run get_spotify_token.py on PC", 10, 157);
  canvas.drawString("2. Update REFRESH_TOKEN in .ino", 10, 169);
  canvas.drawString("3. Re-flash the device", 10, 181);

  // Back button
  canvas.fillRoundRect(100, 200, 120, 30, 5, rgb16(25, 25, 40));
  canvas.drawRoundRect(100, 200, 120, 30, 5, themeColor(0.5f, 0));
  canvas.setTextColor(0xFFFF);
  canvas.drawString("Back to Menu", 120, 212);
}

// ═════════════════════════════════════════════════════════════
//  TOUCH HANDLING FOR CONTROL PANEL
// ═════════════════════════════════════════════════════════════

// Returns next state, or CPANEL_MAIN_MENU to return to menu, or -1 to exit
int handleControlPanelTouch(int tx, int ty, int currentState) {
  switch(currentState) {
    case CPANEL_MAIN_MENU: {
      if(tx >= 10 && tx <= 155 && ty >= 90 && ty <= 122) return CPANEL_TRACK_INFO;
      if(tx >= 165 && tx <= 310 && ty >= 90 && ty <= 122) return CPANEL_SPOTIFY_SETTINGS;
      if(tx >= 10 && tx <= 155 && ty >= 135 && ty <= 167) return -2; // Go to WiFi settings
      if(tx >= 165 && tx <= 310 && ty >= 135 && ty <= 167) return -1; // Exit to viz
      break;
    }
    case CPANEL_TRACK_INFO: {
      if(tx >= 100 && tx <= 220 && ty >= 180 && ty <= 210) return CPANEL_MAIN_MENU;
      break;
    }
    case CPANEL_SPOTIFY_SETTINGS: {
      if(tx >= 10 && tx <= 155 && ty >= 95 && ty <= 127) {
        // Reconnect: force token refresh
        refreshAccessToken();
        return CPANEL_SPOTIFY_SETTINGS;
      }
      if(tx >= 165 && tx <= 310 && ty >= 95 && ty <= 127) {
        canvas.fillSprite(rgb16(20, 20, 30));
        canvas.setTextColor(0xFFFF);
        canvas.setTextSize(1);
        canvas.drawString("Please re-run get_spotify_token.py", 15, 100);
        canvas.drawString("and update REFRESH_TOKEN in .ino", 20, 115);
        canvas.pushSprite(0, 0);
        delay(2000);
        return CPANEL_SPOTIFY_SETTINGS;
      }
      if(tx >= 100 && tx <= 220 && ty >= 200 && ty <= 230) return CPANEL_MAIN_MENU;
      break;
    }
  }
  return currentState;
}

// ═════════════════════════════════════════════════════════════
//  MAIN CONTROL PANEL LOOP
// ═════════════════════════════════════════════════════════════

void runControlPanel() {
  ControlPanelState state = CPANEL_MAIN_MENU;
  unsigned long lastDraw = 0;

  while(true) {
    unsigned long now = millis();

    // Draw current screen (30 Hz)
    if(now - lastDraw > 33) {
      lastDraw = now;
      switch(state) {
        case CPANEL_MAIN_MENU:
          drawControlPanelMenu(canvas);
          break;
        case CPANEL_TRACK_INFO:
          drawTrackInfoScreen(canvas);
          break;
        case CPANEL_SPOTIFY_SETTINGS:
          drawSpotifySettingsScreen(canvas);
          break;
        default:
          break;
      }
      canvas.pushSprite(0, 0);
    }

    // Handle touch
    if(touch.tirqTouched() && touch.touched()) {
      delay(60);
      TS_Point p = touch.getPoint();
      int tx = map(p.x, 200, 3800, 0, W);
      int ty = map(p.y, 200, 3800, 0, H);
      tx = constrain(tx, 0, W - 1);
      ty = constrain(ty, 0, H - 1);

      int result = handleControlPanelTouch(tx, ty, state);
      if(result == -1) {
        // Exit to visualizer
        return;
      } else if(result == -2) {
        // Exit to WiFi settings
        bool changed = runSettingsScreen(canvas, touch);
        if(changed && WiFi.status() == WL_CONNECTED) {
          refreshAccessToken();
          pollCurrentlyPlaying();
        }
      } else if(result >= 0) {
        state = (ControlPanelState)result;
      }
      delay(200);
    }

    // Update Spotify data in background
    static unsigned long lastPoll = 0;
    if(now - lastPoll > POLL_MS) {
      lastPoll = now;
      if(WiFi.status() == WL_CONNECTED) {
        if(now >= tokenExpiry) refreshAccessToken();
        pollCurrentlyPlaying();
      }
    }

    delay(16);
  }
}
