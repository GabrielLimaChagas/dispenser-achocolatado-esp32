#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <string>

LiquidCrystal_I2C lcd(0x27, 16, 2); // Endereço comum: 0x27 ou 0x3F

void setup() {
  Wire.begin();
  lcd.init();
  lcd.backlight();
}

int count = 0;
void loop() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("count: ");
  lcd.print(count);
  count++;
  delay(1000);
}