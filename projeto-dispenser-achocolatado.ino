#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WebServer.h>

// Estado do sistema
int intensidade = 1;  // 1=fraco, 2=médio, 3=forte
String intensidades[] = { "Fraco", "Médio", "Forte" };

// Display LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

void startLCD() {
  lcd.init();
  lcd.backlight();
}

// Wi-Fi
const char* ssid = "Wifi Gabriel L";
const char* password = "senhateste";

void startWiFi() {
  WiFi.begin(ssid, password);
  lcd.print("Conectando WiFi");
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

// Servidor Web (Frontend)
WebServer server(80);

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
  <h2>Iniciar Processo</h2>
  <div class="slider-container">
    <input type="button" id="iniciar-fraco" value="Fraco">
    <input type="button" id="iniciar-medio" value="Médio">
    <input type="button" id="iniciar-forte" value="Forte">
  </div>

  <script>
    const iniciarFraco = document.getElementById("iniciar-fraco")
    const iniciarMedio = document.getElementById("iniciar-medio")
    const iniciarForte = document.getElementById("iniciar-forte")

    iniciarFraco.onclick = function() {
      fetch("/iniciar?intensidade=1");
    }
  </script>
</body>
</html>
)rawliteral";

// Servidor Web (Backend)
void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleStart() {
  iniciarProcesso();
  if (server.hasArg("intensidade")) {
    intensidade = server.arg("intensidade").toInt();
  }
  server.send(200, "text/plain", "OK");
}

void startServer() {
  server.on("/", handleRoot);
  server.on("/iniciar", handleStart);
  server.begin();
}

// Pinos dos componentes
#define SERVO_COPO 13
#define SENSOR_COPO 23
#define SERVO_CHOCOLATE 25
#define SERVO_LEITE 26
#define SERVO_VALVULA 27
#define MIXER 33
#define BOTAO_CIMA 14
#define BOTAO_OK 12

#define TEMPO_DE_MISTURA 5

// Servo motores
Servo servoCopo, servoChocolate, servoLeite, servoValvula;

void startServoMotores() {
  // Setup motores
  servoCopo.attach(SERVO_COPO);
  servoChocolate.attach(SERVO_CHOCOLATE);
  servoLeite.attach(SERVO_LEITE);
  servoValvula.attach(SERVO_VALVULA);
  // Deixar na posição inicial de 10°
  servoCopo.write(10);
  servoChocolate.write(10);
  servoLeite.write(10);
  servoValvula.write(10);
}

void setup() {
  // Console serial
  Serial.begin(115200);

  // WiFi
  startWiFi();

  // Server
  startServer();

  // Servo motores
  startServoMotores();

  // Mixer
  pinMode(MIXER, OUTPUT);
  digitalWrite(MIXER, LOW);

  // Sensor e botões
  pinMode(SENSOR_COPO, INPUT);
  pinMode(BOTAO_CIMA, INPUT_PULLUP);
  pinMode(BOTAO_OK, INPUT_PULLUP);

  // LCD
  startLCD();

  // Mostrar menu no LCD
  mostrarMenu();
}

void loop() {
  server.handleClient();

  if (digitalRead(BOTAO_CIMA) == LOW) {
    intensidade = (intensidade % 3) + 1;
    delay(200);
    mostrarMenu();
  }

  if (digitalRead(BOTAO_OK) == LOW) {
    iniciarProcesso();
  }
}

void mostrarMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Intensidade:");
  lcd.setCursor(0, 1);
  lcd.print(intensidades[intensidade - 1]);
}

bool esperarCopo() {
  for (int tentativas = 0; tentativas < 3; tentativas++) {
    servoCopo.write(100);
    delay(500);
    servoCopo.write(10);
    delay(1000);
    if (digitalRead(SENSOR_COPO) == LOW) return true;
  }
  return false;
}

void dosarChocolate(int ciclos) {
  for (int i = 0; i < ciclos; i++) {
    servoChocolate.write(100);
    delay(500);
    servoChocolate.write(10);
    delay(500);
  }
}

void dosarLeite() {
  servoLeite.write(100);
  delay(2000);  // tempo fixo de vazão
  servoLeite.write(10);
}

void iniciarProcesso() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Iniciando...");
  delay(1000);

  // Dispensar copo
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Dispensando copo");

  if (!esperarCopo()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Erro!");
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Dose choco...");
  dosarChocolate(intensidade);

  lcd.setCursor(0, 1);
  lcd.print("Dose leite...");
  dosarLeite();

  // Mistura
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Misturando...");
  digitalWrite(MIXER, HIGH);
  for (int i = TEMPO_DE_MISTURA; i > 0; i--) {
    lcd.setCursor(0, 1);
    lcd.print("Tempo: ");
    lcd.print(i);
    lcd.print("s ");
    delay(1000);
  }
  digitalWrite(MIXER, LOW);

  // Dispensar bebida
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Liberando bebida");
  servoValvula.write(100);
  delay(3000);
  servoValvula.write(10);

  // Aguarda retirada do copo
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pronto!");
  while (digitalRead(SENSOR_COPO) == LOW) {
    delay(100);
  }
  mostrarMenu();
}
