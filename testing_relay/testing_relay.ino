#include <LiquidCrystal_I2C.h>

#define RELAY 13
#define SDA 21
#define SCL 22

int lcdColumns = 20;
int lcdRows = 4;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);


void open_lock(int relay_pin);
void close_lock(int relay_pin);



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(RELAY, OUTPUT);
//  pinMode(SDA, OUTPUT);
//  pinMode(SCL, OUTPUT);
  lcd.begin();                    
  lcd.backlight();
  void write_lcd(String first_message, String second_message);
  

}


void loop() {
  // put your main code here, to run repeatedly:
  close_lock(RELAY);
  Serial.println("Close");
  write_lcd("Close", ".....");
  delay(2000);
  open_lock(RELAY);
  Serial.println("Open");
  write_lcd("Open", ".....");
  delay(2000);

}

void open_lock(int relay_pin){
  digitalWrite(relay_pin, HIGH);
  Serial.println("HIGH");
  delay(100);
}

void close_lock(int relay_pin){
  digitalWrite(relay_pin, LOW);
  Serial.println("LOW");
  delay(100);
}

void write_lcd(String first_message, String second_message){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(first_message);
  lcd.setCursor(0,1);
  lcd.print(second_message);
}
