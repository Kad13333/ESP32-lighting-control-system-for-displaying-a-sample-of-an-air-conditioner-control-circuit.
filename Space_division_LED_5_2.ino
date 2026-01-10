#include <FastLED.h>

// -------------------------------------
// ตั้งค่าจำนวนหลอด & ชนิดชิป & สวิทช์
#define LED_PIN_1     5         // GPIO ของบอร์ด ZY-ESP32
#define LED_PIN_2   18          // GPIO ของบอร์ด ZY-ESP32
#define LED_PIN_3   19          // GPIO ของบอร์ด ZY-ESP32
#define NUM_LEDS_1    59        // จำนวนหลอดตามจริง ของวงจรที1
#define NUM_LEDS_2    59        // จำนวนหลอดตามจริง ของวงจรที2
#define NUM_LEDS_3    59        // จำนวนหลอดตามจริง ของวงจรที3
#define LED_TYPE    WS2811      // 72D ใช้โปรโตคอลแบบ WS2811
#define COLOR_ORDER RGB         // การเรียงลำดับสีไฟ
#define BTN_PIN_1 25              // GPIO ของบอร์ด ZY-ESP32
#define BTN_PIN_2 26              // GPIO ของบอร์ด ZY-ESP32
#define BTN_PIN_3 27              // GPIO ของบอร์ด ZY-ESP32
#define BTN_PIN_4 33              // GPIO ของบอร์ด ZY-ESP32

// -------------------------------------
// ตัวเก็บขอมูล
CRGB leds1[NUM_LEDS_1];        
CRGB leds2[NUM_LEDS_2];        
CRGB leds3[NUM_LEDS_3];
// -------------------------------------
// ความสว่าง (ศูนย์กลาง)  
uint8_t globalBrightness = 90;   // 0–255
// -------------------------------------
// นวงเวล่า (ศูนย์กลาง)  
uint8_t Fadet_time = 10;
// หน่วงเวลาความสว่าง (ศูนย์กลาง)  
uint8_t Brightness_delay = 1;
// หน่วงเวลาความสว่างไฟวิง (ศูนย์กลาง)  
uint8_t Brightness_delay_2 = 4;      
// -------------------------------------
// ตัวแปรโหมด
bool ledPower = true;         // เปิด/ปิดไฟทั้งหมด
bool rainbowMode = false;     // โหมดสายรุ้ง
// -------------------------------------
// ตัวแปรสวิตช์
bool BTN_state1 = false;     // สถานะ ON / OFF กลาง
bool BTN_state2 = false;
bool BTN_state3 = false;
bool BTN_state4 = false;
bool BTN_lastBtn1 = HIGH;         // สถานะปุ่มก่อนหน้า
bool BTN_lastBtn2 = HIGH;
bool BTN_lastBtn3 = HIGH;
bool BTN_lastBtn4 = HIGH;
// -------------------------------------
//คุมความสว่าง (ศูนย์กลาง)
void applyBrightness() {
  FastLED.setBrightness(globalBrightness);
}

// -------------------------------------
// ฟังก์ชันนวงเวล่าความสว่าง
void fadeAll_On() {
  for (int b = 0; b <= globalBrightness; b += Brightness_delay) {
    FastLED.setBrightness(b);
    FastLED.show();
    delay(Fadet_time);
  }
}
void fadeAll_Off() {
  for (int b = globalBrightness; b >= 0; b -= Brightness_delay) {
    FastLED.setBrightness(b);
    FastLED.show();
    delay(Fadet_time);
  }
}
void fadeAll_Fadet(int fromB, int toB) {
  if (fromB == toB) return;
  
  int step = (fromB > toB) ? -Brightness_delay : Brightness_delay;

  for (int b = fromB; b != toB; b += step) {
    FastLED.setBrightness(b);
    FastLED.show();
    delay(Fadet_time);
  }

  FastLED.setBrightness(toB);
  FastLED.show();
}

// -------------------------------------
// ตั้งค่าสีเริ่มต้นแต่ละช่วง (ศูนย์กลาง)
void setFixedColors() {
  FastLED.clear(); // ตั้งค่า LED ทุกดวงให้เป็น CRGB(0, 0, 0); // ดำ / ดับ
  // ---------- LINE 1 ----------
  for (int i = 0; i < 20 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Blue;
  }
  for (int i = 20; i < 40 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Red;
  }
  for (int i = 40; i < 59 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Green;
  }
  // ---------- LINE 2 ----------
  for (int i = 0; i < 20 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB(70, 130, 180); 
  }
  for (int i = 20; i < 40 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB::Red;
  }
  for (int i = 40; i < 59 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB(255,120,0);
  }
  // ---------- LINE 3 ----------
  for (int i = 0; i < 20 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB(70,130,255);
  }
  for (int i = 20; i < 40 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB(90,110,255);
  }
  for (int i = 40; i < 59 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB(144,238,255);
  }
  fadeAll_On();    
  FastLED.show(); 
}

// -------------------------------------
// ฟังก์ชันสายรุ้ง
void rainbowLoop() {
  static uint8_t hue = 0;
  for (int i = 0; i < NUM_LEDS_1; i++) leds1[i] = CHSV(hue + (i * 3), 255, ledPower ? 255 : 0);
  for (int i = 0; i < NUM_LEDS_2; i++) leds2[i] = CHSV(hue + (i * 3), 255, ledPower ? 255 : 0);
  for (int i = 0; i < NUM_LEDS_3; i++) leds3[i] = CHSV(hue + (i * 3), 255, ledPower ? 255 : 0);

  hue++;
  FastLED.show();
  delay(20);
}

// -------------------------------------
// ไฟติดทีละลอท
void fadeInSequential() {
  if (!ledPower) return;
  fadeAll_Off();
  FastLED.clear();
  applyBrightness();

  for (int i = 0; i < NUM_LEDS_1; i++) {

    // ไล่ความสว่างจาก 0 → globalBrightness
    for (int b = 0; b <= globalBrightness; b += Brightness_delay_2) {
      leds1[i] = CHSV(160, 255, b);  // สีฟ้าอ่อน → ฟ้าเข้ม (ปรับสีได้)
      FastLED.show();
      delay(5);
    }

    leds1[i] = CHSV(160, 255, globalBrightness);
    FastLED.show();
  }
}
void fadeInSequential_Custom(int countLEDs) {
  if (!ledPower) return;
  if (countLEDs < 1) return;
  fadeAll_Off(); 
  FastLED.clear();
  applyBrightness();

  int limit = min(countLEDs, NUM_LEDS_1);

  for (int i = 0; i < limit; i++) {
    for (int b = 0; b <= globalBrightness; b += Brightness_delay_2) {
      leds1[i] = CHSV(160, 255, b);
      FastLED.show();
      delay(5);
    }

    leds1[i] = CHSV(160, 255, globalBrightness);
    FastLED.show();
  }
}

// -------------------------------------
// เปิด-ปิดเฉพาะ Zone (ไม่กระทบ Zone อื่น)
void zoneOff(int startLED, int endLED) {
  startLED = constrain(startLED, 0, NUM_LEDS_1 - 1);
  endLED   = constrain(endLED, 0, NUM_LEDS_1 - 1);

  for (int i = startLED; i <= endLED; i++) {
    leds1[i] = CRGB::Black;
  }
  FastLED.show();
}
// ---------- LINE 1 ----------
void line1_zoneOn(int start, int end, CRGB color) {
  start = constrain(start, 0, NUM_LEDS_1 - 1);
  end   = constrain(end,   0, NUM_LEDS_1 - 1);

  for (int i = start; i <= end; i++) {
    leds1[i] = color;
  }
  FastLED.show();
}
void line1_zoneOff(int start, int end) {
  start = constrain(start, 0, NUM_LEDS_1 - 1);
  end   = constrain(end,   0, NUM_LEDS_1 - 1);

  for (int i = start; i <= end; i++) {
    leds1[i] = CRGB::Black;
  }
  FastLED.show();
}
// ---------- LINE 2 ----------
void line2_zoneOn(int start, int end, CRGB color) {
  start = constrain(start, 0, NUM_LEDS_2 - 1);
  end   = constrain(end,   0, NUM_LEDS_2 - 1);

  for (int i = start; i <= end; i++) {
    leds2[i] = color;
  }
  FastLED.show();
}
void line2_zoneOff(int start, int end) {
  start = constrain(start, 0, NUM_LEDS_2 - 1);
  end   = constrain(end,   0, NUM_LEDS_2 - 1);

  for (int i = start; i <= end; i++) {
    leds2[i] = CRGB::Black;
  }
  FastLED.show();
}
// ---------- LINE 3 ----------
void line3_zoneOn(int start, int end, CRGB color) {
  start = constrain(start, 0, NUM_LEDS_3 - 1);
  end   = constrain(end,   0, NUM_LEDS_3 - 1);

  for (int i = start; i <= end; i++) {
    leds3[i] = color;
  }
  FastLED.show();
}
void line3_zoneOff(int start, int end) {
  start = constrain(start, 0, NUM_LEDS_3 - 1);
  end   = constrain(end,   0, NUM_LEDS_3 - 1);

  for (int i = start; i <= end; i++) {
    leds3[i] = CRGB::Black;
  }
  FastLED.show();
}

// -------------------------------------
// เปลียน สี ทังหมด
void Change_color(CRGB* leds, int numLeds,int r, int g, int b,int start, int end) {

  start = constrain(start, 0, numLeds - 1);
  end   = constrain(end,   0, numLeds - 1);

  for (int i = start; i <= end; i++) {
    leds[i] = CRGB(r, g, b);
  }
  FastLED.show();
}

// -------------------------------------
// ฟังก์ชันแปลงชื่อสี
CRGB parseColor(String c) {
  c.toLowerCase();
  if (c == "red")   return CRGB::Red;
  if (c == "green") return CRGB::Green;
  if (c == "blue")  return CRGB::Blue;
  if (c == "white") return CRGB::White;
  if (c == "black") return CRGB::Black;
  return CRGB::White;
}

// -------------------------------------
// อ่านคำสั่งผ่าน Serial
void checkCommand() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.equalsIgnoreCase("on")) {
      ledPower = true;
      rainbowMode = false;
      setFixedColors();
      Serial.println("LED: ON");
    }
    else if (cmd.equalsIgnoreCase("off")) {
      ledPower = false;
      rainbowMode = false;
      fadeAll_Off();
      Serial.println("LED: OFF");
    }
    else if (cmd.startsWith("bright")) {
      String numText = cmd.substring(6);
      numText.trim();

      int value = constrain(numText.toInt(), 0, 255);
      fadeAll_Fadet(globalBrightness, value);
      globalBrightness = value;
      applyBrightness();

      Serial.printf("Brightness set to %d\n", globalBrightness);
    }
    else if (cmd.equalsIgnoreCase("rainbow")) {
      fadeAll_Off();
      rainbowLoop();
      ledPower = true;
      rainbowMode = true;
      fadeAll_On();
      Serial.println("MODE: RAINBOW");
    }
    else if (cmd.equalsIgnoreCase("fade")) {

      rainbowMode = false;
      Serial.println("MODE: FADE SEQUENTIAL");
      fadeInSequential();
    }
    else  if (cmd.startsWith("color")) {
      int s1 = cmd.indexOf(' ');
      int s2 = cmd.indexOf(' ', s1 + 1);
      int s3 = cmd.indexOf(' ', s2 + 1);
      int s4 = cmd.indexOf(' ', s3 + 1);
      int s5 = cmd.indexOf(' ', s4 + 1);
      int s6 = cmd.indexOf(' ', s5 + 1);
      if (s6 < 0){
        Serial.println("Invalid color command");
        return;
      }
      String line = cmd.substring(s1 + 1, s2);   // l1 / l2 / l3
        int start   = cmd.substring(s2 + 1, s3).toInt();
        int end     = cmd.substring(s3 + 1, s4).toInt();
        int r       = cmd.substring(s4 + 1, s5).toInt();
        int g       = cmd.substring(s5 + 1, s6).toInt();
        int b       = cmd.substring(s6 + 1).toInt();

        CRGB color = CRGB(r, g, b);

        if (line == "l1") {
          line1_zoneOn(start, end, color);
        }
        else if (line == "l2") {
          line2_zoneOn(start, end, color);
        }
        else if (line == "l3") {
          line3_zoneOn(start, end, color);
        }
    Serial.printf(
      "color %s %d %d %d %d %d",
      line.c_str(),
      start, end,
      r, g, b );
    }
    else if (cmd.startsWith("fadeC")) {

      // อ่านเฉพาะตัวเลขหลังคำว่า fadeC
      // ตัวเลขอาจมีช่องว่างหรือไม่มีช่องว่างก็ได้
      String numText = cmd.substring(5);
      numText.trim();

       int countLEDs = numText.toInt();

      Serial.printf("\nFading %d LEDs...\n EX:fadeC 25 ", countLEDs);
      fadeInSequential_Custom(countLEDs);
    }
    else if (cmd.startsWith("l1") || cmd.startsWith("l2") || cmd.startsWith("l3")) {

      int s1 = cmd.indexOf(' ');
      int s2 = cmd.indexOf(' ', s1 + 1);
      int s3 = cmd.indexOf(' ', s2 + 1);
      int s4 = cmd.indexOf(' ', s3 + 1);

      int start = cmd.substring(s1 + 1, s2).toInt();
      int end   = cmd.substring(s2 + 1, s3).toInt();
      String onoff = cmd.substring(s3 + 1, s4);
      String colorText = (s4 > 0) ? cmd.substring(s4 + 1) : "";

      bool on = onoff.equalsIgnoreCase("on");
      CRGB color = parseColor(colorText);

      if (cmd.startsWith("l1")) {
        if (on) line1_zoneOn(start, end, color);
        else    line1_zoneOff(start, end);
      }
      else if (cmd.startsWith("l2")) {
        if (on) line2_zoneOn(start, end, color);
        else    line2_zoneOff(start, end);
      }
      else if (cmd.startsWith("l3")) {
        if (on) line3_zoneOn(start, end, color);
        else    line3_zoneOff(start, end);
      }

      Serial.printf("%s %d-%d %s %s\n",
        cmd.substring(0,2).c_str(),
        start, end,
        on ? "ON" : "OFF",
        colorText.c_str()
      );
    }

    else {
      Serial.println("คำสั่งที่ใช้ได้:\n on -เปิดไฟ \n off -ปิดไฟ \n rainbow -เปิดรุง \n fixed -เปิดทัวไป \n fade -ไฟวิง \n fadeC -ไฟวิง ตามจำทีระบุ EX:fadeC 20 \n p1 -รูปแบบ 1-3 EX:p2 \n  offp1 -ปิดรูปแบบ 1-3 EX:offp2  \n  l -ควบคุมแยกเส้น l1-l3 ระยะติดที่ 0-60 ระยะติดินสุดที่ 0-60 เปิดหรือปิด on-off สี  red  green blue white black EX:l2 5 50 on red /n color EX.color l1 0 10 0 0 255    เส้น 1 สีน้ำเงิน ");
    }
  }
}    

//การตั้งค่าส่งออกข้อมูล
void setup() {
  Serial.begin(115200);

  FastLED.addLeds<LED_TYPE, LED_PIN_1, COLOR_ORDER>(leds1, NUM_LEDS_1);
  FastLED.addLeds<LED_TYPE, LED_PIN_2, COLOR_ORDER>(leds2, NUM_LEDS_2);
  FastLED.addLeds<LED_TYPE, LED_PIN_3, COLOR_ORDER>(leds3, NUM_LEDS_3);
  FastLED.clear();
  //FastLED.setBrightness(255); //บังคับความสวาง
  applyBrightness(); //บังคับความสวาง จากสูนกลาง
  setFixedColors();
}

// -------------------------------------
//วนซ้ำ
void loop() {
  checkCommand();

  if (rainbowMode && ledPower) {
    rainbowLoop();
  }
}