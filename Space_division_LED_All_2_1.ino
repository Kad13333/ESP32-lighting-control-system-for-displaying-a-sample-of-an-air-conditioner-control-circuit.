#include <FastLED.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
//-----------------------------
// ตั้งค่าจำนวนหลอด & ชนิดชิป & สวิทช์
#define LED_PIN_1     5         // GPIO ของบอร์ด ZY-ESP32
#define LED_PIN_2   18          // GPIO ของบอร์ด ZY-ESP32
#define LED_PIN_3   19          // GPIO ของบอร์ด ZY-ESP32
#define NUM_LEDS_1    59        // จำนวนหลอดตามจริง ของวงจรที1
#define NUM_LEDS_2    59        // จำนวนหลอดตามจริง ของวงจรที2
#define NUM_LEDS_3    59        // จำนวนหลอดตามจริง ของวงจรที3
#define LED_TYPE    WS2811      // 72D ใช้โปรโตคอลแบบ WS2811
#define COLOR_ORDER RGB         // การเรียงลำดับสีไฟ
#define BTN_COUNT 5             // ขาสวิทช์
#define SW_po1    0             // ข้อมูลคำสั่งของ สวิทช์ กลุ่มที่ 1
#define SW_po2    1             // ข้อมูลคำสั่งของ สวิทช์ กลุ่มที่ 2
#define SW_po3    2             // ข้อมูลคำสั่งของ สวิทช์ กลุ่มที่ 3

// ---------------- Set up basic LED settings. ---------------------
// ตัวเก็บขอมูล
CRGB leds1[NUM_LEDS_1];
CRGB leds2[NUM_LEDS_2];        
CRGB leds3[NUM_LEDS_3];

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

uint8_t globalBrightness = 90;   // ความสว่าง (0–255)
uint8_t Fadet_time = 10;         // นวงเวล่า (10)
uint8_t Brightness_delay = 1;    // หน่วงเวลาความสว่าง (1)
uint8_t Brightness_delay_2 = 4;  // หน่วงเวลาความสว่างไฟวิง (4)
// ตัวแปรโหมด
bool ledPower = true;         // เปิด/ปิดไฟทั้งหมด
bool rainbowMode = false;     // โหมดสายรุ้ง

// ---------------- Set up basic switches. ---------------------
//ตั้งค่าสวิทช์พื้นฐาน
const uint8_t BTN_PIN[BTN_COUNT] = {27, 26, 25, 33, 32};           // ขาปุ่มกด

bool btnState[BTN_COUNT]   = {false, false, false, false, false};     // สถานะ ON / OFF กลาง
bool lastBtnState[BTN_COUNT] = {false, false, false, false, false};
bool lastBtn[BTN_COUNT]   = {HIGH, HIGH, HIGH, HIGH, HIGH};         // สถานะปุ่มก่อนหน้า

// ---------------- WiFi AP. ---------------------
const char* ssid = "ESP32_CONTROL";
const char* password = "12345678";

DNSServer dnsServer;
WebServer server(80);

//Button 
const char* buttonName[9] = {
  "ZONE A","ZONE B","ZONE C",
  "RAINBOW","RED","GREEN","BLUE",
  "ALL OFF","RESET"
}; 
bool sw[9] = {0};

// Serial Log
String serialLog = "";

//---------------------Set the basic UI system settings.----------------
void SerialBridge(String msg){
  Serial.println(msg);
  serialLog += msg + "\n";

  // จำกัดขนาด log กัน RAM เต็ม
  if(serialLog.length() > 4000){
    serialLog.remove(0, serialLog.length() - 3000);
  }
}
//-------------- HTML --------------------------
String MAIN_page(){
  String page = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
button{width:100%;height:45px;font-size:18px;margin:5px}
.on{background:green;color:white}
.off{background:red;color:white}
#console{position:fixed;bottom:0;width:100%;background:black;color:#0f0}
#log{
  height:120px;
  overflow-y:auto;
  white-space:pre;   /* สำคัญมาก */
  padding:5px;
}
</style>

<script>
setInterval(()=>{
  fetch('/state')
    .then(r=>r.json())
    .then(s=>{
      for(let i=1;i<=9;i++){
        let b = document.getElementById('b'+i);
        b.className = s['b'+i] ? 'on' : 'off';
      }
    });
},500);
</script>

<script>
function toggle(btn){
  fetch('/toggle?b=' + btn)
    .then(r => r.json())
    .then(data => {
      const el = document.getElementById('b' + btn);
      if(data.state){
        el.className = 'on';
      }else{
        el.className = 'off';
      }
    });
}

function sendCmd(e){
  if(e.key === 'Enter'){
    const cmd = document.getElementById('cmd').value;
    fetch('/cmd?cmd=' + encodeURIComponent(cmd));
    document.getElementById('cmd').value = '';
  }
}

// ดึง Serial log ทุก 1 วิ
setInterval(()=>{
  fetch('/log')
    .then(r=>r.text())
    .then(t=>{
      const log = document.getElementById("log");
      log.textContent = t;
      log.scrollTop = log.scrollHeight;
    });
},1000);
</script>


<script>
function toggle(b){
 fetch('/toggle?b='+b)
 .then(r=>r.json())
 .then(d=>{
  let e=document.getElementById('b'+b);
  e.className=d.state?'on':'off';
 });
}

setInterval(()=>{
 fetch('/log').then(r=>r.text()).then(t=>{
  log.textContent=t;
  log.scrollTop=log.scrollHeight;
 });
},1000);

function sendCmd(e){
 if(e.key==='Enter'){
  fetch('/cmd?cmd='+cmd.value);
  cmd.value='';
 }
}
</script>
</head>
<body>
<h2>ESP32 Control</h2>
)HTML";

  for(int i=0;i<9;i++){
    page += "<button id='b"+String(i+1)+"' class='off' onclick='toggle(";
    page += String(i+1)+")'>"+String(buttonName[i])+"</button>";
  }

  page += R"HTML(
<div id="console">
 <div id="log">Serial Ready...</div>
 <input id="cmd" onkeydown="sendCmd(event)" placeholder="command">
</div>
</body>
</html>
)HTML";

  return page;
}

//ให้ “เว็บรู้” เมื่อบอร์ดถูกสั่งจากภายนอก สร้าง API ส่งสถานะทั้งหมด
void handleState(){
  String json = "{";
  for(int i=0;i<9;i++){
    json += "\"b"+String(i+1)+"\":";
    json += sw[i] ? "true" : "false";
    if(i<8) json += ",";
  }
  json += "}";

  server.send(200,"application/json",json);
}

//สร้างฟังก์ชันควบคุมปุ่ม “ศูนย์กลาง”
void setButton(uint8_t index, bool state){
  if(index >= 9) return;

  sw[index] = state;

  SerialBridge(
    String("[SET] BTN ") + (index+1) +
    " " + buttonName[index] +
    " => " + (state ? "ON" : "OFF")
  );
// ===== logic แยกของแต่ละปุ่ม =====
  switch(index){
    case 0:
      if(state){ SerialBridge("ZONE A ENABLE");
                setSwitch(index, true, "SERIAL");
      }
      else{      SerialBridge("ZONE A DISABLE");
                setSwitch(index, false, "SERIAL");
      }        
      break;

    case 1:
      if(state){ SerialBridge("ZONE B ENABLE");
      }
      else{      SerialBridge("ZONE B DISABLE");
      }
      break;

    case 2:
      if(state){ SerialBridge("ZONE C ENABLE");
      }
      else{      SerialBridge("ZONE C DISABLE");
      }
      break;

    case 3:
      if(state){ SerialBridge("RAINBOW MODE ON");
      }
      else{      SerialBridge("RAINBOW MODE OFF");
      }
      break;
    case 4:
      if(state){ SerialBridge("RAINBOW MODE ON");
      }
      else{      SerialBridge("RAINBOW MODE OFF");
      }
      break;
      case 5:
      if(state){ SerialBridge("RAINBOW MODE ON");
      }
      else{      SerialBridge("RAINBOW MODE OFF");
      }
      break;
      case 6:
      if(state){ SerialBridge("RAINBOW MODE ON");
      }
      else{      SerialBridge("RAINBOW MODE OFF");
      }
      break;
      case 7:
      if(state){ SerialBridge("RAINBOW MODE ON");
      }
      else{      SerialBridge("RAINBOW MODE OFF");
      }
      break;
      case 8:
      if(state){ SerialBridge("RAINBOW MODE ON");
                 fadeAll_Off();
                 rainbowLoop();
                 ledPower = true;
                 rainbowMode = true;
                 fadeAll_On();
                 Serial.println("MODE: RAINBOW");
      }
      else{      SerialBridge("RAINBOW MODE OFF");
      }
      break;
  }
}

void handleRoot(){
  server.send(200,"text/html",MAIN_page());
}

void handleLog(){
  server.send(200,"text/plain",serialLog);
}

void handleCommand(){
  String cmd = server.arg("cmd");
  SerialBridge("CMD > "+cmd);

  if(cmd=="status"){
    for(int i=0;i<9;i++)
      SerialBridge("Button "+String(i+1)+" = "+(sw[i]?"ON":"OFF"));
  }
}

void handleToggle(){
  int b = server.arg("b").toInt() - 1;
  if(b < 0 || b >= 9){
    server.send(400,"text/plain","Invalid");
    return;
  }

  setButton(b, !sw[b]);   // ⭐ ใช้ฟังก์ชันกลาง

  server.send(
    200,
    "application/json",
    String("{\"state\":") + (sw[b] ? "true" : "false") + "}"
  );
}

//------------------- Commands used with switches. ---------------
//คำสั่งที่ใช้ร่วมกับสวิตช์
void toggleButton(uint8_t i) {
  btnState[i] = !btnState[i];

  Serial.print("BTN ");
  Serial.print(i + 1);
  Serial.print(" -> ");
  Serial.println(btnState[i] ? "ON" : "OFF");
}

//ฟังก์ชันตั้งค่าสวิตช์จากภายนอก
void setSwitch(uint8_t i, bool state, const char* source) {
  if (btnState[i] != state) {   // เปลี่ยนเฉพาะเมื่อสถานะต่าง
    btnState[i] = state;

    Serial.print(source);
    Serial.print(" SET BTN ");
    Serial.print(i + 1);
    Serial.print(" -> ");
    Serial.println(state ? "ON" : "OFF");
  }
}

// อ่านปุ่ม
void readButtons() {
  for (uint8_t i = 0; i < BTN_COUNT; i++) {
    bool nowBtn = digitalRead(BTN_PIN[i]);

    if (lastBtn[i] == HIGH && nowBtn == LOW) {
      toggleButton(i);
      delay(150); // debounce
    }

    lastBtn[i] = nowBtn;
  }
}

//สังเเต่หนึง
void readButtons_on() {

  if (!lastBtnState[0] && btnState[0]) {
    ledPower = true;
    fadeAll_On();
    Serial.println("POWER ON");
  }
  if (lastBtnState[0] && !btnState[0]) {
    ledPower = false;
    fadeAll_Off();
    Serial.println("POWER OFF");
  }

  if (!lastBtnState[1] && btnState[1]) {
    Serial.println("hello world2");
  }

  if (!lastBtnState[2] && btnState[2]) {
    Serial.println("hello world3");
  }

  if (!lastBtnState[3] && btnState[3]) {
    Serial.println("hello world4");
  }

  if (!lastBtnState[4] && btnState[4]) {
    Serial.println("hello world5");
  }

  for (int i = 0; i < BTN_COUNT; i++) {
    lastBtnState[i] = btnState[i];
  }
}

// ---------------- LED control commands. ---------------------
//คุมความสว่าง (ศูนย์กลาง)
void applyBrightness() {
  FastLED.setBrightness(globalBrightness);
}

// ฟังก์ชันนวงเวล่าความสว่าง
void fadeAll_On() { //เปิด
  for (int b = 0; b <= globalBrightness; b += Brightness_delay) {
    FastLED.setBrightness(b);
    FastLED.show();
    delay(Fadet_time);
  }
}

void fadeAll_Off() { //ปิด
  for (int b = globalBrightness; b >= 0; b -= Brightness_delay) {
    FastLED.setBrightness(b);
    FastLED.show();
    delay(Fadet_time);
  }
}

void fadeAll_Fadet(int fromB, int toB) { //ค่อยๆเปลียน
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

// ตั้งค่าสีเริ่มต้นแต่ละช่วง (ศูนย์กลาง)
void setFixedColors() {
  FastLED.clear(); // ตั้งค่า LED ทุกดวงให้เป็น CRGB(0, 0, 0); // ดำ / ดับ
  // ---------- LED LINE 1 ----------
  for (int i = 0; i < 20 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Blue;
  }
  for (int i = 20; i < 40 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Red;
  }
  for (int i = 40; i < 59 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Green;
  }
  // ---------- LED LINE 2 ----------
  for (int i = 0; i < 20 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB(70, 130, 180); 
  }
  for (int i = 20; i < 40 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB::Red;
  }
  for (int i = 40; i < 59 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB(255,120,0);
  }
  // ---------- LED LINE 3 ----------
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

//--------------------- For use with CMD. -------------------
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

// เปิด-ปิดเฉพาะ Zone (ไม่กระทบ Zone อื่น)
void zoneOff(int startLED, int endLED) {
  startLED = constrain(startLED, 0, NUM_LEDS_1 - 1);
  endLED   = constrain(endLED, 0, NUM_LEDS_1 - 1);

  for (int i = startLED; i <= endLED; i++) {
    leds1[i] = CRGB::Black;
  }
  FastLED.show();
}
// ---------- LED LINE 1 ----------
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
// ---------- LED LINE 2 ----------
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
// ---------- LED LINE 3 ----------
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
// เปลียน สี ทังหมด
void Change_color(CRGB* leds, int numLeds,int r, int g, int b,int start, int end) {

  start = constrain(start, 0, numLeds - 1);
  end   = constrain(end,   0, numLeds - 1);

  for (int i = start; i <= end; i++) {
    leds[i] = CRGB(r, g, b);
  }
  FastLED.show();
}






//-------------------Read commands via Serial. (CMD)------------------------
// อ่านคำสั่งผ่าน Serial ใช้สำหรับการติดตั้ง
void checkCommand() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

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

    // รูปแบบคำสั่ง: set on 2 / set off 2
    else if (cmd.startsWith("set")) {
    int s1 = cmd.indexOf(' ');
    int s2 = cmd.indexOf(' ', s1 + 1);

    if (s1 < 0 || s2 < 0) return;

    String action = cmd.substring(s1 + 1, s2);
    int index = cmd.substring(s2 + 1).toInt() - 1;

    if (index < 0 || index >= BTN_COUNT) return;

    if (action == "on") {
      setSwitch(index, true, "SERIAL");
    } 
    else if (action == "off") {
      setSwitch(index, false, "SERIAL");
    }
  }

    else {
      Serial.println("คำสั่งที่ใช้ได้:\n on -เปิดไฟ \n off -ปิดไฟ \n rainbow -เปิดรุง  \n fade -ไฟวิง \n fadeC -ไฟวิง ตามจำทีระบุ EX:fadeC 20 \n  l -ควบคุมแยกเส้น l1-l3 ระยะติดที่ 0-60 ระยะติดินสุดที่ 0-60 เปิดหรือปิด on-off สี  red  green blue white black EX:l2 5 50 on red /n color EX.color l1 0 10 0 0 255    เส้น 1 สีน้ำเงิน ");
    }
  }
}

//---------------------- Data export setup. ----------------------------
//การตั้งค่าส่งออกข้อมูล
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid,password);
  dnsServer.start(53,"*",WiFi.softAPIP());
  server.on("/state", handleState);
  server.on("/",handleRoot);
  server.on("/toggle",handleToggle);
  server.on("/log",handleLog);
  server.on("/cmd",handleCommand);
  server.onNotFound(handleRoot);
  server.begin();

  Serial.println("ESP32 READY");
  Serial.println(WiFi.softAPIP());

  for (uint8_t i = 0; i < BTN_COUNT; i++) {
    pinMode(BTN_PIN[i], INPUT_PULLUP);
  }

  FastLED.addLeds<LED_TYPE, LED_PIN_1, COLOR_ORDER>(leds1, NUM_LEDS_1);
  FastLED.addLeds<LED_TYPE, LED_PIN_2, COLOR_ORDER>(leds2, NUM_LEDS_2);
  FastLED.addLeds<LED_TYPE, LED_PIN_3, COLOR_ORDER>(leds3, NUM_LEDS_3);
  FastLED.clear();
  //FastLED.setBrightness(255); //บังคับความสวาง
  applyBrightness(); //บังคับความสวาง จากสูนกลาง
  setFixedColors();
}

// ----------------- Loop data export settings. --------------------
//วนซ้ำ
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  readButtons();  // ปุ่มจริง
  checkCommand(); // คำสั่ง cmd
  readButtons_on(); // logic ของคุณ


  if (rainbowMode && ledPower) {
    rainbowLoop();
  }

  if(Serial.available()){
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    // off 1
    if(cmd.startsWith("off")){
      int n = cmd.substring(4).toInt() - 1;
      setButton(n, false);
    }

    // on 1
    else if(cmd.startsWith("on")){
      int n = cmd.substring(3).toInt() - 1;
      setButton(n, true);
    }
  }
}