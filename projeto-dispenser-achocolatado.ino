#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Display LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

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

// Objetos servo
Servo servoCopo, servoChocolate, servoLeite, servoValvula;

// Estado do sistema
int intensidade = 1; // 1=fraco, 2=médio, 3=forte
String intensidades[] = {"Fraco", "Médio", "Forte"};

void setup() {
  Serial.begin(115200);
  
  // Servo motores
  servoCopo.attach(SERVO_COPO);
  servoChocolate.attach(SERVO_CHOCOLATE);
  servoLeite.attach(SERVO_LEITE);
  servoValvula.attach(SERVO_VALVULA);
  servoCopo.write(10);
  servoChocolate.write(10);
  servoLeite.write(10);
  servoValvula.write(10);

  // Mixer
  pinMode(MIXER, OUTPUT);
  digitalWrite(MIXER, LOW);

  // Sensor e botões
  pinMode(SENSOR_COPO, INPUT);
  pinMode(BOTAO_CIMA, INPUT_PULLUP);
  pinMode(BOTAO_OK, INPUT_PULLUP);

  // LCD
  lcd.init();
  lcd.backlight();

  mostrarMenu();
}

void loop() {
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
