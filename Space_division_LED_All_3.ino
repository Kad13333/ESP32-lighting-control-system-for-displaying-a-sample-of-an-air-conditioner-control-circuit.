#include <FastLED.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
//-----------------------------
// ตั้งค่าจำนวนหลอด & ชนิดชิป & สวิทช์
#define LED_PIN_1     5         // GPIO ของบอร์ด ZY-ESP32
#define LED_PIN_2   18          // GPIO ของบอร์ด ZY-ESP32
#define LED_PIN_3   19          // GPIO ของบอร์ด ZY-ESP32
#define NUM_LEDS_1    73        // จำนวนหลอดตามจริง ของวงจรที1
#define NUM_LEDS_2    34        // จำนวนหลอดตามจริง ของวงจรที2
#define NUM_LEDS_3    61        // จำนวนหลอดตามจริง ของวงจรที3
#define LED_TYPE    WS2811      // 72D ใช้โปรโตคอลแบบ WS2811
#define COLOR_ORDER RGB         // การเรียงลำดับสีไฟ
#define BTN_COUNT 10             // ขาสวิตช์จริง 
#define HW_BTN_COUNT   10        // จำนวนสวิตช์จริง (ฮาร์ดแวร์)
#define WEB_BTN_COUNT  10        // จำนวนปุ่มในเว็บ

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

uint8_t globalBrightness = 50;   // ความสว่าง (0–255)
uint8_t Fadet_time = 10;         // นวงเวล่า (10)
uint8_t Brightness_delay = 1;    // หน่วงเวลาความสว่าง (1)
uint8_t Brightness_delay_2 = 4;  // หน่วงเวลาความสว่างไฟวิง (4)
// ตัวแปรโหมด
bool ledPower = true;         // เปิด/ปิดไฟทั้งหมด
bool rainbowMode = false;     // โหมดสายรุ้ง
bool requestRainbow = false;

// ---------------- Set up basic switches. ---------------------
//ตั้งค่าสวิทช์พื้นฐาน
const uint8_t BTN_PIN[HW_BTN_COUNT] = { 13, 14, 15, 16, 17, 25, 26, 32, 27, 33};           // ขาปุ่มกด{27, 26, 25, 33, 32};เพิ่มสวิตช์ได้✔ แนะนำ GPIO: 4, 13–17, 21–23⚠️ GPIO 34–39 ต้องมี R ดึง❌ หลีกเลี่ยง 0, 2, 6–11, 1, 3

bool btnState[WEB_BTN_COUNT] = {0};     // สถานะ ON / OFF กลาง
bool lastHWState[HW_BTN_COUNT] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
bool lastBtn[HW_BTN_COUNT]   = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};         // สถานะปุ่มก่อนหน้า
bool internalUpdate = false;
// -----------------Auto Mode---------------------
bool autoMode = false;

uint8_t autoStep = 0;
unsigned long autoTimer = 0;
bool autoStepStarted = false;  

// ---------------- WiFi AP. ---------------------
const char* ssid = "ESP32_CONTROL";
const char* password = "12345678";

DNSServer dnsServer;
WebServer server(80);

//Button 
const char* buttonName[10] = {
  "Power ON ALL","Motor HIGH","Motor MEDLUM","Motor LOW","COMP","COMP Timer Relay","COMP Delay","Breaker.","Auto", "RGB Run"
}; 
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
/* ---------- BASE ---------- */
body {
  margin: 0;
  padding: 10px;
  font-family: Arial, sans-serif;
  background: #f2f2f2;
}

/* ---------- TITLE ---------- */
h2 {
  text-align: center;
  margin: 10px 0;
}

/* ---------- BUTTON ---------- */
button {
  width: 100%;
  height: 48px;
  font-size: 18px;
  margin: 6px 0;
  border: none;
  border-radius: 6px;
}

.on {
  background: #2ecc71;
  color: white;
}

.off {
  background: #e74c3c;
  color: white;
}

/* ---------- CONSOLE ---------- */
#console {
  margin-top: 15px;
  background: black;
  color: #00ff00;
  border-radius: 6px;
}

#log {
  height: 120px;
  overflow-y: auto;
  white-space: pre-wrap;
  padding: 8px;
  font-size: 14px;
}
</style>

<script>
/* ---------- STATE ---------- */
function updateState(){
  fetch('/state')
    .then(r=>r.json())
    .then(s=>{
      for(let i=1;i<=11;i++){
        const b=document.getElementById('b'+i);
        if(b) b.className = s['b'+i] ? 'on' : 'off';
      }
    });
}
setInterval(updateState,1000);

/* ---------- TOGGLE ---------- */
function toggle(b){
  fetch('/toggle?b='+b)
    .then(r=>r.json())
    .then(d=>{
      document.getElementById('b'+b).className =
        d.state ? 'on' : 'off';
    });
}

/* ---------- LOG ---------- */
function updateLog(){
  fetch('/log')
    .then(r=>r.text())
    .then(t=>{
      const log=document.getElementById('log');
      log.textContent=t;
      log.scrollTop=log.scrollHeight;
    });
}
setInterval(updateLog,1000);
</script>

</head>
<body>

<h2>Demonstration Set of Electrical System for Refrigeration Equipment</h2>
)HTML";

  /* ---------- BUTTON LIST ---------- */
  for(int i = 0; i < WEB_BTN_COUNT; i++){
    page += "<button id='b"+String(i+1)+"' class='off' onclick='toggle("+String(i+1)+")'>"
         + String(buttonName[i]) + "</button>";
  }

  /* ---------- CONSOLE ---------- */
  page += R"HTML(
<div id="console">
  <div id="log">Serial Ready...</div>
</div>

</body>
</html>
)HTML";

  return page;
}




//ให้ “เว็บรู้” เมื่อบอร์ดถูกสั่งจากภายนอก สร้าง API ส่งสถานะทั้งหมด
void handleState(){
  String json = "{";
  for(int i=0;i<WEB_BTN_COUNT;i++){
      json += "\"b"+String(i+1)+"\":";
      json += btnState[i] ? "true" : "false";
    if(i < WEB_BTN_COUNT - 1) json += ",";
  }
  json += "}";

  server.send(200,"application/json",json);
}

//สร้างฟังก์ชันควบคุมปุ่ม “ศูนย์กลาง”
void setButton(uint8_t index, bool state){
  if(internalUpdate) return;
// ===== logic แยกของแต่ละปุ่ม =====
  internalUpdate = true;
  btnState[index] = state;
  switch(index){
     // ===== MASTER POWER =====
    case 0: 
      if(state){ 
          ledPower = true;
          //fadeAll_On();
          //setFixedColors();
          p1_Cirecuit_Breaker_ON();
          // รีเซ็ตสถานะปุ่มอื่น (เฉพาะ state)
          SerialBridge("Power ON ALL");
          /*setSwitch(index, true, "SERIAL");*/
      }
      else{
          ledPower = false;
          //fadeAll_Off();
          p1_Cirecuit_Breaker_OFF();
          // รีเซ็ตสถานะปุ่มอื่น (เฉพาะ state)
          for (int i = 1; i <= 7; i++) {
            btnState[i] = false;
          }
          p1_Motpr_HIGH_OFF();
          p1_Motpr_MEDLUM_OFF();
          p1_Motpr_LOW_OFF();
          p2_Condensing_Unil_0_OFF();
          p2_Timer_Relay_1_OFF();
          p2_Delay_on_Make_2_OFF();
          p3_Motot_FE_OFF();
          SerialBridge("Power OFF ALL");
           /* setSwitch(index, false, "SERIAL");*/
      }      
      break;

    case 1:
      if(state){ 
        SerialBridge("Motpr HIGH ON");
        p1_Motpr_HIGH_ON();
        btnState[2] = false;
        btnState[3] = false;
        p1_Motpr_MEDLUM_OFF();
        p1_Motpr_LOW_OFF();
      }
      else{
        SerialBridge("Motpr HIGH OFF");
        p1_Motpr_HIGH_OFF();
      }
      break;

    case 2:
      if(state){ 
        SerialBridge("Motpr MEDLUM ON");
        p1_Motpr_MEDLUM_ON();
        btnState[1] = false;
        btnState[3] = false;
        setButton(1, false);
        setButton(3, false);
        p1_Motpr_HIGH_OFF();
        p1_Motpr_LOW_OFF();
      }
      else{      
        SerialBridge("Motpr MEDLUM OFF");
        p1_Motpr_MEDLUM_OFF();
      }
      break;
    case 3:
      if(state){
        SerialBridge("Motpr LOW ON");
        p1_Motpr_LOW_ON();
        btnState[1] = false;
        btnState[2] = false;
        setButton(1, false);
        setButton(2, false);
        p1_Motpr_HIGH_OFF();
        p1_Motpr_MEDLUM_OFF();
      }
      else{
        SerialBridge("Motpr LOW OFF");
        p1_Motpr_LOW_OFF();
      }
      break;
      case 4:
      if(state){
        SerialBridge("COMP There is no delay. ON");
        p2_Condensing_Unil_0_ON();
        btnState[5] = false;
        btnState[6] = false;
        setButton(5, false);
        setButton(6, false);
      }
      else{ 
        SerialBridge("COMP There is no delay. OFF");
        p2_Condensing_Unil_0_OFF();
      }
      break;
      case 5:
      if(state){
         SerialBridge("COMP Timer Relay ON");
        p2_Timer_Relay_1_ON();
        btnState[4] = false;
        btnState[6] = false;
        setButton(4, false);
        setButton(6, false);
      }
      else{   
        SerialBridge("COMP Timer Relay OFF");
        p2_Timer_Relay_1_OFF();
      }
      break;
      case 6:
      if(state){ 
        SerialBridge("COMP Delay on Make ON");
        p2_Delay_on_Make_2_ON();
        btnState[4] = false;
        btnState[5] = false;
        setButton(4, false);
        setButton(5, false);
      }
      else{ 
        SerialBridge("COMP Delay on Make OFF");
        p2_Delay_on_Make_2_OFF();
      }
      break;
      case 7:
      if(state){ 
        SerialBridge("Below the circuit breaker. ON");
        p3_Motot_FE_ON();
      }
      else{
        SerialBridge("Below the circuit breaker. OFF");
        p3_Motot_FE_OFF();
      }
      break;
      case 8:
      if(state){ 
        SerialBridge("Auto ON");
        autoMode = true;
        autoStep = 0;
        autoTimer = 0;
        autoStepStarted = false;   
      }
      else{
        SerialBridge("Auto OFF");
        autoMode = false;
        autoTimer = 0;
        autoStepStarted = false;  

        for (int i = 0 ; i < WEB_BTN_COUNT; i++) {
          setSwitch(i, false, "AUTO");
        }
        setFixedColors();
      }
      break;

      case 9:
      if(state){ 
        SerialBridge(" ON");
        rainbowMode = true;
      }
      else{
        SerialBridge(" OFF");
        rainbowMode = false;
        fadeAll_On();
      }
      break;
  }
  internalUpdate = false;
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
      SerialBridge("Button "+String(i+1)+" = "+(btnState[i]?"ON":"OFF"));
  }
}

void handleToggle(){
  int b = server.arg("b").toInt() - 1;
  if(b < 0 || b >= WEB_BTN_COUNT){
    server.send(400,"text/plain","Invalid");
    return;
  }

  setSwitch(b, !btnState[b], "WEB");   //  ใช้ฟังก์ชันกลาง

  server.send(
    200,
    "application/json",
    String("{\"state\":") + (btnState[b] ? "true" : "false") + "}"
  );
}

//สร้าง Command Parser (หัวใจหลัก)
void processCommand(String cmd, const char* source){
  cmd.trim();
  cmd.toLowerCase();

  // set on 1 / set off 2
  if(cmd.startsWith("set")){
    int s1 = cmd.indexOf(' ');
    int s2 = cmd.indexOf(' ', s1 + 1);
    if(s1 < 0 || s2 < 0) return;

    String action = cmd.substring(s1 + 1, s2);
    int index = cmd.substring(s2 + 1).toInt() - 1;

    if(index < 0 || index >= HW_BTN_COUNT) return;

    if(action == "on"){
      setSwitch(index, true, source);
    }
    else if(action == "off"){
      setSwitch(index, false, source);
    }
  }
}


//เพิ่ม handler ฝั่ง ESP32
void handleCmd(){
  String cmd = server.arg("cmd");   

  Serial.print("CMD > ");
  Serial.println(cmd);

  if(cmd.length() == 0){
    server.send(400,"text/plain","NO CMD");
    return;
  }

  processCommand(cmd, "WEB");
  server.send(200,"text/plain","OK");
}
//-------------- ตารางอีเวนต์ ---------------
struct AutoEvent {
  uint8_t buttonIndex;      // ปุ่มที่จะเปิด
  unsigned long duration;   // เวลา (ms)
};

//  ปรับเวลาได้ตรงนี้
AutoEvent autoTable[] = {

  {1, 2000}, // Motor HIGH
  {2, 2000}, // Motor MEDLUM
  {3, 2000}, // Motor LOW
  {4, 2000}, // COMP
  {5, 2000}, // Timer
  {6, 2000}, // Delay
  {7, 2000}, // Breaker
};
const uint8_t AUTO_EVENT_COUNT =
  sizeof(autoTable) / sizeof(autoTable[0]);


void updateAutoMode() {
  if (!autoMode) return;

  unsigned long now = millis();

  if (!autoStepStarted) {
    autoStepStarted = true;
    autoTimer = now;

    uint8_t idx = autoTable[autoStep].buttonIndex;

    // ปิดปุ่มอื่น
    for (int i = 1; i <= 8; i++) {
      if (i != idx) setSwitch(i, false, "AUTO");
    }

    setSwitch(idx, true, "AUTO");

    Serial.printf("[AUTO] step %d start\n", autoStep);
  }

  if (now - autoTimer >= autoTable[autoStep].duration) {
    autoStepStarted = false;
    autoStep++;

    if (autoStep >= AUTO_EVENT_COUNT) {
      autoStep = 0;
      autoMode = false;
      Serial.println("[AUTO] finished");
    }
  }
}

//------------------- Commands used with switches. ---------------
//คำสั่งที่ใช้ร่วมกับสวิตช์
void toggleButton(uint8_t i) {
  setSwitch(i, !btnState[i], "BUTTON");
}

//ฟังก์ชันตั้งค่าสวิตช์จากภายนอก
void setSwitch(uint8_t i, bool state, const char* source){
  if(i >= WEB_BTN_COUNT) return;
  if(btnState[i] == state) return;

  if(autoMode && strcmp(source, "AUTO") != 0){
    autoMode = false;
    SerialBridge("[FSM] AUTO cancelled by MANUAL");
  }

  btnState[i] = state;
  setButton(i, state);
}

// อ่านปุ่ม
void readButtons() {
  for (uint8_t i = 0; i < HW_BTN_COUNT; i++) {

    bool current = digitalRead(BTN_PIN[i]); // HIGH / LOW

    // ตรวจเฉพาะตอน "ตำแหน่งสวิตช์เปลี่ยนจริง"
    if (current != lastHWState[i]) {
      //delay(20); // debounce
      current = digitalRead(BTN_PIN[i]);

      if (current != lastHWState[i]) {
        lastHWState[i] = current;

        bool state = (current == LOW); // LOW = ON
        setSwitch(i, state, "SWITCH");
      }
    }
  }
}



// ---------------- LED control commands. ---------------------
//คุมความสว่าง 
void applyBrightness() {
  FastLED.setBrightness(globalBrightness);
}

// ฟังก์ชันนวงเวล่าความสว่าง
void fadeAll_On() { //เปิด
  for (int b = 0; b <= globalBrightness; b += Brightness_delay) {
    FastLED.setBrightness(b);
    FastLED.show();
    //delay(Fadet_time);
  }
}

void fadeAll_Off() { //ปิด
  for (int b = globalBrightness; b >= 0; b -= Brightness_delay) {
    FastLED.setBrightness(b);
    FastLED.show();
    //delay(Fadet_time);
  }
}

void fadeAll_Fadet(int fromB, int toB) { //ค่อยๆเปลียน
  if (fromB == toB) return;
  
  int step = (fromB > toB) ? -Brightness_delay : Brightness_delay;

  for (int b = fromB; b != toB; b += step) {
    FastLED.setBrightness(b);
    FastLED.show();
    //delay(Fadet_time);
  }

  FastLED.setBrightness(toB);
  FastLED.show();
}

// ตั้งค่าสีเริ่มต้นแต่ละช่วง (ศูนย์กลาง)
void setFixedColors() {
  FastLED.clear(); // ตั้งค่า LED ทุกดวงให้เป็น CRGB(0, 0, 0); // ดำ / ดับ
  // ---------- LED LINE 1 ----------
  for (int i = 0; i < 10 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Blue;
  }
  for (int i = 10; i < 16 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Red;
  }
  for (int i = 16; i < 20 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Blue;
  }
 for (int i = 20; i < 23 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB(139,69,0);//DarkOrange4
  }
  for (int i = 23; i < 37 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB(160, 32, 240);//Purple
  }
  for (int i = 37; i < 50 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB(0, 225, 0);//Green1
  }
  for (int i = 50; i < 62 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB(199, 26, 26); //Brown4
  }
  for (int i = 62; i < 73 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Blue;
  }
  // ---------- LED LINE 2 ----------
  for (int i = 0; i < 34 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB( 205, 205, 0); //Yellow3
  }

  // ---------- LED LINE 3 ----------
  for (int i = 0; i < 2 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Blue;
  }
  for (int i = 2; i < 6 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB(139,69,0);//DarkOrange4
  }
  for (int i = 6; i < 23 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Red; 
  }
  for (int i = 23; i < 37 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB(139,69,0);//DarkOrange4
  }
  for (int i = 37; i < 43 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Blue; 
  }
  for (int i = 43; i < 47 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Blue;
  }
  for (int i = 47; i < 61 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Blue; 
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
  //delay(20);
}

//--------------------- The wiring sequence for each device. -----------------
//ลำดับการเดินสายไฟสำหรับแต่ละอุปกรณ์
void p1_Cirecuit_Breaker_ON() {
  // ---------- LED LINE 1-3 ----------
  for (int i = 0; i < 10 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Blue;
  }
  for (int i = 10; i < 16 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Red;
  }
   for (int i = 62; i < 73 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Blue;
  }
  for (int i = 16; i < 20 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Blue;
  }
 for (int i = 20; i < 23 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB(139,69,0);//DarkOrange4
  }
  for (int i = 0; i < 2 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Blue;
  }
  for (int i = 2; i < 6 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB(139,69,0);//DarkOrange4
  }
  for (int i = 23; i < 37 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB(139,69,0);//DarkOrange4
  }
  for (int i = 37; i < 43 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Blue;
  }
  for (int i = 47; i < 64 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Blue;
  }

  FastLED.show(); 
}

void p1_Cirecuit_Breaker_OFF() {
  // ---------- LED LINE 1-3 ----------
  for (int i = 0; i < 10 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Black;
  }
  for (int i = 10; i < 16 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Black;
  }
   for (int i = 61; i < 73 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Black;
  }
  for (int i = 16; i < 23 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Black;
  }
    for (int i = 0; i < 2 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Black;
  }
  for (int i = 37; i < 43 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Black;
  }
  for (int i = 47; i < 64 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Black;
  }
  FastLED.show(); 
}

void p1_Motpr_SWING_ON() {
  // ---------- LED LINE 1 ----------
  for (int i = 62; i < 73 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Blue;
  }
  FastLED.show(); 
}

void p1_Motpr_SWING_OFF() {
  // ---------- LED LINE 1 ----------
  for (int i = 62; i < 73 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Black;
  }
  FastLED.show(); 
}

void p1_Motpr_HIGH_ON() {
  // ---------- LED LINE 1 ----------
  for (int i = 50; i < 62 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB(199, 26, 26); //Brown4
  }
  FastLED.show(); 
}

void p1_Motpr_HIGH_OFF() {
  // ---------- LED LINE 1 ----------
  for (int i = 50; i < 62 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Black;
  }
  FastLED.show(); 
}

void p1_Motpr_MEDLUM_ON() {
  // ---------- LED LINE 1 ----------
  for (int i = 37; i < 50 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB(0, 225, 0);//Green1
  }
  FastLED.show(); 
}

void p1_Motpr_MEDLUM_OFF() {
  // ---------- LED LINE 1 ----------
  for (int i = 37; i < 50 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Black;
  }
  FastLED.show(); 
}

void p1_Motpr_LOW_ON() {
  // ---------- LED LINE 1 ----------
  for (int i = 23; i < 37 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB(160, 32, 240);//Purple
  }
  FastLED.show(); 
}

void p1_Motpr_LOW_OFF() {
  // ---------- LED LINE 1 ----------
  for (int i = 23; i < 37 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Black;
  }
  FastLED.show(); 
}

void p2_Condensing_Unil_0_ON() {
  // ---------- LED LINE 2 ----------
  for (int i = 28; i < 31 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB( 205, 205, 0); //Yellow3
  }
  //----
  for (int i = 0; i < 28 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB::Black;
  }
  for (int i = 31; i < 34 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB::Black;
  }
    for (int i = 43; i < 47 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Black;
  }
  FastLED.show(); 
}

void p2_Condensing_Unil_0_OFF() {
  // ---------- LED LINE 2 ----------
  for (int i = 28; i < 31 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB::Black;
  }    
  FastLED.show(); 
}

void p2_Timer_Relay_1_ON() {
  // ---------- LED LINE 2 
  for (int i = 0; i < 6 && i < NUM_LEDS_1; i++) {
    leds1[i] = CRGB::Blue;
  }
  for (int i = 0; i < 9 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB( 205, 205, 0); //Yellow3
  }
  for (int i = 21; i < 29 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB( 205, 205, 0); //Yellow3
  }
  for (int i = 31; i < 34 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB( 205, 205, 0); //Yellow3
  }
  for (int i = 0; i < 2 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Blue;
  }
  for (int i = 43; i < 47 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Blue;
  }
  for (int i = 56; i < 61 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Blue;
  }
  //-------
  for (int i = 9; i < 21 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB::Black;
  }
  for (int i = 29; i < 31 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB::Black;
  }
  FastLED.show(); 
}

void p2_Timer_Relay_1_OFF() {
  // ---------- LED LINE 2 ----------
  for (int i = 0; i < 9 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB::Black;
  }
  for (int i = 21; i < 29 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB::Black;
  }
  for (int i = 31; i < 34 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB::Black;
  }
  for (int i = 43; i < 47 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Black;
  }
  
  FastLED.show(); 
}

void p2_Delay_on_Make_2_ON() {
  // ---------- LED LINE 2 ----------
  for (int i = 0; i < 13 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB( 205, 205, 0); //Yellow3
  }
  for (int i = 13; i < 29 && i < NUM_LEDS_2; i++) { 
    leds2[i] = CRGB( 205, 205, 0); //Yellow3
  }
  //----
  for (int i = 29; i < 34 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB::Black;
  }
  for (int i = 43; i < 47 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Black;
  } 
  FastLED.show(); 
}

void p2_Delay_on_Make_2_OFF() {
  // ---------- LED LINE 2 ----------
  for (int i = 0; i < 13 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB::Black;
  }
  for (int i = 13; i < 29 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB::Black;
  }
  FastLED.show(); 
}

void p3_Motot_FE_ON() {
  // ---------- LED LINE 3 ----------
  for (int i = 0; i < 2 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Blue;
  }
  for (int i = 2; i < 6 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB(139,69,0);//DarkOrange4
  }
  for (int i = 6; i < 23 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Red; 
  }
  for (int i = 23; i < 37 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB(139,69,0);//DarkOrange4
  }
  for (int i = 37; i < 43 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Blue; 
  }
  for (int i = 47; i < 61 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Blue; 
  }  
  FastLED.show(); 
}
void p3_Motot_FE_OFF() {
  // ---------- LED LINE 3 ----------
  for (int i = 0; i < 2 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Black;
  }
  for (int i = 2; i < 6 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Black;
  }
  for (int i = 6; i < 23 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Black; 
  }
  for (int i = 23; i < 37 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Black;
  }
  for (int i = 37; i < 43 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Black; 
  }
  for (int i = 47; i < 61 && i < NUM_LEDS_3; i++) {
    leds3[i] = CRGB::Black; 
  } 
  FastLED.show(); 
}
void p4_Magnetic_Contactor(bool on) {

  FastLED.show();
}

void ALL_Black_ON() {
  // ---------- LED LINE 2 ----------
  for (int i = 1; i < 119 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB::Black;
  }
  FastLED.show(); 
}
void ALL_Black_OFF() {
  // ---------- LED LINE 2 ----------
  for (int i = 1; i < 1 && i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB::Black;
  }
  FastLED.show(); 
}
//-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-+/-/
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
      //delay(5);
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
      //delay(5);
    }

    leds1[i] = CHSV(160, 255, globalBrightness);
    FastLED.show();
  }
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
void rainbowLoopNonBlocking() {
  static uint32_t lastUpdate = 0;
  static uint8_t baseHue = 0;
  static uint8_t currentBrightness = 0;

  if (millis() - lastUpdate < 20) return;
  lastUpdate = millis();

  // 🔆 Fade in
  if (currentBrightness < globalBrightness) {
    currentBrightness++;
    FastLED.setBrightness(currentBrightness);
  }

  for (int i = 0; i < NUM_LEDS_1; i++)
    leds1[i] = CHSV(baseHue + i * 3, 255, 255);

  for (int i = 0; i < NUM_LEDS_2; i++)
    leds2[i] = CHSV(baseHue + i * 3, 255, 255);

  for (int i = 0; i < NUM_LEDS_3; i++)
    leds3[i] = CHSV(baseHue + i * 3, 255, 255);

  baseHue++;
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
  server.on("/cmd", HTTP_POST, handleCmd);
  server.onNotFound(handleRoot);
  server.begin();

  Serial.println("ESP32 READY");
  Serial.println(WiFi.softAPIP());

  for (uint8_t i = 0; i < HW_BTN_COUNT; i++) {
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
  readButtons();
  checkCommand();

  if(autoMode){
    updateAutoMode();   
  }

  if (rainbowMode && ledPower) {
    rainbowLoopNonBlocking();
  }
}
