#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include "FS.h"
#include "SPIFFS.h"

// Estado do sistema
int intensidade = 1;  // 1=fraco, 2=médio, 3=forte
String intensidades[] = { "Fraco", "Médio", "Forte" };

int nivelLeite = 750;
int nivelChoco = 500;
int nivelCopos = 15;

struct Preparacao {
  time_t timestamp;
  int intensidade;
  int nivelLeiteUsado;
  int nivelChocoUsado;
};

#define MAX_HISTORICO 100  // Número máximo de registros a armazenar
Preparacao historico[MAX_HISTORICO];
int historicoCount = 0;

// Display LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

void startLCD() {
  lcd.init();
  lcd.backlight();
}

// Wi-Fi
const char* ssid = "WiFi Notebook";
const char* password = "senhateste";

void startWiFi() {
  WiFi.begin(ssid, password);
  lcd.print("Conectando WiFi");
  Serial.print("Conectando ao Wi-Fi");

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    tentativas++;
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    lcd.clear();
    lcd.print("WiFi falhou!");
    Serial.println("\nFalha na conexão WiFi.");
    return;
  }

  Serial.println("\nWiFi conectado!");
  lcd.clear();
  lcd.print("WiFi conectado!");
}


//variaveis para mostrar o contador de processos
int contadorProcessos = 0;
String ultimaDataEHora = "Nunca Iniciado";

// Servidor Web (Frontend)
WebServer server(80);

const char index_html[] = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Máquina de Achocolatado</title>
  <style>
    body {
      margin: 0;
      font-family: 'Segoe UI', sans-serif;
      background: #f2efe8;
      color: #333;
    }
    header {
      background: #6b4b3e;
      color: #fff;
      padding: 20px;
      text-align: center;
    }
    .container {
      max-width: 600px;
      margin: auto;
      padding: 20px;
    }
    .card {
      background: #fff;
      border-radius: 15px;
      padding: 20px;
      box-shadow: 0 3px 10px rgba(0,0,0,0.1);
      margin-bottom: 20px;
    }
    .card h3 {
      margin-top: 0;
      color: #6b4b3e;
    }
    .status {
      font-size: 1.1em;
      margin: 8px 0;
    }
    .option-group {
      display: flex;
      justify-content: space-around;
      margin-top: 10px;
    }
    .option {
      padding: 10px 20px;
      border: 2px solid #ccc;
      border-radius: 10px;
      cursor: pointer;
    }
    .option.selected {
      border-color: #6b4b3e;
      background-color: #f1e9e2;
      font-weight: bold;
    }
    input[type=number] {
      width: 70px;
      padding: 5px;
      border-radius: 5px;
      border: 1px solid #999;
    }
    button {
      padding: 12px 20px;
      background: #6b4b3e;
      color: #fff;
      border: none;
      border-radius: 10px;
      font-size: 1em;
      cursor: pointer;
    }
    button:hover {
      background: #5b3e34;
    }
    .flex {
      display: flex;
      justify-content: space-between;
      align-items: center;
      flex-wrap: wrap;
      gap: 10px;
    }
    .button-center {
      width: 100%;
      display: flex;
      justify-content: center;
      margin-top: 15px;
    }
  </style>
</head>
<body>
  <header>
    <h1>Máquina de Achocolatado</h1>
  </header>

  <div class="container">
    <div class="card">
      <h3>Histórico</h3>
      <p class="status">Visualize o histórico de preparações.</p>
      <div class="button-center">
        <button onclick="window.location.href='/historico'">Ver Histórico</button>
      </div>
    </div>
    <div class="card">
        <h3>Status do Sistema</h3>
        <p class="status" id="statusInfo">Carregando...</p>
    </div>
    <div class="card">
      <h3>Níveis Manuais</h3>
      <div class="flex">
        <div>Leite (ml): <input type="number" id="leite" value="750"></div>
        <div>Choco (g): <input type="number" id="choco" value="500"></div>
        <div>Copos: <input type="number" id="copos" value="15"></div>
        <div class="button-center">
          <button onclick="atualizarNiveis()">Atualizar</button>
        </div>
      </div>
    </div>

    <div class="card">
      <h3>Intensidade</h3>
      <div class="option-group">
        <div class="option" onclick="setIntensidade(1)">Fraco</div>
        <div class="option selected" onclick="setIntensidade(2)">Médio</div>
        <div class="option" onclick="setIntensidade(3)">Forte</div>
      </div>
    </div>

    <div class="card">
      <h3>Iniciar Preparo</h3>
      <p class="status">Clique no botão abaixo para iniciar o processo de achocolatado.</p>
      <div class="button-center">
        <button onclick="iniciar()">Iniciar</button>
      </div>
    </div>
  </div>

  <script>
    let intensidade = 2;
    function setIntensidade(nivel) {
      intensidade = nivel;
      document.querySelectorAll('.option').forEach(el => el.classList.remove('selected'));
      document.querySelectorAll('.option')[nivel - 1].classList.add('selected');
    }
    function atualizarNiveis() {
      const leite = document.getElementById("leite").value;
      const choco = document.getElementById("choco").value;
      const copos = document.getElementById("copos").value;
      fetch(`/setNiveis?leite=${leite}&choco=${choco}&copos=${copos}`)
        .then(res => res.text())
        .then(msg => alert(msg));
    }
    function atualizarStatus() {
    fetch("/status")
        .then(res => res.text())
        .then(info => {
        document.getElementById("statusInfo").innerText = info;
        });
    }

    function iniciar() {
        fetch(`/iniciar?intensidade=${intensidade}`)
            .then(res => res.text())
            .then(msg => {
                alert("Processo iniciado");
                atualizarStatus(); // Atualiza status após o início
    });
}


    // Atualiza o status quando a página carregar
    window.onload = () => {
        atualizarStatus();
    };

  </script>
</body>
</html>
)rawliteral";

// Servidor Web (Backend)
void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleStart() {
  if (server.hasArg("intensidade")) {
    intensidade = server.arg("intensidade").toInt();
  }
  iniciarProcesso();  // Só inicia aqui
  server.send(200, "text/plain", "Processo iniciado");
}

void handleSetNiveis() {
  if (server.hasArg("leite") && server.arg("leite").toInt() >= 0)
    nivelLeite = server.arg("leite").toInt();
  if (server.hasArg("choco") && server.arg("choco").toInt() >= 0)
    nivelChoco = server.arg("choco").toInt();
  if (server.hasArg("copos") && server.arg("copos").toInt() >= 0)
    nivelCopos = server.arg("copos").toInt();

  String msg = "Níveis atualizados:\n";
  msg += "Leite: " + String(nivelLeite) + "ml\n";
  msg += "Choco: " + String(nivelChoco) + "g\n";
  msg += "Copos: " + String(nivelCopos);
  server.send(200, "text/plain", msg);
}

void inicializarSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("Falha ao montar o sistema de arquivos SPIFFS");
    return;
  }
  Serial.println("Sistema de arquivos SPIFFS montado com sucesso");
}

void salvarHistorico() {
  File file = SPIFFS.open("/historico.dat", "w");
  if (!file) {
    Serial.println("Falha ao abrir arquivo para escrita");
    return;
  }
  
  file.write((uint8_t*)&historicoCount, sizeof(historicoCount));
  file.write((uint8_t*)historico, sizeof(Preparacao) * historicoCount);
  
  file.close();
  Serial.println("Histórico salvo com sucesso");
}

void carregarHistorico() {
  if (!SPIFFS.exists("/historico.dat")) {
    Serial.println("Arquivo de histórico não encontrado");
    return;
  }
  
  File file = SPIFFS.open("/historico.dat", "r");
  if (!file) {
    Serial.println("Falha ao abrir arquivo para leitura");
    return;
  }
  
  file.read((uint8_t*)&historicoCount, sizeof(historicoCount));
  file.read((uint8_t*)historico, sizeof(Preparacao) * historicoCount);
  
  file.close();
  Serial.println("Histórico carregado com sucesso");
}

void adicionarAoHistorico(int intens, int leiteUsado, int chocoUsado) {
  if (historicoCount >= MAX_HISTORICO) {
    // Desloca os registros mais antigos para abrir espaço
    for (int i = 1; i < MAX_HISTORICO; i++) {
      historico[i-1] = historico[i];
    }
    historicoCount = MAX_HISTORICO - 1;
  }
  
  historico[historicoCount].timestamp = time(nullptr);
  historico[historicoCount].intensidade = intens;
  historico[historicoCount].nivelLeiteUsado = leiteUsado;
  historico[historicoCount].nivelChocoUsado = chocoUsado;
  
  historicoCount++;
  salvarHistorico();
}

void handleHistorico() {
  String html = "<html><head><title>Histórico de Preparações</title>";
  html += "<style>body {font-family: Arial; margin: 20px;}";
  html += "table {border-collapse: collapse; width: 100%;}";
  html += "th, td {border: 1px solid #ddd; padding: 8px; text-align: left;}";
  html += "tr:nth-child(even) {background-color: #f2f2f2;}</style></head>";
  html += "<body><h1>Histórico de Preparações</h1>";
  html += "<table><tr><th>Data/Hora</th><th>Intensidade</th><th>Leite (ml)</th><th>Choco (g)</th></tr>";

  for (int i = 0; i < historicoCount; i++) {
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%d/%m/%Y %H:%M", localtime(&historico[i].timestamp));
    
    html += "<tr>";
    html += "<td>" + String(timeStr) + "</td>";
    html += "<td>" + intensidades[historico[i].intensidade - 1] + "</td>";
    html += "<td>" + String(historico[i].nivelLeiteUsado) + "</td>";
    html += "<td>" + String(historico[i].nivelChocoUsado) + "</td>";
    html += "</tr>";
  }

  html += "</table></body></html>";
  server.send(200, "text/html", html);
}

void handleStatus() {
  String resposta = "Processos: " + String(contadorProcessos) + "\n";
  resposta += "Último: " + ultimaDataEHora;
  server.send(200, "text/plain", resposta);
}

void startServer() {
  server.on("/", handleRoot);
  server.on("/iniciar", handleStart);
  server.on("/setNiveis", handleSetNiveis);
  server.on("/status", handleStatus);
  server.on("/historico", handleHistorico);
  server.begin();
}

// Pinos dos componentes
#define SERVO_COPO 13
#define SENSOR_COPO 23
#define SERVO_LEITE 26
#define SERVO_VALVULA 27
#define RELE_CHOCOLATE 25
#define RELE_MIXER 33
#define BOTAO_CIMA 14
#define BOTAO_OK 12

// Configurações Básicas
#define TEMPO_DE_MISTURA 10

// Configuração Avançadas
#define ANGULO_INICIAL_VALVULA 45
#define ANGULO_FINAL_VALVULA 140
#define TEMPO_VALVULA 15 * 1000
#define ANGULO_INICIAL_LEITE 135
#define ANGULO_FINAL_LEITE 45
#define TEMPO_LEITE 10 * 1000
#define ANGULO_INICIAL_COPO 45
#define ANGULO_FINAL_COPO 110
#define INTERVALO_COPO 5 * 1000

// Servo motores
Servo servoCopo, servoLeite, servoValvula;

void startServoMotores() {
  // Setup motores
  servoCopo.attach(SERVO_COPO);
  servoLeite.attach(SERVO_LEITE);
  servoValvula.attach(SERVO_VALVULA);
  // Deixar na posição inicial
  servoCopo.write(ANGULO_INICIAL_COPO);
  servoLeite.write(ANGULO_INICIAL_LEITE);
  servoValvula.write(ANGULO_INICIAL_VALVULA);
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

  // Motores (Mixer e Dosador de chocolate)
  pinMode(RELE_MIXER, OUTPUT);
  pinMode(RELE_CHOCOLATE, OUTPUT);
  digitalWrite(RELE_MIXER, HIGH);
  digitalWrite(RELE_CHOCOLATE, HIGH);

  // Sensor e botões
  pinMode(SENSOR_COPO, INPUT);
  pinMode(BOTAO_CIMA, INPUT_PULLUP);
  pinMode(BOTAO_OK, INPUT_PULLUP);
  pinMode(RELE_CHOCOLATE, OUTPUT);

  // LCD
  startLCD();

  // Mostrar menu no LCD
  mostrarMenu();

  // Inicializa SPIFFS e carrega histórico
  inicializarSPIFFS();
  carregarHistorico();

  // Configura o tempo (NTP)
  configTime(0, 0, "pool.ntp.org");  // Configura o cliente NTP
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
    // servoCopo.write(ANGULO_FINAL_COPO);
    // delay(INTERVALO_COPO);
    // servoCopo.write(ANGULO_INICIAL_COPO);
    delay(INTERVALO_COPO);
    if (digitalRead(SENSOR_COPO) == LOW) return true;
  }
  return false;
}

void dosarChocolate(int ciclos) {
  digitalWrite(RELE_CHOCOLATE, LOW);
  delay(3000 + ciclos * 2000);
  digitalWrite(RELE_CHOCOLATE, HIGH);
}

void dosarLeite() {
  servoLeite.write(ANGULO_FINAL_LEITE);
  delay(TEMPO_LEITE);
  servoLeite.write(ANGULO_INICIAL_LEITE);
}


void iniciarProcesso() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Iniciando...");
  delay(1000);
  contadorProcessos++;
  ultimaDataEHora = String(__DATE__) + " " + String(__TIME__);  // simples forma de registrar data/hora da compilação

  if (nivelLeite < 100 || nivelChoco < 50 || nivelCopos <= 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sem ingredientes!");
    return;
  }

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
  digitalWrite(RELE_MIXER, LOW);
  for (int i = TEMPO_DE_MISTURA; i > 0; i--) {
    lcd.setCursor(0, 1);
    lcd.print("Tempo: ");
    lcd.print(i);
    lcd.print("s ");
    delay(1000);
  }
  digitalWrite(RELE_MIXER, HIGH);

  // Dispensar bebida
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Liberando bebida");
  servoValvula.write(ANGULO_FINAL_VALVULA);
  delay(TEMPO_VALVULA);
  servoValvula.write(ANGULO_INICIAL_VALVULA);

  // Aguarda retirada do copo
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pronto!");
  while (digitalRead(SENSOR_COPO) == LOW) {
    delay(100);
  }

    // Calcula o consumo aproximado
  int leiteUsado = 200;  // Valor aproximado em ml
  int chocoUsado = 30 * intensidade;  // Valor aproximado em gramas

  // Adiciona ao histórico
  adicionarAoHistorico(intensidade, leiteUsado, chocoUsado);

  // Atualiza contadores
  nivelLeite -= leiteUsado;
  nivelChoco -= chocoUsado;
  nivelCopos--;
  mostrarMenu();
}