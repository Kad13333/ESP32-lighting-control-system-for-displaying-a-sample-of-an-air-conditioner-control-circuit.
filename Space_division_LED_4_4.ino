#include <FastLED.h>

// -------------------------------------
// ตั้งค่าจำนวนหลอด & ชนิดชิป
#define LED_PIN_1     5         // GPIO ของบอร์ด ZY-ESP32
#define LED_PIN_2   18          // GPIO ของบอร์ด ZY-ESP32
#define LED_PIN_3   19          // GPIO ของบอร์ด ZY-ESP32
#define NUM_LEDS_1    59        // จำนวนหลอดตามจริง ของวงจรที1
#define NUM_LEDS_2    59        // จำนวนหลอดตามจริง ของวงจรที2
#define NUM_LEDS_3    59        // จำนวนหลอดตามจริง ของวงจรที3
#define LED_TYPE    WS2811      // 72D ใช้โปรโตคอลแบบ WS2811
#define COLOR_ORDER RGB         // การเรียงลำดับสีไฟ
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
// -------------------------------------
// ตัวแปรโหมด
bool ledPower = true;         // เปิด/ปิดไฟทั้งหมด
bool rainbowMode = false;     // โหมดสายรุ้ง
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
  FastLED.clear();
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
    leds3[i] = CRGB(70,130,180);
  }
  for (int i = 20; i < 40 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB(90,110,130);
  }
  for (int i = 40; i < 59 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB(144,238,144);
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
    else {
      Serial.println("คำสั่งที่ใช้ได้:\n on -เปิดไฟ \n off -ปิดไฟ \n rainbow -เปิดรุง \n fixed -เปิดทัวไป \n fade -ไฟวิง \n fadeC -ไฟวิง ตามจำทีระบุ EX:fadeC 20 \n p1 -รูปแบบ 1-3 EX:p2 \n  offp1 -ปิดรูปแบบ 1-3 EX:offp2  \n  l -ควบคุมแยกเส้น l1-l3 ระยะติดที่ 0-60 ระยะติดินสุดที่ 0-60 เปิดหรือปิด on-off EX:l2 5 50 on  /n");
    }
  }
}    

//การตั้งค่าส่งออกข้อมูล
void setup() {
  Serial.begin(115200);

  FastLED.addLeds<LED_TYPE, LED_PIN_1, COLOR_ORDER>(leds1, NUM_LEDS_1);
  FastLED.addLeds<LED_TYPE, LED_PIN_2, COLOR_ORDER>(leds2, NUM_LEDS_2);
  FastLED.addLeds<LED_TYPE, LED_PIN_3, COLOR_ORDER>(leds3, NUM_LEDS_3);
  //FastLED.setBrightness(255); //ลับคับความสวาง
  applyBrightness();
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