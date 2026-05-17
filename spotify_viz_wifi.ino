/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║   SPOTIFY LIVE VISUALIZER  —  ESP32-2432S028R (CYD)         ║
 * ║   Wi-Fi · OAuth · Beat-Synced · Settings · Gear Icon        ║
 * ║   Ellis Edition :: The Far Edge                             ║
 * ╚══════════════════════════════════════════════════════════════╝
 *
 *  Touch the ⚙ cogwheel (bottom-right HUD) to open Wi-Fi settings.
 *  Default credentials compile in; saved credentials persist in NVS.
 *
 *  Files in this sketch folder:
 *    spotify_viz_wifi.ino   <- this file (main)
 *    settings.h             <- Wi-Fi settings UI + NVS storage
 *
 *  Setup:
 *    1. Fill DEFAULT_SSID / DEFAULT_PASS in settings.h
 *    2. Fill CLIENT_ID, CLIENT_SECRET, REFRESH_TOKEN below
 *    3. Run get_spotify_token.py on PC once to get REFRESH_TOKEN
 *    4. Flash to CYD — tap gear to change Wi-Fi any time
 *
 *  Libraries: TFT_eSPI · XPT2046_Touchscreen · ArduinoJson
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <Preferences.h>
#include <math.h>
#include "spotify_certs.h"

// ═══════════════════════════════════════════════════════════════
//  SPOTIFY CREDENTIALS
// ═══════════════════════════════════════════════════════════════
#define CLIENT_ID      "YOUR_SPOTIFY_CLIENT_ID"
#define CLIENT_SECRET  "YOUR_SPOTIFY_CLIENT_SECRET"
#define REFRESH_TOKEN  "YOUR_REFRESH_TOKEN"

// ═══════════════════════════════════════════════════════════════
//  HARDWARE
// ═══════════════════════════════════════════════════════════════
#define TOUCH_CS  33
#define TOUCH_IRQ 36
#define LED_R     17
#define LED_G     16
#define LED_B      4
#define W  320
#define H  240
#define CX (W/2)
#define CY (H/2)

TFT_eSPI             tft    = TFT_eSPI();
TFT_eSprite          canvas = TFT_eSprite(&tft);
XPT2046_Touchscreen  touch(TOUCH_CS, TOUCH_IRQ);

// ── Colour helpers — must be declared before settings.h ───────
uint16_t rgb16(uint8_t r, uint8_t g, uint8_t b) {
  return ((r>>3)<<11)|((g>>2)<<5)|(b>>3);
}
uint16_t hsv16(float h, float s, float v) {
  h=fmod(h,360.0f); if(h<0)h+=360;
  float c=v*s,x=c*(1-fabs(fmod(h/60.0f,2)-1)),m=v-c;
  float r=0,g=0,b=0;
  if(h<60){r=c;g=x;}else if(h<120){r=x;g=c;}
  else if(h<180){g=c;b=x;}else if(h<240){g=x;b=c;}
  else if(h<300){r=x;b=c;}else{r=c;b=x;}
  return ((uint16_t)((r+m)*31)<<11)|((uint16_t)((g+m)*63)<<5)|(uint16_t)((b+m)*31);
}

// Forward declarations for colour system (defined later)
float getBaseHue();
float getHueSweep();
uint16_t themeColor(float t, float bb);

// ── Include settings module ────────────────────────────────────
#include "settings.h"
#include "control_panel.h"

// ═══════════════════════════════════════════════════════════════
//  SPOTIFY STATE
// ═══════════════════════════════════════════════════════════════
struct SpotifyTrack {
  String trackId, trackName, artistName;
  bool   isPlaying;
  long   progressMs, durationMs;
  float  tempo, energy, danceability, valence, loudness;
  int    key, musicalMode;
  bool   featuresLoaded;
};
SpotifyTrack  track;
String        accessToken = "";
unsigned long tokenExpiry = 0;
unsigned long lastPoll    = 0;
const unsigned long POLL_MS = 3000;
unsigned long lastSpotifyError = 0;
bool apiConnected = false;

// ═══════════════════════════════════════════════════════════════
//  BEAT ENGINE
// ═══════════════════════════════════════════════════════════════
float beatPhase     = 0.0f;
float beatIntensity = 0.0f;
long  localProgressMs    = 0;
unsigned long progressTimestamp = 0;

void ledBeat();   // forward
void ledNewTrack();

void updateBeatEngine() {
  if(track.isPlaying)
    localProgressMs = track.progressMs + (long)(millis() - progressTimestamp);
  if(track.tempo < 20.0f) return;
  float beatMs   = 60000.0f / track.tempo;
  float rawPhase = fmod((float)localProgressMs, beatMs) / beatMs;
  bool  onBeat   = rawPhase < beatPhase;
  beatPhase = rawPhase;
  if(onBeat) { beatIntensity = track.energy*0.8f+0.2f; ledBeat(); }
  else        beatIntensity *= 0.88f;
}

// ═══════════════════════════════════════════════════════════════
//  COLOUR SYSTEM
// ═══════════════════════════════════════════════════════════════
const float KEY_HUE[12] = {0,30,60,90,120,150,180,210,240,270,300,330};
float getBaseHue()  { return (track.key>=0&&track.key<12)?KEY_HUE[track.key]:180.0f; }
float getHueSweep() { return track.valence * 120.0f; }
uint16_t themeColor(float t, float bb) {
  float h=fmod(getBaseHue()+t*getHueSweep(),360.0f);
  float s=(track.musicalMode==0)?0.9f:0.7f+t*0.3f;
  float v=min(0.3f+t*0.7f+bb,1.0f);
  return hsv16(h,s,v);
}

// ═══════════════════════════════════════════════════════════════
//  SPECTRUM + WAVEFORM SYNTHESIS
// ═══════════════════════════════════════════════════════════════
#define DISPLAY_BARS 40
float spectrum[DISPLAY_BARS], specPeak[DISPLAY_BARS];
int   specPeakTimer[DISPLAY_BARS];
float waveformBuf[W];

void synthesizeSpectrum() {
  float t=millis()*0.001f,bph=beatPhase,bi=beatIntensity;
  float en=max(0.05f,track.energy),dan=max(0.05f,track.danceability);
  float bpm=max(60.0f,track.tempo);
  for(int b=0;b<DISPLAY_BARS;b++){
    float n=(float)b/(DISPLAY_BARS-1);
    float bass=0,mid=0,treble=0;
    if(n<0.2f) bass=bi*en*(1-n*5)+sin(t*bpm/30+n*3.14f)*en*0.3f;
    if(n>=0.1f&&n<0.7f) mid=max(0.0f,sin(t*2.3f+n*8)*dan*0.4f+sin(t*1.1f+n*4)*en*0.2f);
    if(n>=0.5f) treble=max(0.0f,sin(t*5.7f+n*12+bph*6.28f)*track.valence*0.3f);
    float raw=min(max(bass+mid+treble,0.0f),1.0f);
    spectrum[b]=spectrum[b]*0.65f+raw*0.35f;
    if(spectrum[b]>specPeak[b]){specPeak[b]=spectrum[b];specPeakTimer[b]=40;}
    else if(specPeakTimer[b]>0){if(--specPeakTimer[b]==0)specPeak[b]=spectrum[b];}
    else specPeak[b]=max(specPeak[b]*0.97f,spectrum[b]);
  }
}

void synthesizeWaveform() {
  float t=millis()*0.001f;
  for(int x=0;x<W;x++){
    float xn=(float)x/W,w=0;
    for(int b=0;b<DISPLAY_BARS;b+=3)
      w+=sin(xn*(1+b*0.4f)*6.28f*4+t*(1+b*0.1f))*spectrum[b]*0.3f;
    w+=sin(xn*6.28f*2+t*6.28f*(track.tempo/60.0f))*beatIntensity*0.3f;
    waveformBuf[x]=max(-1.0f,min(1.0f,w));
  }
}

// ═══════════════════════════════════════════════════════════════
//  VISUALISATION STATE
// ═══════════════════════════════════════════════════════════════
#define NUM_MODES 7
int   vizMode=0;
float simPhase=0.0f;

#define N_PARTICLES 100
struct Particle{float x,y,vx,vy,life;uint16_t col;};
Particle particles[N_PARTICLES]; int particleIdx=0;

#define N_STARS 80
struct Star{float x,y,z;};
Star stars[N_STARS];
void initStar(int i,bool rndZ);

#define MATRIX_COLS 32
int matrixY[MATRIX_COLS]; uint8_t matrixSpeed[MATRIX_COLS];

float waterfall[H][DISPLAY_BARS]; int waterfallRow=0;
String scrollText=""; float scrollX=W;

// GEAR ICON — hit test
bool gearTapped(int tx, int ty) {
  int dx=tx-GEAR_X, dy=ty-GEAR_Y;
  return (dx*dx+dy*dy) <= (GEAR_R+8)*(GEAR_R+8);
}

// MENU ICON — hit test
bool menuTapped(int tx, int ty) {
  int dx=tx-MENU_X, dy=ty-MENU_Y;
  return (dx*dx+dy*dy) <= (MENU_R+8)*(MENU_R+8);
}

// ═══════════════════════════════════════════════════════════════
//  SETUP
// ═══════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  pinMode(LED_R,OUTPUT);pinMode(LED_G,OUTPUT);pinMode(LED_B,OUTPUT);
  digitalWrite(LED_R,HIGH);digitalWrite(LED_G,HIGH);digitalWrite(LED_B,HIGH);
  pinMode(0,INPUT_PULLUP);

  tft.init(); tft.setRotation(1); tft.fillScreen(0x0000);
  ledcSetup(0,5000,8); ledcAttachPin(21,0); ledcWrite(0,210);
  canvas.createSprite(W,H); canvas.setColorDepth(16);
  touch.begin(); touch.setRotation(1);

  for(int i=0;i<N_STARS;i++) initStar(i,true);
  for(int i=0;i<MATRIX_COLS;i++){matrixY[i]=random(H);matrixSpeed[i]=random(1,4);}

  // Safe defaults
  track.tempo=120;track.energy=0.5f;track.danceability=0.5f;
  track.valence=0.5f;track.loudness=-10;track.key=0;track.musicalMode=1;
  track.isPlaying=false;track.featuresLoaded=false;

  // Connect using saved or default credentials
  WifiCreds creds=loadCreds();
  connectWithCreds(creds);
  if(WiFi.status()==WL_CONNECTED){refreshAccessToken();pollCurrentlyPlaying();}

  splashScreen();
  delay(1200);
}

// ═══════════════════════════════════════════════════════════════
//  MAIN LOOP
// ═══════════════════════════════════════════════════════════════
void loop() {
  unsigned long now=millis();

  // BOOT button cycles viz mode
  static bool btnLast=HIGH; static unsigned long btnT=0;
  bool btnNow=digitalRead(0);
  if(btnNow==LOW&&btnLast==HIGH&&now-btnT>300){btnT=now;nextMode();}
  btnLast=btnNow;

  // Touch
  if(touch.tirqTouched()&&touch.touched()){
    static unsigned long lastT=0;
    if(now-lastT>150){
      lastT=now;
      TS_Point p=touch.getPoint();
      int tx=constrain(map(p.x,200,3800,0,W),0,W-1);
      int ty=constrain(map(p.y,200,3800,0,H),0,H-1);
      if(menuTapped(tx,ty)){
        // Open control panel
        runControlPanel();
        for(int i=0;i<N_STARS;i++) initStar(i,true);
        lastPoll=0;
      } else if(gearTapped(tx,ty)){
        // Open settings — blocks until user exits
        bool changed=runSettingsScreen(canvas,touch);
        if(changed&&WiFi.status()==WL_CONNECTED){
          refreshAccessToken();
          pollCurrentlyPlaying();
        }
        for(int i=0;i<N_STARS;i++) initStar(i,true);
        lastPoll=0;
      } else {
        nextMode();
      }
    }
  }

  // Spotify polling
  if(now-lastPoll>POLL_MS){
    lastPoll=now;
    if(WiFi.status()==WL_CONNECTED){
      if(now>=tokenExpiry) refreshAccessToken();
      pollCurrentlyPlaying();
    }
  }

  updateBeatEngine();
  synthesizeSpectrum();
  synthesizeWaveform();

  canvas.fillSprite(0x0000);
  switch(vizMode){
    case 0:drawWaveform();    break;
    case 1:drawBarChart();    break;
    case 2:drawScatter();     break;
    case 3:drawRadialBurst(); break;
    case 4:drawMatrixRain();  break;
    case 5:drawStarfield();   break;
    case 6:drawWaterfall();   break;
  }
  drawHUD();
  canvas.pushSprite(0,0);
  simPhase+=0.04f;
}

// ═══════════════════════════════════════════════════════════════
//  WI-FI CONNECTION
// ═══════════════════════════════════════════════════════════════
void connectWithCreds(const WifiCreds &creds) {
  tft.fillScreen(0x0000);
  tft.setTextSize(1); tft.setTextColor(0xFFFF);
  tft.drawString("Connecting: "+creds.ssid, 8, 95);
  tft.setTextColor(rgb16(70,70,70));
  tft.drawString(creds.fromNVS?"(saved network)":"(home default)", 8, 110);

  WiFi.disconnect(true); delay(100);
  WiFi.begin(creds.ssid.c_str(), creds.pass.c_str());

  unsigned long t0=millis(); int dots=0;
  while(WiFi.status()!=WL_CONNECTED&&millis()-t0<12000){
    delay(350);
    tft.drawPixel(8+dots*4,125,hsv16(dots*12.0f,1.0f,0.9f));
    dots++;
  }

  if(WiFi.status()==WL_CONNECTED){
    tft.setTextColor(rgb16(0,220,0));
    tft.drawString("Connected: "+WiFi.localIP().toString(),8,140);
    for(int i=0;i<2;i++){digitalWrite(LED_G,LOW);delay(150);digitalWrite(LED_G,HIGH);delay(150);}
  } else {
    tft.setTextColor(rgb16(220,80,0));
    tft.drawString("Wi-Fi failed. Tap gear icon to reconfigure.",4,140);
    delay(2200);
  }
  delay(400);
}

// ═══════════════════════════════════════════════════════════════
//  VISUALISATIONS
// ═══════════════════════════════════════════════════════════════
void drawWaveform() {
  int midY=(H-30)/2+8;
  canvas.drawFastHLine(0,midY,W,rgb16(0,20,30));
  for(int x=1;x<W;x++){
    float a=waveformBuf[x],ap=waveformBuf[x-1];
    int y0=midY-(int)(ap*(midY-12)),y1=midY-(int)(a*(midY-12));
    float e=fabs(a);
    canvas.drawLine(x-1,y0,x,y1,themeColor(e,beatIntensity*0.1f));
    canvas.drawLine(x-1,y0+1,x,y1+1,hsv16(getBaseHue(),0.4f,e*0.3f));
    canvas.drawLine(x-1,y0-1,x,y1-1,hsv16(getBaseHue(),0.4f,e*0.3f));
  }
  if(beatIntensity>0.6f) canvas.drawFastHLine(0,midY,W,themeColor(beatIntensity,0));
}

void drawBarChart() {
  int baseY=H-28,bw=W/DISPLAY_BARS;
  for(int b=0;b<DISPLAY_BARS;b++){
    int bh=(int)(spectrum[b]*(baseY-6)),x=b*bw;
    canvas.fillRect(x+1,baseY-bh,bw-2,bh,themeColor((float)b/DISPLAY_BARS,spectrum[b]*beatIntensity*0.2f));
    canvas.drawFastHLine(x+1,baseY-(int)(specPeak[b]*(baseY-6))-1,bw-2,0xFFFF);
    int rh=bh/4;
    for(int dy=0;dy<rh;dy++) if(dy%2==0)
      canvas.drawFastHLine(x+1,baseY+1+dy,bw-2,hsv16(getBaseHue(),0.6f,spectrum[b]*(float)(rh-dy)/rh*0.4f));
  }
  if(beatIntensity>0.7f) canvas.drawFastHLine(0,H-28,W,themeColor(1.0f,0));
}

void drawScatter() {
  if(beatIntensity>0.5f&&random(3)==0){
    for(int n=0;n<(int)(beatIntensity*3)+1;n++){
      Particle &p=particles[particleIdx%N_PARTICLES];
      float a=random(360)*DEG_TO_RAD,sp=track.danceability*4+beatIntensity*2;
      p.x=CX;p.y=CY-15;p.vx=cos(a)*sp;p.vy=sin(a)*sp;
      p.life=1.0f;p.col=themeColor(track.valence,0.2f);particleIdx++;
    }
  }
  for(int i=0;i<N_PARTICLES;i++){
    if(particles[i].life<=0) continue;
    particles[i].x+=particles[i].vx;particles[i].y+=particles[i].vy;
    particles[i].vx*=0.97f;particles[i].vy*=0.97f;particles[i].life-=0.012f;
    int px=(int)particles[i].x,py=(int)particles[i].y;
    if(px>0&&px<W&&py>0&&py<H-24){
      canvas.drawPixel(px,py,particles[i].col);
      if(particles[i].life>0.5f) canvas.drawPixel(px+1,py,particles[i].col);
    }
  }
  float px2=-999,py2=-999;
  for(int i=0;i<W-1;i+=2){
    float s1=waveformBuf[i],s2=waveformBuf[(i+W/2)%W];
    float ppx=CX+s1*55,ppy=(CY-15)+s2*55;
    if(px2>-999) canvas.drawLine((int)px2,(int)py2,(int)ppx,(int)ppy,themeColor(fabs(s1)+fabs(s2),0));
    px2=ppx;py2=ppy;
  }
  canvas.drawFastHLine(CX-20,CY-15,41,rgb16(40,40,40));
  canvas.drawFastVLine(CX,CY-35,41,rgb16(40,40,40));
}

void drawRadialBurst() {
  int innerR=35+(int)(beatIntensity*8);
  float rot=simPhase*0.3f;
  for(int b=0;b<DISPLAY_BARS;b++){
    float angle=rot+(float)b/DISPLAY_BARS*TWO_PI-HALF_PI;
    float r=innerR+spectrum[b]*95,rp=innerR+specPeak[b]*95;
    int x0=CX+(int)(cos(angle)*innerR),y0=(CY-12)+(int)(sin(angle)*innerR);
    int x1=CX+(int)(cos(angle)*r),    y1=(CY-12)+(int)(sin(angle)*r);
    uint16_t col=themeColor((float)b/DISPLAY_BARS,beatIntensity*0.1f);
    canvas.drawLine(x0,y0,x1,y1,col);canvas.drawLine(x0+1,y0,x1+1,y1,col);
    canvas.drawPixel(CX+(int)(cos(angle)*rp),(CY-12)+(int)(sin(angle)*rp),0xFFFF);
  }
  canvas.drawCircle(CX,CY-12,(int)(innerR+beatIntensity*20),themeColor(beatIntensity,0));
}

void drawMatrixRain() {
  float h=getBaseHue();
  for(int y=0;y<H-24;y+=2) canvas.drawFastHLine(0,y,W,hsv16(h,0.8f,0.04f));
  int colW=W/MATRIX_COLS;
  for(int c=0;c<MATRIX_COLS;c++){
    float ce=spectrum[(c*DISPLAY_BARS)/MATRIX_COLS];
    matrixY[c]+=matrixSpeed[c]+(int)(ce*4)+(beatIntensity>0.7f?2:0);
    if(matrixY[c]>H+20){matrixY[c]=-20;matrixSpeed[c]=random(1,4);}
    int x=c*colW;
    canvas.fillRect(x,matrixY[c],colW-1,8,themeColor(ce,0.2f));
    for(int t2=1;t2<7;t2++){
      int ty=matrixY[c]-t2*8; if(ty<0) continue;
      float fade=1.0f-(float)t2/7;
      canvas.fillRect(x,ty,colW-1,8,hsv16(h,0.7f+ce*0.3f,fade*ce*0.8f+0.05f));
    }
    canvas.drawPixel(x+random(colW),matrixY[c]+random(8),0xFFFF);
  }
}

void initStar(int i,bool rndZ){
  stars[i].x=(random(W)-CX)*1.0f;
  stars[i].y=(random(H-24)-(CY-12))*1.0f;
  stars[i].z=rndZ?random(100)+1:100.0f;
}
void drawStarfield() {
  float bass=(spectrum[0]+spectrum[1]+spectrum[2]+spectrum[3])/4;
  float speed=0.4f+bass*6+beatIntensity*4;
  for(int i=0;i<N_STARS;i++){
    stars[i].z-=speed; if(stars[i].z<=0) initStar(i,false);
    float invZ=100.0f/stars[i].z;
    int sx=CX+(int)(stars[i].x*invZ),sy=(CY-12)+(int)(stars[i].y*invZ);
    if(sx<0||sx>=W||sy<0||sy>=H-24){initStar(i,false);continue;}
    float br=1.0f-stars[i].z/100.0f;
    uint16_t col=beatIntensity>0.4f?themeColor(br,0):rgb16(br*255,br*255,br*255);
    if(br>0.8f) canvas.fillRect(sx-1,sy-1,3,3,col);
    else canvas.drawPixel(sx,sy,col);
  }
  if(beatIntensity>0.6f) canvas.drawCircle(CX,CY-12,(int)((1-beatIntensity)*80),themeColor(beatIntensity,0));
}

void drawWaterfall() {
  int bw=W/DISPLAY_BARS,dispH=H-24;
  for(int b=0;b<DISPLAY_BARS;b++) waterfall[waterfallRow][b]=spectrum[b];
  waterfallRow=(waterfallRow+1)%dispH;
  float h=getBaseHue();
  for(int row=0;row<dispH;row++){
    int sr=(waterfallRow-dispH+row+dispH)%dispH;
    for(int b=0;b<DISPLAY_BARS;b++){
      float val=waterfall[sr][b];
      uint16_t col;
      if(val<0.25f)      col=hsv16(h+240,1.0f,val*4*0.8f);
      else if(val<0.5f)  col=hsv16(h+120,1.0f,0.4f+(val-0.25f)*4*0.4f);
      else if(val<0.75f) col=hsv16(h+60,1.0f,0.7f+(val-0.5f)*4*0.3f);
      else               col=themeColor(val,0.1f);
      canvas.fillRect(b*bw,row,bw,1,col);
    }
  }
  for(int b=0;b<DISPLAY_BARS;b++)
    canvas.fillRect(b*bw,dispH,bw,4,themeColor(spectrum[b],beatIntensity*0.2f));
}

// ═══════════════════════════════════════════════════════════════
//  HUD
// ═══════════════════════════════════════════════════════════════
const char* modeNames[]={"WAVE","SPEC","SCAT","RADL","MTRX","STAR","FALL"};

void drawHUD() {
  int hudY=H-22;
  canvas.fillRect(0,hudY,W,22,rgb16(10,10,10));
  canvas.drawFastHLine(0,hudY,W,themeColor(0.5f,0));

  // Menu button (hamburger icon, bottom-left)
  drawMenuIcon(canvas, MENU_X, MENU_Y, rgb16(130,130,130));

  // Track scroll
  canvas.setTextColor(0xFFFF); canvas.setTextSize(1);
  if(scrollText.length()>0){
    scrollX-=1.2f+track.tempo/300.0f;
    if(scrollX<-(int)(scrollText.length()*6)) scrollX=W;
    canvas.drawString(scrollText,(int)scrollX,hudY+7);
  } else {
    canvas.setTextColor(rgb16(70,70,70));
    canvas.drawString(WiFi.status()==WL_CONNECTED?"No track playing — start Spotify":"No Wi-Fi — tap gear",10,hudY+7);
  }

  // Mode dots (shifted left to leave room for gear)
  for(int i=0;i<NUM_MODES;i++){
    int x=W-NUM_MODES*9-22+i*9;
    if(i==vizMode) canvas.fillCircle(x,hudY+4,3,themeColor(1.0f,0));
    else           canvas.drawCircle(x,hudY+4,2,rgb16(60,60,60));
  }

  // Beat pulse dot (top-left)
  if(beatIntensity>0.5f) canvas.fillCircle(5,5,4,themeColor(beatIntensity,0));
  else                   canvas.drawCircle(5,5,4,rgb16(40,40,40));

  // Wi-Fi signal bars (top-right, left of gear)
  if(WiFi.status()==WL_CONNECTED){
    int rssi=WiFi.RSSI();
    int bars=(rssi>-50)?4:(rssi>-65)?3:(rssi>-75)?2:1;
    for(int b=0;b<4;b++){
      uint16_t bc=b<bars?rgb16(0,180,0):rgb16(40,40,40);
      canvas.fillRect(W-44+b*5, 12-(b+1)*2, 4, (b+1)*2+1, bc);
    }
    // Spotify API status indicator (red=error, green=ok, yellow=no track)
    uint16_t spotifyLed=rgb16(40,40,40);
    if(millis()-lastSpotifyError<5000) spotifyLed=rgb16(200,60,0);  // red - recent error
    else if(apiConnected&&track.isPlaying) spotifyLed=rgb16(0,200,0);  // green - playing
    else if(apiConnected) spotifyLed=rgb16(200,180,0);  // yellow - connected but not playing
    canvas.fillCircle(W-50, 7, 2, spotifyLed);
  } else {
    canvas.setTextColor(rgb16(200,60,0)); canvas.setTextSize(1);
    canvas.drawString("!", W-44, 4);
  }

  // Progress bar (bottom pixel row)
  if(track.durationMs>0){
    int pw=min((int)((float)localProgressMs/track.durationMs*W),W);
    canvas.fillRect(0,H-2,pw,2,themeColor(0.7f,0));
    canvas.fillRect(pw,H-2,W-pw,2,rgb16(25,25,25));
  }

  // GEAR ICON — always visible, bottom-right corner of HUD
  drawGearIcon(canvas, GEAR_X, GEAR_Y, rgb16(130,130,130), false);
}

// ═══════════════════════════════════════════════════════════════
//  SPOTIFY API
// ═══════════════════════════════════════════════════════════════
void refreshAccessToken() {
  if(WiFi.status()!=WL_CONNECTED) return;
  WiFiClientSecure client;
  client.setCACert(SPOTIFY_ROOT_CA);
  HTTPClient http;
  http.begin(client,"https://accounts.spotify.com/api/token");
  http.addHeader("Content-Type","application/x-www-form-urlencoded");
  String b64=base64Encode(String(CLIENT_ID)+":"+String(CLIENT_SECRET));
  http.addHeader("Authorization","Basic "+b64);
  String body="grant_type=refresh_token&refresh_token="; body+=REFRESH_TOKEN;
  int code=http.POST(body);
  if(code==200){
    DynamicJsonDocument doc(512);
    if(deserializeJson(doc,http.getStream())==DeserializationError::Ok){
      accessToken=doc["access_token"].as<String>();
      tokenExpiry=millis()+(int(doc["expires_in"]|3600)-60)*1000UL;
      apiConnected=true;
      lastSpotifyError=0;
    }
  } else {
    Serial.printf("Token refresh failed: HTTP %d\n",code);
    lastSpotifyError=millis();
    apiConnected=false;
  }
  http.end();
}

void pollCurrentlyPlaying() {
  if(WiFi.status()!=WL_CONNECTED||accessToken.isEmpty()) return;
  WiFiClientSecure client;
  client.setCACert(SPOTIFY_ROOT_CA);
  HTTPClient http;
  http.begin(client,"https://api.spotify.com/v1/me/player/currently-playing");
  http.addHeader("Authorization","Bearer "+accessToken);
  http.addHeader("Accept","application/json");
  int code=http.GET();
  if(code==200){
    DynamicJsonDocument doc(4096);
    if(deserializeJson(doc,http.getStream())==DeserializationError::Ok){
      track.isPlaying=doc["is_playing"]|false;
      auto item=doc["item"];
      if(!item.isNull()){
        String newId=item["id"]|"";
        track.trackName=item["name"]|"Unknown";
        track.artistName=item["artists"][0]["name"]|"Unknown";
        track.durationMs=item["duration_ms"]|0;
        track.progressMs=doc["progress_ms"]|0;
        progressTimestamp=millis();
        scrollText="  *  "+track.trackName+"  -  "+track.artistName+"   ";
        if(newId!=track.trackId){track.trackId=newId;loadAudioFeatures(newId);}
        apiConnected=true;
        lastSpotifyError=0;
      }
    }
  } else if(code==204) {
    track.isPlaying=false;
    apiConnected=true;
    lastSpotifyError=0;
  } else if(code==401) {
    Serial.println("Invalid token - will refresh");
    tokenExpiry=0;
  } else {
    Serial.printf("Poll failed: HTTP %d\n",code);
    lastSpotifyError=millis();
    apiConnected=false;
  }
  http.end();
}

void loadAudioFeatures(String id) {
  if(WiFi.status()!=WL_CONNECTED||accessToken.isEmpty()) return;
  WiFiClientSecure client;
  client.setCACert(SPOTIFY_ROOT_CA);
  HTTPClient http;
  http.begin(client,"https://api.spotify.com/v1/audio-features/"+id);
  http.addHeader("Authorization","Bearer "+accessToken);
  int code=http.GET();
  if(code==200){
    DynamicJsonDocument doc(1024);
    if(deserializeJson(doc,http.getStream())==DeserializationError::Ok){
      track.tempo=doc["tempo"]|120.0f;
      track.energy=doc["energy"]|0.5f;
      track.danceability=doc["danceability"]|0.5f;
      track.valence=doc["valence"]|0.5f;
      track.loudness=doc["loudness"]|-10.0f;
      track.key=doc["key"]|0;
      track.musicalMode=doc["mode"]|1;
      track.featuresLoaded=true;
      ledNewTrack();
    }
  } else {
    Serial.printf("Audio features failed: HTTP %d\n",code);
  }
  http.end();
}

// ═══════════════════════════════════════════════════════════════
//  HELPERS
// ═══════════════════════════════════════════════════════════════
void nextMode(){vizMode=(vizMode+1)%NUM_MODES;digitalWrite(LED_B,LOW);delay(25);digitalWrite(LED_B,HIGH);}

void ledBeat(){
  float h=getBaseHue();
  digitalWrite(LED_R,h>60&&h<300?HIGH:LOW);
  digitalWrite(LED_G,h>120&&h<240?LOW:HIGH);
  digitalWrite(LED_B,h>200?LOW:HIGH);
  delay(4);
  digitalWrite(LED_R,HIGH);digitalWrite(LED_G,HIGH);digitalWrite(LED_B,HIGH);
}

void ledNewTrack(){
  for(int i=0;i<3;i++){digitalWrite(LED_G,LOW);delay(80);digitalWrite(LED_G,HIGH);delay(80);}
}

void splashScreen(){
  tft.fillScreen(0x0000);
  tft.setTextSize(2);tft.setTextColor(themeColor(0.7f,0));
  tft.drawString("SPOTIFY VIZ",60,65);
  tft.setTextSize(1);tft.setTextColor(rgb16(100,100,100));
  tft.drawString("Ellis Edition :: The Far Edge",34,96);
  tft.setTextColor(rgb16(60,80,60));
  tft.drawString("Tap anywhere to cycle modes",64,140);
  tft.setTextColor(rgb16(100,100,80));
  tft.drawString("Tap gear icon for Wi-Fi settings",52,155);
  for(int i=0;i<NUM_MODES;i++)
    tft.drawCircle(CX-NUM_MODES*10+i*20,195,5,hsv16(i*51.4f,1.0f,0.8f));
}

// base64
const char B64C[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
String base64Encode(const String &in){
  String out;int i=0;uint8_t b3[3],b4[4];
  const char*s=in.c_str();int len=in.length();
  while(len--){b3[i++]=*(s++);if(i==3){b4[0]=(b3[0]&0xfc)>>2;b4[1]=((b3[0]&3)<<4)+((b3[1]&0xf0)>>4);b4[2]=((b3[1]&0xf)<<2)+((b3[2]&0xc0)>>6);b4[3]=b3[2]&0x3f;for(int j=0;j<4;j++)out+=B64C[b4[j]];i=0;}}
  if(i){for(int j=i;j<3;j++)b3[j]=0;b4[0]=(b3[0]&0xfc)>>2;b4[1]=((b3[0]&3)<<4)+((b3[1]&0xf0)>>4);b4[2]=((b3[1]&0xf)<<2)+((b3[2]&0xc0)>>6);for(int j=0;j<i+1;j++)out+=B64C[b4[j]];while(i++<3)out+='=';}
  return out;
}
