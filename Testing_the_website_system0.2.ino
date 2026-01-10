#include <FastLED.h>
//ความยาวของชิบ
#define NUM1 59
#define NUM2 59
#define NUM3 59
#define NUM4 59

// สร้าง buffer ให้แต่ละสาย
CRGB leds1[NUM1];
CRGB leds2[NUM2];
CRGB leds3[NUM3];
CRGB leds4[NUM4];

// สถานะเปิด/ปิด
bool pwr1 = true;
bool pwr2 = true;
bool pwr3 = true;
bool pwr4 = true;

// โหมดแต่ละเส้น
uint8_t mode1 = 0;
uint8_t mode2 = 0;
uint8_t mode3 = 0;
uint8_t mode4 = 0;

CRGB color1 = CRGB::White;
CRGB color2 = CRGB::White;
CRGB color3 = CRGB::White;
CRGB color4 = CRGB::White;

uint8_t hue1 = 0;
uint8_t hue2 = 0;
uint8_t hue3 = 0;
uint8_t hue4 = 0;


void setup() {
  Serial.begin(115200);

  // ต้องกำหนดแบบนี้เท่านั้น — ห้ามใช้ array index
  FastLED.addLeds<WS2811, 5,   GRB>(leds1, NUM1);   // OUT1
  FastLED.addLeds<WS2811, 18,  GRB>(leds2, NUM2);   // OUT2
  FastLED.addLeds<WS2811, 19,  GRB>(leds3, NUM3);   // OUT3
  FastLED.addLeds<WS2811, 21,  GRB>(leds4, NUM4);   // OUT4

  FastLED.setBrightness(200);
}


// เอฟเฟกต์ Rainbow function
void rainbow(CRGB* strip, int num, uint8_t &h) {
  for (int i = 0; i < num; i++) {
    strip[i] = CHSV((h + i * 255 / num) % 255, 255, 255);
  }
  h++;
}


// ตั้งสีคงที่
void solid(CRGB* strip, int num, CRGB color) {
  for (int i = 0; i < num; i++) strip[i] = color;
}


void loop() {
  // -------- STRIP 1 --------
  if (!pwr1) fill_solid(leds1, NUM1, CRGB::Black);
  else if (mode1 == 0) rainbow(leds1, NUM1, hue1);
  else solid(leds1, NUM1, color1);

  // -------- STRIP 2 --------
  if (!pwr2) fill_solid(leds2, NUM2, CRGB::Black);
  else if (mode2 == 0) rainbow(leds2, NUM2, hue2);
  else solid(leds2, NUM2, color2);

  // -------- STRIP 3 --------
  if (!pwr3) fill_solid(leds3, NUM3, CRGB::Black);
  else if (mode3 == 0) rainbow(leds3, NUM3, hue3);
  else solid(leds3, NUM3, color3);

  // -------- STRIP 4 --------
  if (!pwr4) fill_solid(leds4, NUM4, CRGB::Black);
  else if (mode4 == 0) rainbow(leds4, NUM4, hue4);
  else solid(leds4, NUM4, color4);

  FastLED.show();
  delay(20);

  // ----- รับคำสั่ง Serial -----
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    command(cmd);
  }
}

void command(String c) {

  int st = c.substring(0,1).toInt(); 
  String ac = c.substring(1);

  if (st == 1) {
    if (ac == "on")  pwr1 = true;
    else if (ac == "off") pwr1 = false;
    else if (ac == "rain") mode1 = 0;
    else if (ac == "red") { mode1 = 1; color1 = CRGB::Red; }
    else if (ac == "green") { mode1 = 1; color1 = CRGB::Green; }
    else if (ac == "blue") { mode1 = 1; color1 = CRGB::Blue; }
  }

  if (st == 2) {
    if (ac == "on")  pwr2 = true;
    else if (ac == "off") pwr2 = false;
    else if (ac == "rain") mode2 = 0;
    else if (ac == "red") { mode2 = 1; color2 = CRGB::Red; }
    else if (ac == "green") { mode2 = 1; color2 = CRGB::Green; }
    else if (ac == "blue") { mode2 = 1; color2 = CRGB::Blue; }
  }

  if (st == 3) {
    if (ac == "on")  pwr3 = true;
    else if (ac == "off") pwr3 = false;
    else if (ac == "rain") mode3 = 0;
    else if (ac == "red") { mode3 = 1; color3 = CRGB::Red; }
    else if (ac == "green") { mode3 = 1; color3 = CRGB::Green; }
    else if (ac == "blue") { mode3 = 1; color3 = CRGB::Blue; }
  }

  if (st == 4) {
    if (ac == "on")  pwr4 = true;
    else if (ac == "off") pwr4 = false;
    else if (ac == "rain") mode4 = 0;
    else if (ac == "red") { mode4 = 1; color4 = CRGB::Red; }
    else if (ac == "green") { mode4 = 1; color4 = CRGB::Green; }
    else if (ac == "blue") { mode4 = 1; color4 = CRGB::Blue; }
  }
}
