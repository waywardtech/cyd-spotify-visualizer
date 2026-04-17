/*
 * settings.h — Wi-Fi Credential Manager
 * NVS storage · On-screen keyboard · Gear icon tap zone
 * Ellis Edition :: The Far Edge
 */

#pragma once
#include <Preferences.h>
#include <WiFi.h>

// ── Compiled-in defaults (home network fallback) ─────────────
// Ellis: change SAVED_SSID/PASS via the cogwheel to use test networks.
// The device always tries NVS first; these are the safety net.
#define DEFAULT_SSID  "YourHomeSSID"
#define DEFAULT_PASS  "YourHomePassword"

// ── NVS namespace ─────────────────────────────────────────────
#define NVS_NS        "wifi_cfg"
#define NVS_KEY_SSID  "ssid"
#define NVS_KEY_PASS  "pass"

// ── Gear icon tap zone (bottom-right corner) ─────────────────
#define GEAR_X  (W - 18)
#define GEAR_Y  (H - 22 + 4)   // sits in HUD strip
#define GEAR_R  8               // tap radius

// ═════════════════════════════════════════════════════════════
//  CREDENTIAL STORAGE
// ═════════════════════════════════════════════════════════════

Preferences prefs;

struct WifiCreds {
  String ssid;
  String pass;
  bool   fromNVS;   // true = user-saved, false = compiled default
};

WifiCreds loadCreds() {
  WifiCreds c;
  prefs.begin(NVS_NS, true);  // read-only
  c.ssid = prefs.getString(NVS_KEY_SSID, "");
  c.pass = prefs.getString(NVS_KEY_PASS, "");
  prefs.end();

  if(c.ssid.length() > 0) {
    c.fromNVS = true;
  } else {
    c.ssid    = DEFAULT_SSID;
    c.pass    = DEFAULT_PASS;
    c.fromNVS = false;
  }
  return c;
}

void saveCreds(const String &ssid, const String &pass) {
  prefs.begin(NVS_NS, false);  // read-write
  prefs.putString(NVS_KEY_SSID, ssid);
  prefs.putString(NVS_KEY_PASS, pass);
  prefs.end();
}

void clearCreds() {
  prefs.begin(NVS_NS, false);
  prefs.clear();
  prefs.end();
}

// ═════════════════════════════════════════════════════════════
//  GEAR ICON RENDERER
//  Draws a 16×16 cogwheel at (cx, cy). Pure pixel math, no bitmaps.
// ═════════════════════════════════════════════════════════════
void drawGearIcon(TFT_eSprite &canvas, int cx, int cy, uint16_t col, bool highlighted) {
  uint16_t c = highlighted ? 0xFFFF : col;
  int r = 7;  // outer radius
  int ri = 4; // inner (bore) radius
  int rt = 5; // tooth inner radius

  // Draw gear teeth as thick lines at 8 positions
  for(int i = 0; i < 8; i++) {
    float a0 = (i * 45 - 10) * DEG_TO_RAD;
    float a1 = (i * 45 + 10) * DEG_TO_RAD;
    // Tooth as a small rectangle approximated by lines
    for(float a = a0; a <= a1; a += 0.05f) {
      int x0 = cx + (int)(cos(a) * rt);
      int y0 = cy + (int)(sin(a) * rt);
      int x1 = cx + (int)(cos(a) * r);
      int y1 = cy + (int)(sin(a) * r);
      canvas.drawLine(x0, y0, x1, y1, c);
    }
  }

  // Body ring
  for(int rr = ri; rr <= rt; rr++)
    canvas.drawCircle(cx, cy, rr, c);

  // Centre dot
  canvas.fillCircle(cx, cy, 2, c);
}

// ═════════════════════════════════════════════════════════════
//  ON-SCREEN KEYBOARD
//  Resistive touch QWERTY — returns entered string
//  field: "SSID" or "PASSWORD"
// ═════════════════════════════════════════════════════════════

// Keyboard layout
const char* ROWS[] = {
  "1234567890",
  "qwertyuiop",
  "asdfghjkl",
  "zxcvbnm.-_"
};
#define N_ROWS 4

// Key dimensions
#define KEY_W  28
#define KEY_H  26
#define KEY_X0  6
#define KEY_Y0  70

struct KeyboardResult {
  String  text;
  bool    confirmed;  // true=OK, false=Cancel
};

// Draw the keyboard overlay
void drawKeyboard(TFT_eSprite &canvas, const String &field,
                  const String &current, bool showPass,
                  int highlightRow, int highlightCol) {

  // Background
  canvas.fillSprite(rgb16(10, 12, 20));

  // Title
  canvas.setTextColor(rgb16(160, 160, 160));
  canvas.setTextSize(1);
  canvas.drawString("Enter " + field + ":", 6, 8);

  // Input field display
  String display = showPass ? String("*").substring(0,0) : current; // show chars for SSID
  if(field == "PASSWORD") {
    // Show last char, mask rest
    display = "";
    for(int i = 0; i < (int)current.length() - 1; i++) display += '*';
    if(current.length() > 0) display += current[current.length()-1];
  } else {
    display = current;
  }
  display += "_";  // cursor

  // Field box
  canvas.fillRect(4, 20, W - 8, 22, rgb16(25, 30, 45));
  canvas.drawRect(4, 20, W - 8, 22, themeColor(0.6f, 0));
  canvas.setTextColor(0xFFFF);
  canvas.setTextSize(1);
  // Clip to last 40 chars if long
  String disp2 = display.length() > 40 ? display.substring(display.length()-40) : display;
  canvas.drawString(disp2, 8, 26);

  // Keys
  for(int row = 0; row < N_ROWS; row++) {
    const char* r = ROWS[row];
    int len = strlen(r);
    int rowX = KEY_X0 + (row == 3 ? 8 : 0);  // indent bottom row slightly

    for(int col = 0; col < len; col++) {
      int kx = rowX + col * KEY_W;
      int ky = KEY_Y0 + row * KEY_H;
      bool lit = (row == highlightRow && col == highlightCol);

      uint16_t bg  = lit ? themeColor(0.7f, 0.3f) : rgb16(35, 40, 60);
      uint16_t bdr = lit ? 0xFFFF : rgb16(70, 80, 110);

      canvas.fillRoundRect(kx, ky, KEY_W - 2, KEY_H - 2, 3, bg);
      canvas.drawRoundRect(kx, ky, KEY_W - 2, KEY_H - 2, 3, bdr);
      canvas.setTextColor(lit ? 0x0000 : 0xFFFF);
      canvas.setTextSize(1);
      char ch[2] = {r[col], 0};
      canvas.drawString(ch, kx + 8, ky + 8);
    }
  }

  // Special keys row
  int specY = KEY_Y0 + N_ROWS * KEY_H + 2;

  // SPACE
  canvas.fillRoundRect(KEY_X0, specY, 90, KEY_H-2, 3, rgb16(35,40,60));
  canvas.drawRoundRect(KEY_X0, specY, 90, KEY_H-2, 3, rgb16(70,80,110));
  canvas.setTextColor(0xFFFF); canvas.setTextSize(1);
  canvas.drawString("SPACE", KEY_X0+25, specY+8);

  // BKSP
  canvas.fillRoundRect(KEY_X0+96, specY, 50, KEY_H-2, 3, rgb16(60,30,30));
  canvas.drawRoundRect(KEY_X0+96, specY, 50, KEY_H-2, 3, rgb16(150,60,60));
  canvas.drawString("<DEL", KEY_X0+102, specY+8);

  // UPPER/lower toggle
  canvas.fillRoundRect(KEY_X0+152, specY, 40, KEY_H-2, 3, rgb16(30,50,40));
  canvas.drawRoundRect(KEY_X0+152, specY, 40, KEY_H-2, 3, rgb16(60,120,80));
  canvas.drawString("^UP", KEY_X0+156, specY+8);

  // OK
  canvas.fillRoundRect(KEY_X0+198, specY, 36, KEY_H-2, 3, rgb16(20,80,30));
  canvas.drawRoundRect(KEY_X0+198, specY, 36, KEY_H-2, 3, rgb16(40,180,60));
  canvas.setTextColor(0x07E0);
  canvas.drawString("OK", KEY_X0+210, specY+8);

  // CANCEL
  canvas.fillRoundRect(KEY_X0+240, specY, 52, KEY_H-2, 3, rgb16(80,20,20));
  canvas.drawRoundRect(KEY_X0+240, specY, 52, KEY_H-2, 3, rgb16(180,40,40));
  canvas.setTextColor(0xF800);
  canvas.drawString("CANCEL", KEY_X0+242, specY+8);

  // Char count
  canvas.setTextColor(rgb16(80,80,80));
  canvas.drawString(String(current.length())+" ch", W-32, 8);
}

// Touch → key mapping
// Returns: char to add, '\b'=backspace, '\n'=OK, '\x1B'=cancel, ' '=space, '\t'=toggle case, '\0'=none
char touchToKey(int tx, int ty, bool &upperCase) {
  int specY = KEY_Y0 + N_ROWS * KEY_H + 2;

  // Special row
  if(ty >= specY && ty < specY + KEY_H) {
    if(tx >= KEY_X0    && tx < KEY_X0+90)  return ' ';
    if(tx >= KEY_X0+96 && tx < KEY_X0+146) return '\b';
    if(tx >= KEY_X0+152&& tx < KEY_X0+192) { upperCase=!upperCase; return '\t'; }
    if(tx >= KEY_X0+198&& tx < KEY_X0+234) return '\n';
    if(tx >= KEY_X0+240&& tx < W-2)        return '\x1B';
    return '\0';
  }

  // Key rows
  for(int row = 0; row < N_ROWS; row++) {
    int ky = KEY_Y0 + row * KEY_H;
    if(ty < ky || ty >= ky + KEY_H) continue;
    const char* r = ROWS[row];
    int len = strlen(r);
    int rowX = KEY_X0 + (row == 3 ? 8 : 0);
    for(int col = 0; col < len; col++) {
      int kx = rowX + col * KEY_W;
      if(tx >= kx && tx < kx + KEY_W) {
        char ch = r[col];
        if(upperCase && ch >= 'a' && ch <= 'z') ch -= 32;
        return ch;
      }
    }
  }
  return '\0';
}

// ═════════════════════════════════════════════════════════════
//  SETTINGS SCREEN (full flow)
// ═════════════════════════════════════════════════════════════

// Runs the complete settings UI. Blocks until user cancels or saves.
// Returns true if new credentials were saved (caller should reconnect).
bool runSettingsScreen(TFT_eSprite &canvas, XPT2046_Touchscreen &touch) {

  // Load current saved values to pre-populate
  WifiCreds current = loadCreds();
  String newSSID = current.ssid == DEFAULT_SSID ? "" : current.ssid;
  String newPass = "";

  bool upperCase = false;
  bool editingSSID = true;   // start on SSID field, then password

  // ── State machine ─────────────────────────────────────────
  enum SettingsState { MENU, EDIT_SSID, EDIT_PASS, CONNECTING, RESULT };
  SettingsState state = MENU;

  String resultMsg  = "";
  bool   resultOK   = false;
  unsigned long stateEntered = millis();

  while(true) {
    canvas.fillSprite(rgb16(8, 10, 18));

    switch(state) {

      // ── MENU ──────────────────────────────────────────────
      case MENU: {
        canvas.setTextColor(0xFFFF); canvas.setTextSize(2);
        canvas.drawString("Wi-Fi Settings", 30, 12);
        canvas.setTextSize(1);
        canvas.setTextColor(rgb16(120,120,120));
        canvas.drawFastHLine(0, 32, W, rgb16(50,60,80));

        // Current network info
        canvas.setTextColor(rgb16(100,200,100));
        canvas.drawString("Current network:", 10, 42);
        canvas.setTextColor(0xFFFF);
        canvas.drawString(current.fromNVS ? current.ssid : current.ssid+" (default)", 10, 54);
        canvas.setTextColor(rgb16(80,80,80));
        canvas.drawString(current.fromNVS ? "saved in device memory" : "compiled-in default", 10, 66);

        // Wi-Fi status
        canvas.setTextColor(WiFi.status()==WL_CONNECTED ? rgb16(0,200,0) : rgb16(200,80,0));
        canvas.drawString(WiFi.status()==WL_CONNECTED ?
          "Connected  " + WiFi.localIP().toString() : "Not connected", 10, 80);

        // Buttons
        // [Change Network]
        canvas.fillRoundRect(20, 108, 140, 32, 5, rgb16(20,50,80));
        canvas.drawRoundRect(20, 108, 140, 32, 5, themeColor(0.6f, 0));
        canvas.setTextColor(0xFFFF); canvas.setTextSize(1);
        canvas.drawString("Change Network", 38, 120);

        // [Reset to Default]
        canvas.fillRoundRect(170, 108, 130, 32, 5, rgb16(60,30,20));
        canvas.drawRoundRect(170, 108, 130, 32, 5, rgb16(180,80,40));
        canvas.setTextColor(rgb16(255,160,60));
        canvas.drawString("Reset to Default", 178, 120);

        // [Close]
        canvas.fillRoundRect(100, 155, 120, 30, 5, rgb16(25,40,25));
        canvas.drawRoundRect(100, 155, 120, 30, 5, rgb16(60,120,60));
        canvas.setTextColor(rgb16(80,200,80));
        canvas.drawString("Back to Viz", 120, 162);

        // Gear redraw in corner
        drawGearIcon(canvas, GEAR_X, GEAR_Y, themeColor(0.5f,0), true);

        canvas.pushSprite(0, 0);

        // Touch handling
        if(touch.tirqTouched() && touch.touched()) {
          delay(60);
          TS_Point p = touch.getPoint();
          // Map touch to screen (CYD resistive calibration, landscape)
          int tx = map(p.x, 200, 3800, 0, W);
          int ty = map(p.y, 200, 3800, 0, H);
          tx = constrain(tx, 0, W-1);
          ty = constrain(ty, 0, H-1);

          if(tx>=20  && tx<=160 && ty>=108 && ty<=140) { state=EDIT_SSID; newSSID=""; newPass=""; upperCase=false; }
          if(tx>=170 && tx<=300 && ty>=108 && ty<=140) {
            clearCreds();
            current = loadCreds();
            // Brief confirmation flash
            canvas.fillSprite(rgb16(8,10,18));
            canvas.setTextColor(rgb16(255,160,60));
            canvas.setTextSize(1);
            canvas.drawString("Reset to compiled default.", 40, 100);
            canvas.drawString("Reboot to reconnect.", 55, 115);
            canvas.pushSprite(0,0);
            delay(1500);
          }
          if(tx>=100 && tx<=220 && ty>=155 && ty<=185) return false;  // Back
          delay(200);
        }
        break;
      }

      // ── EDIT SSID ─────────────────────────────────────────
      case EDIT_SSID: {
        drawKeyboard(canvas, "SSID", newSSID, false, -1, -1);
        canvas.pushSprite(0, 0);

        if(touch.tirqTouched() && touch.touched()) {
          delay(60);
          TS_Point p = touch.getPoint();
          int tx = map(p.x, 200, 3800, 0, W);
          int ty = map(p.y, 200, 3800, 0, H);
          tx = constrain(tx, 0, W-1);
          ty = constrain(ty, 0, H-1);

          char k = touchToKey(tx, ty, upperCase);
          if(k == '\n') {
            if(newSSID.length() > 0) { state = EDIT_PASS; upperCase = false; }
          } else if(k == '\x1B') {
            state = MENU;
          } else if(k == '\b') {
            if(newSSID.length() > 0) newSSID.remove(newSSID.length()-1);
          } else if(k == '\t') {
            // case toggled in touchToKey
          } else if(k != '\0' && newSSID.length() < 32) {
            newSSID += k;
          }
          delay(120);
        }
        break;
      }

      // ── EDIT PASSWORD ─────────────────────────────────────
      case EDIT_PASS: {
        drawKeyboard(canvas, "PASSWORD", newPass, true, -1, -1);
        canvas.pushSprite(0, 0);

        if(touch.tirqTouched() && touch.touched()) {
          delay(60);
          TS_Point p = touch.getPoint();
          int tx = map(p.x, 200, 3800, 0, W);
          int ty = map(p.y, 200, 3800, 0, H);
          tx = constrain(tx, 0, W-1);
          ty = constrain(ty, 0, H-1);

          char k = touchToKey(tx, ty, upperCase);
          if(k == '\n') {
            // Save and try connecting
            saveCreds(newSSID, newPass);
            state = CONNECTING;
            stateEntered = millis();
            // Kick off connection
            WiFi.disconnect(true);
            delay(200);
            WiFi.begin(newSSID.c_str(), newPass.c_str());
          } else if(k == '\x1B') {
            state = EDIT_SSID;  // back to SSID
          } else if(k == '\b') {
            if(newPass.length() > 0) newPass.remove(newPass.length()-1);
          } else if(k == '\t') {
            // case toggled
          } else if(k != '\0' && newPass.length() < 63) {
            newPass += k;
          }
          delay(120);
        }
        break;
      }

      // ── CONNECTING ────────────────────────────────────────
      case CONNECTING: {
        canvas.setTextColor(themeColor(0.7f, 0));
        canvas.setTextSize(1);
        canvas.drawString("Connecting to:", 10, 40);
        canvas.setTextColor(0xFFFF); canvas.setTextSize(2);
        canvas.drawString(newSSID, 10, 56);
        canvas.setTextSize(1);

        // Animated dots
        unsigned long el = millis() - stateEntered;
        int dots = (el / 400) % 4;
        String dotStr = "";
        for(int d=0;d<dots;d++) dotStr += ".";
        canvas.setTextColor(themeColor(0.5f, 0));
        canvas.drawString("Connecting" + dotStr, 10, 90);

        // Signal strength bars (placeholder animation)
        for(int b=0;b<4;b++) {
          int bh = (b+1)*6;
          bool lit = (int)(el/200) % 4 > b;
          uint16_t bc = lit ? themeColor((float)b/3, 0) : rgb16(40,40,40);
          canvas.fillRect(200+b*14, 100-bh, 10, bh, bc);
        }

        // Timeout after 12 seconds
        if(el > 12000) {
          WiFi.disconnect(true);
          resultOK  = false;
          resultMsg = "Connection timed out.\nCheck SSID and password.";
          state     = RESULT;
        }

        // Check connection
        if(WiFi.status() == WL_CONNECTED) {
          resultOK  = true;
          resultMsg = "Connected!\n" + WiFi.localIP().toString();
          state     = RESULT;
          stateEntered = millis();
          // Green LED flash
          for(int i=0;i<3;i++){
            digitalWrite(LED_G,LOW); delay(100); digitalWrite(LED_G,HIGH); delay(100);
          }
        } else if(WiFi.status() == WL_CONNECT_FAILED) {
          resultOK  = false;
          resultMsg = "Wrong password or\nnetwork not found.";
          state     = RESULT;
          stateEntered = millis();
        }

        canvas.pushSprite(0, 0);
        break;
      }

      // ── RESULT ────────────────────────────────────────────
      case RESULT: {
        canvas.setTextSize(2);
        if(resultOK) {
          canvas.setTextColor(rgb16(0,255,0));
          canvas.drawString("Connected!", 60, 50);
          canvas.setTextSize(1);
          canvas.setTextColor(0xFFFF);
          canvas.drawString(WiFi.localIP().toString(), 90, 80);
          canvas.setTextColor(rgb16(100,200,100));
          canvas.drawString("Spotify sync active.", 70, 100);

          // Draw little starfield reward
          for(int s=0;s<40;s++) {
            float a = s * 9.0f * DEG_TO_RAD;
            int r2  = 20 + (s % 5) * 10;
            canvas.fillCircle(CX+(int)(cos(a)*r2), 140+(int)(sin(a)*r2*0.4f),
              1+(s%3), themeColor((float)s/40, 0));
          }
        } else {
          canvas.setTextColor(rgb16(255,80,40));
          canvas.drawString("Failed.", 100, 50);
          canvas.setTextSize(1);
          canvas.setTextColor(rgb16(200,150,100));
          // Split message at \n
          int nl = resultMsg.indexOf('\n');
          if(nl > 0) {
            canvas.drawString(resultMsg.substring(0,nl), 20, 80);
            canvas.drawString(resultMsg.substring(nl+1), 20, 94);
          } else {
            canvas.drawString(resultMsg, 20, 80);
          }
          canvas.setTextColor(rgb16(100,100,100));
          canvas.drawString("Credentials saved. Try again?", 20, 115);
        }

        // Buttons
        canvas.fillRoundRect(20, 160, 120, 30, 5, rgb16(20,50,20));
        canvas.drawRoundRect(20, 160, 120, 30, 5, rgb16(60,120,60));
        canvas.setTextColor(rgb16(80,200,80)); canvas.setTextSize(1);
        canvas.drawString("Back to Menu", 32, 172);

        canvas.fillRoundRect(170, 160, 130, 30, 5, rgb16(15,20,50));
        canvas.drawRoundRect(170, 160, 130, 30, 5, themeColor(0.6f,0));
        canvas.setTextColor(0xFFFF);
        canvas.drawString("Return to Viz", 185, 172);

        canvas.pushSprite(0, 0);

        // Touch or auto-exit after 4 seconds on success
        bool autoExit = resultOK && (millis()-stateEntered > 4000);
        if(autoExit) return resultOK;

        if(touch.tirqTouched() && touch.touched()) {
          delay(80);
          TS_Point p = touch.getPoint();
          int tx = map(p.x, 200, 3800, 0, W);
          int ty = map(p.y, 200, 3800, 0, H);
          tx = constrain(tx, 0, W-1);
          ty = constrain(ty, 0, H-1);
          if(tx>=20  && tx<=140 && ty>=160 && ty<=190) { state=MENU; delay(200); }
          if(tx>=170 && tx<=300 && ty>=160 && ty<=190) return resultOK;
          delay(150);
        }
        break;
      }
    }

    delay(16);
  }
}
