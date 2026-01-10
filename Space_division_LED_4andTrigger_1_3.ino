/*************************************************
 * ESP32 Multi-Input ON / OFF Switch
 * - Button (GPIO 12)
 * - Serial command
 *************************************************/

#define BTN_COUNT 5
const uint8_t BTN_PIN[BTN_COUNT] = {27, 26, 25, 33, 32};           // ขาปุ่มกด

bool btnState[BTN_COUNT]   = {false, false, false, false, false};     // สถานะ ON / OFF กลาง
bool lastBtnState[BTN_COUNT] = {false, false, false, false, false};
bool lastBtn[BTN_COUNT]   = {HIGH, HIGH, HIGH, HIGH, HIGH};         // สถานะปุ่มก่อนหน้า

// ---------- ฟังก์ชันกลาง ----------
void toggleButton(uint8_t i) {
  btnState[i] = !btnState[i];

  Serial.print("BTN ");
  Serial.print(i + 1);
  Serial.print(" -> ");
  Serial.println(btnState[i] ? "ON" : "OFF");
}
//----------------ฟังก์ชันตั้งค่าสวิตช์จากภายนอก------
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
// ---------- อ่านปุ่ม ----------
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
//---------------สังเเต่หนึง-----------
void readButtons_on() {
  // ตรวจจับ OFF -> ON ของสวิตช์ตัวที่ 2
  if (lastBtnState[1] == false && btnState[1] == true) {
    Serial.println("hello world");
  }

  // อัปเดตสถานะเดิม
  lastBtnState[1] = btnState[1];
}
// ---------- SETUP ----------
void setup() {
  Serial.begin(9600);

  for (uint8_t i = 0; i < BTN_COUNT; i++) {
    pinMode(BTN_PIN[i], INPUT_PULLUP);
  }
}
//------------------ฟังก์ชันอ่านคำสั่งจาก Serial-------------------
void readSerialCommand() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toLowerCase();

  // รูปแบบคำสั่ง: set on 2 / set off 2
  if (cmd.startsWith("set")) {
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
}
// ---------- LOOP ----------
void loop() {
  readButtons();  // ปุ่มจริง
  readSerialCommand();  // คำสั่งภายนอก
  readButtons_on(); // logic ของคุณ

  // ตัวอย่างการใช้งาน
  if (btnState[0]) {
    // ปุ่มที่ 1 ON
//    Serial.print("FFFFFFF1 /n ");
  }

  if (btnState[1]) {
    // ปุ่มที่ 2 ON
//    Serial.print("FFFFFFF2 /n ");
  }
}
