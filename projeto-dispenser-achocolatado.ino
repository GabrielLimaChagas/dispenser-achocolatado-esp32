#include <ESP32Servo.h>

Servo servoMotor;

const int pinServo = 13;

int servoPos = 0;

void setup() {
  servoMotor.setPeriodHertz(50);
  servoMotor.attach(pinServo, 500, 2400);
  servoMotor.write(servoPos);
}

void loop() {
  while (servoPos < 180) {
    servoMotor.write(servoPos);
    servoPos++;
    delay(100);
  }
}