#include <Arduino.h>
#include <SPI.h>
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
#define DOOR_SENSOR 14

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
String UID;
String visitorName;
bool IS_AUTHORIZED = false;
int IS_OPEN;
int state_door;

Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

String read_nfc();
bool check_data(String UID);
void write_lcd(String first_message, String second_message);
void nfc_setup();
void open_lock(int relay_pin);
void close_lock(int relay_pin);
void keepWiFiAlive(void * parameters);
void printLocalTime();


void setup() {
  Serial.begin(115200);
  pinMode(RELAY, OUTPUT);
  close_lock(RELAY);   
  
  pinMode(DOOR_SENSOR, INPUT);
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
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  //Inisialisasi LCD
  lcd.begin();                    
  lcd.backlight();

  nfc_setup();
  Serial.println("Approximate your card to the reader...");
  Serial.println();
  write_lcd("Starting setup", "Success");
  delay(1500);
  lcd.clear();
}

void loop() {
  
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 7000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    
    if (Firebase.RTDB.getInt(&fbdo, "/utils/modeSignup")) {
      modeSignup = fbdo.intData();
      Serial.print("modeSignup: ");
      Serial.println(modeSignup);
    }

    if (Firebase.RTDB.getString(&fbdo, "/utils/statusMessage")) {
        statusMessage = fbdo.stringData();
        Serial.println(statusMessage);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(statusMessage);
        Serial.println("Test 3 Berhasil");
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    write_lcd("Tap Kartu Master", "SEDANG OFFLINE");
  }

  IS_OPEN = digitalRead(DOOR_SENSOR);
  UID = read_nfc();
  if (UID == "") return;

  if (WiFi.status() == WL_CONNECTED){
    if (modeSignup == 0) {
      //CODE UNLOCK LEMARI
      Serial.print("UID Status: ");
      Serial.println("Mode Online");
      Serial.println("Status Door Sensor: " + IS_OPEN);
      IS_OPEN = digitalRead(DOOR_SENSOR);
      
      if(!IS_OPEN){
        if (Firebase.RTDB.getJSON(&fbdo, "/users/" + UID)) {
          lcd.clear();
          if (Firebase.RTDB.getString(&fbdo, "/users/" + UID + "/name")) {
            visitorName = fbdo.stringData();
            write_lcd("Selamat Datang", visitorName);
          }
          printLocalTime();
          Serial.println(String(digitalRead(PN532_SS)));
          Serial.println("Access granted");
          write_lcd("Akses Diterima", "!!!");
          open_lock(RELAY);
          while(!IS_OPEN){
            IS_OPEN = digitalRead(DOOR_SENSOR);
          }
          delay(500);
          close_lock(RELAY);
        }
        else {
          Serial.println("Not Authorized");
          write_lcd("Anda tidak", "memiliki akses ");
        }
      }else{
        Serial.println("Pintu Terbuka");
        write_lcd("Pintu Terbuka", "Jangan lupa ditutup");
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
    IS_OPEN = digitalRead(DOOR_SENSOR);
    if(!IS_OPEN){
      IS_AUTHORIZED = check_data(UID);
      if(IS_AUTHORIZED){
        Serial.println(String(digitalRead(PN532_SS)));
        Serial.println("Access granted");
        write_lcd("Access Diterimaa", "!!!");
        open_lock(RELAY);
        while(!IS_OPEN){
          IS_OPEN = digitalRead(DOOR_SENSOR);
        }
        delay(500);
        close_lock(RELAY);
      }
      else {
        if(UID != ""){
          Serial.println("Access Denied");
          Serial.println("Your Id: " + UID);
          write_lcd("Access Ditolak", UID);
        } 
      }
    }
    else {
      Serial.println("Pintu Terbuka");
      write_lcd("Pintu Terbuka", "Jangan lupa ditutup");
    }  
  }
  delay(500);
}

void keepWiFiAlive(void * parameters) {
  for (;;) {
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

void check_door_state(){
  int dum1, dum2, dum3, total_dum;

  dum1 = digitalRead(DOOR_SENSOR);
  delay(30);
  dum2 = digitalRead(DOOR_SENSOR);
  delay(30);
  dum3 = digitalRead(DOOR_SENSOR);
  delay(30);
  total_dum = dum1 + dum2 + dum3;
  while(total_dum != 3 || total_dum != 0){
    dum1 = digitalRead(DOOR_SENSOR);
    delay(30);
    dum2 = digitalRead(DOOR_SENSOR);
    delay(30);
    dum3 = digitalRead(DOOR_SENSOR);
    delay(30);
    total_dum = dum1 + dum2 + dum3;
  }
  if(total_dum == 3){
    IS_OPEN = 1;
  }else if(total_dum == 0){
    IS_OPEN = 0;
  }
  
}

void nfc_setup(){
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.println("Didn't find PN53x board");
    write_lcd("PN5X Board", "Didn't Found");
    while (!versiondata){
      versiondata = nfc.getFirmwareVersion();
      write_lcd("Checking NFC SENSOR", "...");
      delay(100);
    }
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
//  String masterCard = "237497AC";
  String masterCard = "04382652F85180";
  if(UID == masterCard){
    return true;
  }else{
    return false;
  }
}

void open_lock(int relay_pin){
  digitalWrite(relay_pin, LOW);
  Serial.println("LOW");
  delay(100);
}

void close_lock(int relay_pin){
  digitalWrite(relay_pin, HIGH);
  Serial.println("HIGH");
  delay(100);
}

void write_lcd(String first_message, String second_message){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(first_message);
  lcd.setCursor(0,1);
  lcd.print(second_message);
}

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
