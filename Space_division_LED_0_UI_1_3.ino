#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

//---------------- WiFi AP ----------------
const char* ssid = "ESP32_CONTROL";
const char* password = "12345678";

DNSServer dnsServer;
WebServer server(80);

//---------------- Button -----------------
const char* buttonName[9] = {
  "ZONE A","ZONE B","ZONE C",
  "RAINBOW","RED","GREEN","BLUE",
  "ALL OFF","RESET"
}; 
bool sw[9] = {0};

//---------------- Serial Log -------------
String serialLog = "";

//----------------------------------------
void SerialBridge(String msg){
  Serial.println(msg);
  serialLog += msg + "\n";

  // จำกัดขนาด log กัน RAM เต็ม
  if(serialLog.length() > 4000){
    serialLog.remove(0, serialLog.length() - 3000);
  }
}

//----------------------------------------
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
//----------ให้ “เว็บรู้” เมื่อบอร์ดถูกสั่งจากภายนอก สร้าง API ส่งสถานะทั้งหมด--------------
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
//----------------สร้างฟังก์ชันควบคุมปุ่ม “ศูนย์กลาง”-------------------------
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
      if(state) SerialBridge("ZONE A ENABLE");
      else      SerialBridge("ZONE A DISABLE");
      break;

    case 1:
      if(state) SerialBridge("ZONE B ENABLE");
      else      SerialBridge("ZONE B DISABLE");
      break;

    case 2:
      if(state) SerialBridge("ZONE C ENABLE");
      else      SerialBridge("ZONE C DISABLE");
      break;

    case 3:
      if(state) SerialBridge("RAINBOW MODE ON");
      else      SerialBridge("RAINBOW MODE OFF");
      break;
  }
}
//----------------------------------------
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


//----------------------------------------
void setup(){
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
}

void loop(){
  dnsServer.processNextRequest();
  server.handleClient();

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

