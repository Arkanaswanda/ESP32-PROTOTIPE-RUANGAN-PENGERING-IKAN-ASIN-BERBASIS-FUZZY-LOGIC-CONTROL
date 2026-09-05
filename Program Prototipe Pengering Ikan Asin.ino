#include "DHT.h"
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Fuzzy.h>
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "MaggotBalap";     
const char* pass = "12121212"; 

WebServer server(80); 

#define DHTPIN 4          
#define DHTTYPE DHT22     
#define RAIN_PIN 34       

const int pinKipas_IN1 = 25; 
const int pinKipas_IN2 = 14; 
const int pinLED_IN3   = 26; 
const int pinLED_IN4   = 27; 
const int pinServo     = 13; 

DHT dht(DHTPIN, DHTTYPE);
Servo servoAtap; 
LiquidCrystal_I2C lcd(0x27, 16, 2); 
Fuzzy *fuzzy = new Fuzzy();

String statusCuaca = "Cerah";
String statusAtapLcd = "OPEN";
float globalSuhu = 0;
float globalLembab = 0;
int globalRawHujan = 4095;

int modeSistem = 0;         
int servoManualWeb = 175;   

unsigned long waktuTerakhirServo = 0;
const int jedaServoPelan = 15; 
unsigned long waktuTerakhirSensor = 0;
const int jedaBacaSensor = 1000; 

int sudutServoTujuan = 175;   
int sudutServoSekarang = 175; 

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<link rel='stylesheet' href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css'>";
  html += "<title>PENGERING IKAN ASIN</title>";
  html += "<style>body{background:#f4f6f9; font-family:sans-serif;} .card{box-shadow:0 4px 8px rgba(0,0,0,0.05); border:none; margin-bottom:20px;}</style>";
  html += "</head><body>";
  
  html += "<div class='container mt-4'>";
  html += "  <div class='bg-primary text-white p-4 rounded card text-center'>";
  html += "    <h2>KONTROL CERDAS DAN AKUISISI DATA</h2>";
  html += "    <p class='mb-0'>Dashboard Sistem Cerdas Prototipe Pengering Ikan Asin</p>";
  html += "  </div>";
  
  html += "  <div class='row'>";
  html += "    <div class='col-md-4'><div class='card p-4 text-center'><h5>Suhu Udara</h5><h2 class='text-danger' id='suhu'>0.0</h2><p>&deg;C</p></div></div>";
  html += "    <div class='col-md-4'><div class='card p-4 text-center'><h5>Kelembaban</h5><h2 class='text-primary' id='lembab'>0.0</h2><p>%</p></div></div>";
  html += "    <div class='col-md-4'><div class='card p-4 text-center'><h5>Kondisi Cuaca</h5><h2 class='text-warning' id='cuaca'>-</h2><p id='rawHujan' class='text-muted'>(ADC: -)</p></div></div>";
  html += "  </div>";

  html += "  <div class='row'>";
  html += "    <div class='col-md-6'>";
  html += "      <div class='card p-4'>";
  html += "        <h5>Mode Sistem Kendali</h5>";
  html += "        <div class='d-flex gap-2 mt-3'>";
  html += "          <button class='btn btn-success w-50' onclick='setMode(0)'>Mode Otomatis (Fuzzy)</button>";
  html += "          <button class='btn btn-secondary w-50' onclick='setMode(1)'>Mode Manual (Web)</button>";
  html += "        </div>";
  html += "        <h6 class='mt-3'>Status Aktif: <span class='badge bg-info text-dark' id='statusMode'>Otomatis (Fuzzy)</span></h6>";
  html += "      </div>";
  html += "    </div>";
  html += "    <div class='col-md-6'>";
  html += "      <div class='card p-4'>";
  html += "        <h5>Kontrol Manual Atap (Servo)</h5>";
  html += "        <input type='range' class='form-range mt-3' min='45' max='175' id='sliderServo' onchange='kirimServo(this.value)'>";
  html += "        <p class='mt-2'>Sudut Atap: <span id='valServo'>175</span>&deg; (<span id='txtAtap'>OPEN</span>)</p>";
  html += "      </div>";
  html += "    </div>";
  html += "  </div>";
  html += "</div>";

  html += "<script>";
  html += "setInterval(ambilData, 1000);";
  html += "function ambilData() {";
  html += "  fetch('/readData').then(response => response.json()).then(data => {";
  html += "    document.getElementById('suhu').innerText = data.suhu;";
  html += "    document.getElementById('lembab').innerText = data.lembab;";
  html += "    document.getElementById('cuaca').innerText = data.cuaca;";
  html += "    document.getElementById('rawHujan').innerText = '(ADC: ' + data.rawRain + ')';";
  html += "    document.getElementById('statusMode').innerText = data.mode == 0 ? 'Otomatis (Fuzzy)' : 'Manual (Web)';";
  html += "    if(data.mode == 0) {";
  html += "      document.getElementById('sliderServo').value = data.targetServo;";
  html += "      document.getElementById('valServo').innerText = data.targetServo;";
  html += "    }";
  html += "    document.getElementById('txtAtap').innerText = data.statusAtap;";
  html += "  });";
  html += "}";
  html += "function setMode(m) { fetch('/setMode?val=' + m); }";
  html += "function kirimServo(s) { document.getElementById('valServo').innerText = s; fetch('/setServo?val=' + s); }";
  html += "</script>";
  
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleReadData() {
  String json = "{";
  json += "\"suhu\":" + String(globalSuhu, 1) + ",";
  json += "\"lembab\":" + String(globalLembab, 1) + ",";
  json += "\"rawRain\":" + String(globalRawHujan) + ",";
  json += "\"cuaca\":\"" + statusCuaca + "\",";
  json += "\"mode\":" + String(modeSistem) + ",";
  json += "\"targetServo\":" + String(sudutServoTujuan) + ",";
  json += "\"statusAtap\":\"" + statusAtapLcd + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleSetMode() {
  if (server.hasArg("val")) {
    modeSistem = server.arg("val").toInt();
  }
  server.send(200, "text/plain", "OK");
}

void handleSetServo() {
  if (server.hasArg("val")) {
    servoManualWeb = server.arg("val").toInt();
  }
  server.send(200, "text/plain", "OK");
}

void setupFuzzy() {
  FuzzyInput *suhu = new FuzzyInput(1);
  FuzzySet *dingin = new FuzzySet(0, 0, 25, 28); 
  FuzzySet *normal = new FuzzySet(25, 28, 32, 35); 
  FuzzySet *panas = new FuzzySet(32, 35, 50, 50);    
  suhu->addFuzzySet(dingin); suhu->addFuzzySet(normal); suhu->addFuzzySet(panas); 
  fuzzy->addFuzzyInput(suhu);

  FuzzyInput *kelembaban = new FuzzyInput(2);
  FuzzySet *kering = new FuzzySet(0, 0, 40, 50); 
  FuzzySet *sedang = new FuzzySet(40, 50, 65, 75); 
  FuzzySet *lembab = new FuzzySet(65, 75, 100, 100);  
  kelembaban->addFuzzySet(kering); kelembaban->addFuzzySet(sedang); kelembaban->addFuzzySet(lembab); 
  fuzzy->addFuzzyInput(kelembaban);

  FuzzyInput *hujan = new FuzzyInput(3);
  FuzzySet *hujanKering = new FuzzySet(0, 0, 20, 40); 
  FuzzySet *hujanMendung = new FuzzySet(25, 45, 55, 75); 
  FuzzySet *hujanBasah = new FuzzySet(60, 80, 100, 100); 
  hujan->addFuzzySet(hujanKering); hujan->addFuzzySet(hujanMendung); hujan->addFuzzySet(hujanBasah); 
  fuzzy->addFuzzyInput(hujan);

  FuzzyOutput *kipas = new FuzzyOutput(1);
  FuzzySet *kipasMati = new FuzzySet(0, 0, 0, 0); 
  FuzzySet *kipasSedang = new FuzzySet(10, 40, 40, 70); 
  FuzzySet *kipasCepat = new FuzzySet(60, 100, 100, 100);
  kipas->addFuzzySet(kipasMati); kipas->addFuzzySet(kipasSedang); kipas->addFuzzySet(kipasCepat); 
  fuzzy->addFuzzyOutput(kipas);

  FuzzyOutput *led = new FuzzyOutput(2);
  FuzzySet *ledMati = new FuzzySet(0, 0, 0, 0); 
  FuzzySet *ledSedang = new FuzzySet(10, 40, 40, 70); 
  FuzzySet *ledTerang = new FuzzySet(60, 100, 100, 100);
  led->addFuzzySet(ledMati); led->addFuzzySet(ledSedang); led->addFuzzySet(ledTerang); 
  fuzzy->addFuzzyOutput(led);

  FuzzyOutput *servoOut = new FuzzyOutput(3);
  FuzzySet *atapTutup = new FuzzySet(45, 45, 45, 45); 
  FuzzySet *atapSetengah = new FuzzySet(70, 105, 105, 135); 
  FuzzySet *atapBuka = new FuzzySet(150, 175, 175, 175);   
  servoOut->addFuzzySet(atapTutup); servoOut->addFuzzySet(atapSetengah); servoOut->addFuzzySet(atapBuka); 
  fuzzy->addFuzzyOutput(servoOut);

  createRule(1, dingin, kering, hujanKering, kipasSedang, ledSedang, atapBuka);
  createRule(2, dingin, sedang, hujanKering, kipasSedang, ledTerang, atapBuka);
  createRule(3, dingin, lembab, hujanKering, kipasCepat,  ledTerang, atapBuka);
  createRule(4, normal, kering, hujanKering, kipasMati,   ledMati,   atapBuka);
  createRule(5, normal, sedang, hujanKering, kipasSedang, ledSedang, atapBuka);
  createRule(6, normal, lembab, hujanKering, kipasCepat,  ledTerang, atapBuka);
  createRule(7, panas,  kering, hujanKering, kipasMati,   ledMati,   atapBuka);
  createRule(8, panas,  sedang, hujanKering, kipasSedang, ledMati,   atapBuka);
  createRule(9, panas,  lembab, hujanKering, kipasCepat,  ledMati,   atapBuka);

  createRule(10, dingin, kering, hujanMendung, kipasSedang, ledTerang, atapSetengah);
  createRule(11, dingin, sedang, hujanMendung, kipasSedang, ledTerang, atapSetengah);
  createRule(12, dingin, lembab, hujanMendung, kipasCepat,  ledTerang, atapSetengah);
  createRule(13, normal, kering, hujanMendung, kipasSedang, ledSedang, atapSetengah);
  createRule(14, normal, sedang, hujanMendung, kipasSedang, ledTerang, atapSetengah);
  createRule(15, normal, lembab, hujanMendung, kipasCepat,  ledTerang, atapSetengah);
  createRule(16, panas,  kering, hujanMendung, kipasSedang, ledSedang, atapSetengah);
  createRule(17, panas,  sedang, hujanMendung, kipasSedang, ledSedang, atapSetengah);
  createRule(18, panas,  lembab, hujanMendung, kipasCepat,  ledSedang, atapSetengah);

  createRule(19, dingin, kering, hujanBasah, kipasSedang, ledTerang, atapTutup);
  createRule(20, dingin, sedang, hujanBasah, kipasSedang, ledTerang, atapTutup);
  createRule(21, dingin, lembab, hujanBasah, kipasCepat,  ledTerang, atapTutup);
  createRule(22, normal, kering, hujanBasah, kipasSedang, ledTerang, atapTutup);
  createRule(23, normal, sedang, hujanBasah, kipasSedang, ledTerang, atapTutup);
  createRule(24, normal, lembab, hujanBasah, kipasCepat,  ledTerang, atapTutup);
  createRule(25, panas,  kering, hujanBasah, kipasCepat,  ledTerang, atapTutup); 
  createRule(26, panas,  sedang, hujanBasah, kipasCepat,  ledTerang, atapTutup); 
  createRule(27, panas,  lembab, hujanBasah, kipasCepat,  ledTerang, atapTutup); 
}

void createRule(int id, FuzzySet *inSuhu, FuzzySet *inLembab, FuzzySet *inHujan, FuzzySet *outKipas, FuzzySet *outLED, FuzzySet *outServo) {
  FuzzyRuleAntecedent *suhuAndLembab = new FuzzyRuleAntecedent(); 
  suhuAndLembab->joinWithAND(inSuhu, inLembab);
  FuzzyRuleAntecedent *ifCondition = new FuzzyRuleAntecedent(); 
  ifCondition->joinWithAND(suhuAndLembab, inHujan); 
  FuzzyRuleConsequent *thenCondition = new FuzzyRuleConsequent(); 
  thenCondition->addOutput(outKipas); thenCondition->addOutput(outLED); thenCondition->addOutput(outServo);
  FuzzyRule *rule = new FuzzyRule(id, ifCondition, thenCondition); 
  fuzzy->addFuzzyRule(rule);
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  lcd.init(); lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("Connecting WiFi");

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("WiFi Connected!");
  lcd.setCursor(0, 1); lcd.print(WiFi.localIP());
  Serial.print("IP Web Dashboard Anda: "); Serial.println(WiFi.localIP());
  delay(3000); lcd.clear();

  server.on("/", handleRoot);
  server.on("/readData", handleReadData);
  server.on("/setMode", handleSetMode);
  server.on("/setServo", handleSetServo);
  server.begin();

  ESP32PWM::allocateTimer(0);
  servoAtap.setPeriodHertz(50); servoAtap.attach(pinServo, 500, 2400); 

  pinMode(pinKipas_IN1, OUTPUT); pinMode(pinKipas_IN2, OUTPUT);
  pinMode(pinLED_IN3, OUTPUT); pinMode(pinLED_IN4, OUTPUT);
  
  setupFuzzy();
  servoAtap.write(sudutServoSekarang); 
}

void loop() {
  server.handleClient(); 

  if (millis() - waktuTerakhirServo >= jedaServoPelan) {
    waktuTerakhirServo = millis();
    if (sudutServoSekarang < sudutServoTujuan) { sudutServoSekarang++; servoAtap.write(sudutServoSekarang); } 
    else if (sudutServoSekarang > sudutServoTujuan) { sudutServoSekarang--; servoAtap.write(sudutServoSekarang); }
  }

  if (millis() - waktuTerakhirSensor >= jedaBacaSensor) {
    waktuTerakhirSensor = millis();

    globalSuhu = dht.readTemperature();
    globalLembab = dht.readHumidity();
    globalRawHujan = analogRead(RAIN_PIN); 

    if (isnan(globalSuhu) || isnan(globalLembab)) return;

    int rawHujanFiltered = globalRawHujan;
    if (globalRawHujan > 3650) rawHujanFiltered = 4095; 
    else if (globalRawHujan < 2450) rawHujanFiltered = 0;    

    int inputHujanFuzzy = map(rawHujanFiltered, 4095, 0, 0, 100);
    if (inputHujanFuzzy < 0) inputHujanFuzzy = 0; if (inputHujanFuzzy > 100) inputHujanFuzzy = 100;

    if (globalRawHujan <= 2500) statusCuaca = "Hujan";
    else if (globalRawHujan > 2500 && globalRawHujan < 3600) statusCuaca = "Mendung";
    else statusCuaca = "Cerah";

    fuzzy->setInput(1, globalSuhu); fuzzy->setInput(2, globalLembab); fuzzy->setInput(3, inputHujanFuzzy); 
    fuzzy->fuzzify();                

    int outFuzzyKipas = fuzzy->defuzzify(1); int outFuzzyLED = fuzzy->defuzzify(2); int outFuzzyServo = fuzzy->defuzzify(3); 

    if (modeSistem == 0) {
      sudutServoTujuan = outFuzzyServo; 
    } else {
      sudutServoTujuan = servoManualWeb; 
    }

    int pwmKipasAktif = (outFuzzyKipas > 0) ? map(outFuzzyKipas, 1, 100, 65, 160) : 0;
    int pwmLEDAktif = (outFuzzyLED > 0) ? map(outFuzzyLED, 1, 100, 15, 180) : 0;
    if (pwmLEDAktif > 0 && pwmLEDAktif < 40) pwmLEDAktif = 40; 

    analogWrite(pinKipas_IN1, pwmKipasAktif); analogWrite(pinKipas_IN2, 0); 
    analogWrite(pinLED_IN3, pwmLEDAktif); analogWrite(pinLED_IN4, 0); 

    lcd.setCursor(0, 0); lcd.print("T:"); lcd.print(globalSuhu, 1); lcd.print("C ");
    lcd.setCursor(8, 0); lcd.print("H:"); lcd.print(globalLembab, 1); lcd.print("% ");
    lcd.setCursor(0, 1);
    if (modeSistem == 1) lcd.print("M:MANUAL "); else { lcd.print("W:"); lcd.print(statusCuaca); lcd.print("  "); }
    
    lcd.setCursor(9, 1);
    if(sudutServoTujuan >= 140) { lcd.print("A:OPEN "); statusAtapLcd = "OPEN"; }
    else if(sudutServoTujuan > 60 && sudutServoTujuan < 140) { lcd.print("A:MID  "); statusAtapLcd = "MID"; }
    else { lcd.print("A:CLOSE"); statusAtapLcd = "CLOSE"; }
  }
}