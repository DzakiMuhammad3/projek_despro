#include <Arduino.h>
#include <SPI.h>
//#include <MFRC522.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include "time.h"
#include <Wire.h>
#include <Adafruit_PN532.h>
#include <LiquidCrystal_I2C.h>
#include <bits/stdc++.h>
using namespace std;

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define PN532_SCK   18
#define PN532_MISO  19
#define PN532_MOSI  23
#define PN532_SS    5

#define RELAY 13

#define WIFI_SSID "wow"
#define WIFI_PASSWORD "hehehehe"
#define WIFI_TIMEOUT_MS 20000

#define API_KEY "AIzaSyAXM39cqFGaeTtAMiePxRw9L8RPZTqVf2Y"
#define DATABASE_URL "https://smart-cabinet-747fd-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL "muhammad.nur94@ui.ac.id"
#define USER_PASSWORD "#11Ismail"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson json;

int lcdColumns = 20;
int lcdRows = 4;
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 25200;
const int   daylightOffset_sec = 3600;
unsigned long sendDataPrevMillis = 0;
unsigned long sendDataPrevMillis2 = 0;
int modeSignup;
int statusRegister;
String statusMessage;
String statusSignup;
String UID;
String visitorName;
bool signupOK = false;
int timestamp;
char *string_now;

String IDCARD;
bool IS_AUTHORIZED = false;
bool IS_OPEN = false;
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

//#define SS_PIN 5
//#define RST_PIN 27
//#define WATCHDOG_TIMEOUT_S 10
//MFRC522 mfrc522(SS_PIN, RST_PIN);
//hw_timer_t * watchDogTimer = NULL;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

String read_nfc();
bool check_data(String idcard);
void write_lcd(String first_message, String second_message);
void nfc_setup();
void open_lock(int relay_pin);
void close_lock(int relay_pin);
void keepWiFiAlive(void * parameters);
void printLocalTime();

void setup() {
  Serial.begin(115200);
  pinMode(RELAY, OUTPUT);
  //Multitasking untuk selalu reconnecting WiFi
  xTaskCreatePinnedToCore(
    keepWiFiAlive,
    "Keep WiFi Alive",
    5000,
    NULL,
    1,
    NULL,
    0
    );
    
  //Set Waktu untuk logging
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  //set untuk Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  
//  if (Firebase.signUp(&config, &auth, "", "")) {
//    Serial.println(fbdo.jsonString());
//    signupOK = true;
//  }

//  else {
//    Serial.printf("%s\n", config.signer.signupError.message.c_str());
//  }
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);


  //Inisialisasi LCD
//  Wire.begin();
  lcd.begin();                    
  lcd.backlight();
 
//  SPI.begin();
  nfc_setup();
  //mfrc522.PCD_Init();
  Serial.println("Approximate your card to the reader...");
  Serial.println();
  write_lcd("Oy Jalanin", "dfdf");
  delay(1500);
  lcd.clear();
  

}

//void loop(){
//  close_lock(RELAY);
//  Serial.println("Close");
//  write_lcd("Close", ".....");
//  delay(2000);
//  open_lock(RELAY);
//  Serial.println("Open");
//  write_lcd("Open", ".....");
//  delay(2000);
//}

void loop() {
  Serial.println("Test 0");
  Serial.println(String(signupOK));
  Serial.println(String(Firebase.ready()));
  
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 7000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    Serial.println("Test 1 Berhasil");
    
    if (Firebase.RTDB.getInt(&fbdo, "/utils/modeSignup")) {
      modeSignup = fbdo.intData();
      Serial.print("modeSignup: ");
      Serial.println(modeSignup);
      Serial.println("Test 2 Berhasil");
    }else{
      Serial.println("Test 2 Gagal");
    }

    if (Firebase.RTDB.getString(&fbdo, "/utils/statusMessage")) {
        statusMessage = fbdo.stringData();
        Serial.println(statusMessage);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(statusMessage);
        Serial.println("Test 3 Berhasil");
    }else{
      Serial.println("Test 3 Gagal");
    }
  }
  
  UID = read_nfc();

  if(WiFi.status() == WL_CONNECTED){
    
  
    if (modeSignup == 0) {
      //CODE UNLOCK LEMARI
      Serial.print("UID Status: ");
      if (Firebase.RTDB.getJSON(&fbdo, "/users/" + UID)) {
        lcd.clear();
        if (Firebase.RTDB.getString(&fbdo, "/users/" + UID + "/name")) {
          visitorName = fbdo.stringData();
          lcd.print(visitorName);
        }
        Serial.println("Unlocked");
        lcd.setCursor(0, 1);
        lcd.print("Unlocked");
        for (int i = 0; i < 3; i++) {
          delay(300);
          lcd.print(".");
        }
        printLocalTime();
        
        //CODE BUKA LOCK
        if(IS_OPEN) {
          close_lock(RELAY);
          IS_OPEN = false;
        }
        else {
          open_lock(RELAY);
          IS_OPEN = true;
        }
      }
      else {
        Serial.println("Not Authorized");
        write_lcd("Access denied", "Not Authorized");
        delay(3000);
      }
    }
    else {
      //CODE REGISTER
      while(modeSignup == 1) {
        if (Firebase.ready() && (millis() - sendDataPrevMillis2 > 7000 || sendDataPrevMillis2 == 0)){
          sendDataPrevMillis2 = millis();
          if (Firebase.RTDB.getInt(&fbdo, "/utils/modeSignup")) {
            modeSignup = fbdo.intData();
            if (modeSignup == 0) {
              break;
            }
          }
          Serial.print("UID scanning status: ");
          if (Firebase.RTDB.setString(&fbdo, "/utils/readRFID", UID)) {
            Serial.println("Sending success");
            Serial.print("Status RFID: ");
            if (Firebase.RTDB.setInt(&fbdo, "/utils/statusRFID", 1)) {
              Serial.println("Registering UID...");
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Registering");
              for (int i = 0; i < 3; i++) {
                delay(300);
                lcd.print(".");
              }
            }
            else {
              Serial.println("Registering Failed");
              write_lcd("Failed", "Tap again");
            }
          }
          else {
            Serial.println("Sending failed");
            write_lcd("Failed", "Tap again");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Failed");
            lcd.setCursor(0, 1);
            lcd.print("Tap again");
          }
        }
      }
      if (WiFi.status() == WL_CONNECTED) {
        if (Firebase.RTDB.getInt(&fbdo, "/utils/statusRegister")) {
          statusRegister = fbdo.intData();
          lcd.clear();
          lcd.print("Register");
          lcd.setCursor(0,1);
          if (statusRegister == 1) {
            lcd.print("Succeed");
            Firebase.RTDB.setInt(&fbdo, "/utils/statusRegister", 0);
          }
          else {
            lcd.print("Failed/Canceled");
          }
        }
        else {
          write_lcd("Register", "Failed/Canceled");
        }
      }
      delay(3000);
    }
  }
  else if((WiFi.status() != WL_CONNECTED)) {
    //Scan UID OFFLINE
    Serial.println("Success offline");
    IS_AUTHORIZED = check_data(UID);
    if(IS_AUTHORIZED){
      Serial.println(String(digitalRead(PN532_SS)));
      Serial.println("Access granted");
      write_lcd("Access Diterima", "!!!");
      if(IS_OPEN) {
        close_lock(RELAY);
        IS_OPEN = false;
      }
      else {
        open_lock(RELAY);
        IS_OPEN = true;
      }
    }
    else {
//      Serial.println(String(digitalRead(PN532_SS)));
      if(UID == ""){
        Serial.println("Go On");
      }else{
        
      
        Serial.println("Access Denied");
        Serial.println("Your Id: " + UID);
        write_lcd("Access Ditolak", UID);
      } 
    }
  }
  delay(500);
}

void keepWiFiAlive(void * parameters) {
  for (;;) {
//    Serial.print("setup() running on core ");
//    Serial.println(xPortGetCoreID());
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi still connected");
      vTaskDelay(10000 / portTICK_PERIOD_MS);
      continue;
    }
    Serial.println("WiFi Connecting");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS) {
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Connecting to wifi failed");
        vTaskDelay(20000 / portTICK_PERIOD_MS);
        continue;
      }

      Serial.println("Wifi Connected: " + WiFi.localIP());
    }
  }
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

String read_nfc(){
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

bool check_data(String UID){
  String masterCard = "04382652F85180";
  if(UID == masterCard){
    return true;
  }else{
    return false;
  }
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

//String readCard() {
//  Serial.print("UID tag :");
//  String content = "";
//  byte letter;
//  lcd.clear();
//  lcd.setCursor(0, 0);
//  lcd.print("Reading");
//  for (int i = 0; i < 3; i++) {
//    delay(300);
//    lcd.print(".");
//  }
//  
//  for (byte i = 0; i < mfrc522.uid.size; i++) {
//    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
//    Serial.print(mfrc522.uid.uidByte[i], HEX);
//    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
//    content.concat(String(mfrc522.uid.uidByte[i], HEX));
//  }
//
//  Serial.println();
//  content.toUpperCase();
//  delay(2000);
//  return content.substring(1);
//}

//void connectWiFi() {
//  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//  Serial.print("Connecting to ");
//  Serial.print(WIFI_SSID);
//
//  watchDogRefresh();
//  long startTime = millis();
//
//  while(WiFi.status() != WL_CONNECTED) {
//    delay(1000);
//    Serial.print(". ");
//    long loopTime = millis() - startTime;
//    if(loopTime > 7000) {
//      Serial.println("\nConnection Failed!");
//      delay(3000);
//    }
//  }
//
//  watchDogRefresh();
//  Serial.println("\nConnected to " + String(WIFI_SSID));
//  Serial.print("IP address: ");
//  Serial.println(WiFi.localIP());
//}

void write_lcd(String first_message, String second_message){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(first_message);
  lcd.setCursor(0,1);
  lcd.print(second_message);
}

//void scrollText(int row, String message, int delayTime, int lcdColumns) {
//  for (int i=0; i < lcdColumns; i++) {
//    message = " " + message;  
//  } 
//  message = message + " "; 
//  for (int pos = 0; pos < message.length(); pos++) {
//    lcd.setCursor(0, row);
//    lcd.print(message.substring(pos, pos + lcdColumns));
//    delay(delayTime);
//  }
//}

void printLocalTime(){
  struct tm timeinfo;
  char jsonTime[13];
  char timeValue[70];
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  strftime(jsonTime, 13, "%y%m%d%H%M%S", &timeinfo);
  Serial.println(jsonTime);
  strftime(timeValue, 70, "%A %B %d %Y %H:%M:%S", &timeinfo);
  Serial.println(timeValue);
  String jsonTimeString = jsonTime;
  String timeValueString = timeValue;
  if (Firebase.RTDB.setString(&fbdo, "/logs/" + jsonTimeString + "/time", timeValueString)) {
    if (Firebase.RTDB.setString(&fbdo, "/logs/" + jsonTimeString + "/uid", UID)) {
      if (Firebase.RTDB.setString(&fbdo, "/logs/" + jsonTimeString + "/name", visitorName)) {
        Serial.println("Success logging");
      }
    }
  }
}
