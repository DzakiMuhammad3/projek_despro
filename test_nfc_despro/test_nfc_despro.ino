#include <Adafruit_PN532.h>
#include <LiquidCrystal_I2C.h>

#define PN532_SCK   18
#define PN532_MISO  19
#define PN532_MOSI  23
#define PN532_SS    5

#define RELAY 13






int lcdColumns = 20;
int lcdRows = 4;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

String IDCARD;
void nfc_setup();
String read_nfc();
String read2_nfc();



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  lcd.begin();
  lcd.backlight();
  nfc_setup();

}

void loop() {
  // put your main code here, to run repeatedly:
  IDCARD = read2_nfc();
  Serial.println("Halo");
  write_lcd("Your ID: ", IDCARD);
  delay(20);

}

void nfc_setup(){
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.println("Didn't find PN53x board");
    write_lcd("PN5X Board", "Didn't Found");
    while (1); // halt
  }
  Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);
  write_lcd("PN5X Board", "is found");
  nfc.SAMConfig();
}

String read2_nfc(){
  String card;
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;
  uint16_t timeout = 1000;
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, timeout);
  if (success) {
    // Display some basic information about the card
    card = "";
    for (byte i = 0; i <= uidLength - 1; i++) {
      card += (uid[i] < 0x10 ? "0" : "") +
      String(uid[i], HEX);
    }
    Serial.print("ID CARD : ");
    card.toUpperCase();
    Serial.print(card);
    Serial.println("");
  }
  return card;  
}

String read_nfc(){
  String card;
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  if (success) {
    // Display some basic information about the card
    card = "";
    for (byte i = 0; i <= uidLength - 1; i++) {
      card += (uid[i] < 0x10 ? "0" : "") +
      String(uid[i], HEX);
    }
    Serial.print("ID CARD : ");
    card.toUpperCase();
    Serial.print(card);
    Serial.println("");
  }else{
    return "";
  }
  return card;  
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
