#include <ESP32Servo.h>

Servo servoMotor;

const int pinServo = 13;
const int pinIRSensor = 14;

int servoPos = 90;

void handleDispenseCup() {
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
}

void setup() {
  pinMode(pinIRSensor, INPUT);

  servoMotor.setPeriodHertz(50);
  servoMotor.attach(pinServo, 500, 2400);
  servoMotor.write(servoPos);
}

void loop() {
  if (digitalRead(pinIRSensor) == LOW) {
    handleDispenseCup();
  }
}