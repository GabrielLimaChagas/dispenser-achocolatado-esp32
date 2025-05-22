#include <LittleFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>

const char* ssid = "Wifi Gabriel L";
const char* password = "senhateste";

WebServer server(80);

Servo servoLeite;
Servo servoChoco;
Servo servoCopo;

const int pinServoLeite = 13;
const int pinServoChoco = 12;
const int pinServoCopo = 14;

// Estoque máximo
int estoqueLeite = 1000;
int estoqueChoco = 750;
int estoqueCopos = 20;

// Variáveis dinâmicas
int usadoLeite = 0;
int usadoChoco = 0;
int coposRestantes = 20;
int totalServido = 0;

const char* dadosPath = "/data.json";

void salvarDados() {
  StaticJsonDocument<256> doc;
  doc["usadoLeite"] = usadoLeite;
  doc["usadoChoco"] = usadoChoco;
  doc["coposRestantes"] = coposRestantes;
  doc["totalServido"] = totalServido;

  File file = LittleFS.open(dadosPath, "w");
  if (!file) {
    Serial.println("Erro ao salvar dados no LittleFS");
    return;
  }
  serializeJson(doc, file);
  file.close();
}

void carregarDados() {
  if (!LittleFS.exists(dadosPath)) {
    salvarDados(); // cria o arquivo com dados zerados
    return;
  }

  File file = LittleFS.open(dadosPath, "r");
  if (!file) {
    Serial.println("Erro ao abrir data.json");
    return;
  }

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println("Erro ao ler JSON");
    file.close();
    return;
  }

  usadoLeite = doc["usadoLeite"] | 0;
  usadoChoco = doc["usadoChoco"] | 0;
  coposRestantes = doc["coposRestantes"] | 20;
  totalServido = doc["totalServido"] | 0;

  file.close();
}

void handleStatus() {
  String json = "{";
  json += "\"leite\":" + String(estoqueLeite - usadoLeite) + ",";
  json += "\"choco\":" + String(estoqueChoco - usadoChoco) + ",";
  json += "\"copos\":" + String(coposRestantes) + ",";
  json += "\"leiteUsado\":" + String(usadoLeite) + ",";
  json += "\"chocoUsado\":" + String(usadoChoco) + ",";
  json += "\"total\":" + String(totalServido);
  json += "}";
  server.send(200, "application/json", json);
}

void acionarServo(Servo& servo, int tempo) {
  servo.write(90);
  delay(tempo);
  servo.write(0);
  delay(500);
}

void handleDispense() {
  if (!server.hasArg("volume") || !server.hasArg("intensity")) {
    server.send(400, "text/plain", "Faltando parametros");
    return;
  }

  int volume = server.arg("volume").toInt();
  int intensidade = server.arg("intensity").toInt();

  int leiteNecessario = volume - intensidade;
  int chocoNecessario = intensidade;

  if (estoqueLeite - usadoLeite < leiteNecessario ||
      estoqueChoco - usadoChoco < chocoNecessario ||
      coposRestantes <= 0) {
    server.send(400, "text/plain", "Estoque insuficiente");
    return;
  }

  acionarServo(servoCopo, 600);
  delay(500);
  acionarServo(servoLeite, map(leiteNecessario, 0, 300, 300, 1000));
  delay(500);
  acionarServo(servoChoco, map(chocoNecessario, 0, 50, 200, 800));

  usadoLeite += leiteNecessario;
  usadoChoco += chocoNecessario;
  coposRestantes--;
  totalServido++;

  salvarDados();
  server.send(200, "text/plain", "Dispensado");
}

void handleRoot() {
  File file = LittleFS.open("/index.html", "r");
  if (!file) {
    server.send(500, "text/plain", "Erro ao carregar index.html");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}

void handleRefill() {
  usadoLeite = 0;
  usadoChoco = 0;
  coposRestantes = estoqueCopos;
  salvarDados();
  server.send(200, "text/plain", "Estoque recarregado");
}

void handleReset() {
  totalServido = 0;
  usadoLeite = 0;
  usadoChoco = 0;
  salvarDados();
  server.send(200, "text/plain", "Estatísticas resetadas");
}

void setup() {
  Serial.begin(115200);

  if (!LittleFS.begin()) {
    Serial.println("Erro ao montar LittleFS");
    return;
  }

  carregarDados();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.println(WiFi.localIP());

  servoLeite.setPeriodHertz(50);
  servoLeite.attach(pinServoLeite, 500, 2400);
  servoChoco.setPeriodHertz(50);
  servoChoco.attach(pinServoChoco, 500, 2400);
  servoCopo.setPeriodHertz(50);
  servoCopo.attach(pinServoCopo, 500, 2400);

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/dispense", handleDispense);
  server.on("/refill", handleRefill);
  server.on("/reset", handleReset);

  server.begin();
}

void loop() {
  server.handleClient();
}
