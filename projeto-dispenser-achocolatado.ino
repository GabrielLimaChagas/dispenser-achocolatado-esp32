#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// const char* ssid = "TP-LINK_6CBAC2";
// const char* password = "99915933";
const char* ssid = "Wifi Gabriel L";
const char* password = "senhateste";

WebServer server(80);
Servo servoMotor;
LiquidCrystal_I2C lcd(0x27, 16, 2); // Endereço comum: 0x27 ou 0x3F

const int pinServo = 13;
const int pinDispenseButton = 14;


int servoPos = 90;

const char index_html[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Controle de Servo</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; padding: 40px; }
    input[type=range] { width: 80%%; }
    .slider-container { margin-bottom: 40px; }
  </style>
</head>
<body>
  <h2>Controle do Servo Motor</h2>
  <div class="slider-container">
    <p>Ângulo: <span id="servoValue">90</span>°</p>
    <input type="range" min="0" max="180" value="90" id="servoSlider">
  </div>

  <h2>Dispensar Copo</h2>
  <div class="slider-container">
    <input type="button" id="dispenseCup" value="Dispensar">
  </div>

  <script>
    const servoSlider = document.getElementById("servoSlider");
    const servoValue = document.getElementById("servoValue");
    const dispenseCup = document.getElementById("dispenseCup")

    servoSlider.oninput = function() {
      servoValue.innerHTML = this.value;
      fetch("/servo?value=" + this.value);
    }

    dispenseCup.onclick = function() {
      fetch("/dispenseCup")
    }
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleServo() {
  if (server.hasArg("value")) {
    servoPos = server.arg("value").toInt();
    servoMotor.write(servoPos);
  }
  server.send(200, "text/plain", "OK");
}

void handleDispenseCup() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Dispensando...");
  for (int i = 70; i < 120; i++) {
    servoPos = i;
    servoMotor.write(servoPos);
    delay(10);
  }
  for (int i = 70; i < 70; i--) {
    servoPos = i;
    servoMotor.write(servoPos);
    delay(10);
  }
  servoPos = 70;
  servoMotor.write(servoPos);
  server.send(200, "text/plain", "OK");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Copo pronto!");
  delay(1500);
  updateLCD();
}

void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Servo: ");
  lcd.print(servoPos);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);

  pinMode(pinDispenseButton, INPUT);

  servoMotor.setPeriodHertz(50);
  servoMotor.attach(pinServo, 500, 2400);
  servoMotor.write(servoPos);

  WiFi.begin(ssid, password);
  lcd.print("Conectando WiFi");
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  delay(2000);
  updateLCD();

  Serial.println("\nConectado ao Wi-Fi");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/servo", handleServo);
  server.on("/dispenseCup", handleDispenseCup);
  server.begin();
}

void loop() {
  server.handleClient();
  if (digitalRead(pinDispenseButton) == LOW) {
    handleDispenseCup();
  }
}