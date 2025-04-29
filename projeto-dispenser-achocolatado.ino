const int pinIRSensor = 14;

void setup() {
  Serial.begin(115200);
  pinMode(pinIRSensor, INPUT);
}

void loop() {
  if (digitalRead(pinIRSensor) == LOW) {
    Serial.println("Detectado");
  }
}