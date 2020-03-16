#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "WiFi.h"
#include <HTTPClient.h>
#include <Wire.h>
#include <Button2.h>
#include <Free_Fonts.h>
#include <WifiCredentials.h>
#include <ArduinoJson.h>  //https://arduinojson.org/v6/assistant/
#include <MillisTimer.h>
#include <string>

#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN   0x10
#endif

#define TFT_MOSI              19
#define TFT_SCLK              18
#define TFT_CS                5
#define TFT_DC                16
#define TFT_RST               23

#define TFT_BL                4  // Display backlight control pin
#define ADC_EN                14
#define ADC_PIN               34
#define BUTTON_1              35
#define BUTTON_2              0

#define SCREEN_HEIGHT         135
#define SCREEN_WIDTH          240

TFT_eSPI tft = TFT_eSPI(SCREEN_HEIGHT, SCREEN_WIDTH);
Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);

char buff[512];
int vref = 1100;
int btnCick = false;

const char* ssid = WIFI_SSID;
const char* password =  WIFI_PASSWD;

String baseUrl = "http://nerf-data-api-dfw.herokuapp.com";
String urlStatus = "/koth/status";

struct JsonTeamStruct {
  const char* teamName = ""; // "Red"
  bool isActive = false; // false
  int elapsedTimeInSeconds = 0; // 0
}jsonTeamStruct;

#define sizeOfTeamsAllowed 4
JsonTeamStruct teams[sizeOfTeamsAllowed];

MillisTimer statusTimer = MillisTimer(1000);

////////////////////////////////////////////////////////  Function Declerations  //////////////////////////////////////////////////////// 
void ConnectToNetwork(); 
double GetVoltage();
void espDelay(int ms);

void DrawTextCentered(String text);
void UpdateStatus(MillisTimer &mt);
void UpdateDisplay();
void SetTeamValuesFromJson (String jsonVal);
uint16_t GetTeamColor (std::string teamName);

////////////////////////////////////////////////////////  Setup & Loop  //////////////////////////////////////////////////////// 

void setup() {
  tft.init();
  tft.setRotation(1);

  Serial.begin(115200);

  DrawTextCentered("Starting");

  ConnectToNetwork();
  statusTimer.expiredHandler(UpdateStatus);
  statusTimer.start();
  tft.fillScreen(TFT_BLACK);
}
 
void loop() {
  statusTimer.run();
}

////////////////////////////////////////////////////////  Functions Implementations //////////////////////////////////////////////////////// 

void ConnectToNetwork() {
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    String connectingText = "Connecting to " + String(ssid);
    Serial.println(connectingText);
    DrawTextCentered (connectingText);
    // delay(500);
  }
  String connectedText = "Connected to " + String(ssid);
  Serial.println(connectedText);
  DrawTextCentered (connectedText);
  // espDelay(2000);
}

double GetVoltage () {
  // static uint64_t timeStamp = 0;
  // if (millis() - timeStamp > 1000) {
  //     timeStamp = millis();
      uint16_t v = analogRead(ADC_PIN);
      float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
      String voltage = "Voltage :" + String(battery_voltage) + "V";
      // tft.fillScreen(TFT_BLACK);
      // tft.setTextColor(TFT_RED);
      // tft.drawString( voltage ,  0 , SCREEN_HEIGHT );
      return battery_voltage;
  // }
  // return 9999;
}

//! Long time delay, it is recommended to use shallow sleep, which can effectively reduce the current consumption
void espDelay(int ms)
{   
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH,ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}

void DrawTextCentered (String text) {
  tft.setFreeFont(&FreeSans9pt7b);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(text, SCREEN_WIDTH / 2 , SCREEN_HEIGHT / 2);
}

void UpdateStatus(MillisTimer &mt) {
  if (WiFi.status() == WL_CONNECTED) { //Check the current connection status
 
    HTTPClient http;
 
    String fullUrl = baseUrl + urlStatus;
    Serial.println(fullUrl);
    http.begin(fullUrl);
    int httpCode = http.GET();                                        //Make the request
 
    if (httpCode > 0) { //Check for the returning code
 
        String payload = http.getString();
        Serial.println(httpCode);
        Serial.println(payload);
        SetTeamValuesFromJson(payload);
        UpdateDisplay();
        // tft.setCursor(0,0);
        // tft.println(payload);
      }
 
    else {
      Serial.println("Error on HTTP request");
    }
 
    http.end(); //Free the resources
  } else {
    Serial.println("No HTTP for some reason");
    ConnectToNetwork();
  }
}

void SetTeamValuesFromJson (String jsonVal) {
  const size_t capacity = JSON_ARRAY_SIZE(4) + JSON_OBJECT_SIZE(1) + 4*JSON_OBJECT_SIZE(4) + 280;
  DynamicJsonDocument doc(capacity);

  // const char* json = "{\"Teams\":[{\"teamName\":\"Red\",\"isActive\":false,\"timerStartedAt\":null,\"elapsedTimeInSeconds\":0},{\"teamName\":\"Blue\",\"isActive\":false,\"timerStartedAt\":null,\"elapsedTimeInSeconds\":0},{\"teamName\":\"Green\",\"isActive\":false,\"timerStartedAt\":null,\"elapsedTimeInSeconds\":0},{\"teamName\":\"Yellow\",\"isActive\":false,\"timerStartedAt\":null,\"elapsedTimeInSeconds\":0}]}";

  // deserializeJson(doc, json);
  
  deserializeJson(doc, jsonVal);

  JsonArray Teams = doc["Teams"];
  
  for (int i = 0 ; i < sizeOfTeamsAllowed; i++) {
    teams[i] = {};  // Clear out object just in case-ies'
    JsonObject Team = Teams[i];
    teams[i].teamName = Team["teamName"];
    teams[i].isActive = Team["isActive"];
    teams[i].elapsedTimeInSeconds = Team["elapsedTimeInSeconds"];
    Serial.println(String(teams[i].teamName));
    Serial.println(String(teams[i].elapsedTimeInSeconds));
  }
}

void UpdateDisplay() {
  // // Clear Display
  // tft.fillScreen(TFT_BLACK);

  // Voltage
  double voltage = GetVoltage();
  if (voltage > 3.7) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
  }
  String voltageText = String(voltage) + "v";
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextPadding(50);
  tft.drawString( voltageText , 0 , SCREEN_HEIGHT );

  // Team Info
  int padding = 100; //tft.textWidth("Team: 9999", );
  tft.setTextPadding(padding);
  tft.setTextDatum(TL_DATUM);
  tft.setFreeFont(&FreeSans18pt7b);
  for (int i =0 ; i < 2 ; i++){
    String displayText = " " + String(teams[i].teamName) + " " + String(teams[i].elapsedTimeInSeconds) + " ";
    
    std::string s;
    s.append(teams[i].teamName);
    
    tft.setTextColor(GetTeamColor(s),TFT_BLACK);
    tft.drawString(displayText, 0, (40*i) + 5);
  }
  
}

uint16_t GetTeamColor (std::string teamName) {
    if (teamName.find("Red") != std::string::npos) {
      return TFT_RED;
    } else if (teamName.find("Blue") != std::string::npos) {
      return TFT_BLUE;
    } 
    return TFT_YELLOW;
  }