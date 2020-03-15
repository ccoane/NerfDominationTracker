#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "WiFi.h"
#include <Wire.h>
#include <Button2.h>
#include <Free_Fonts.h>
#include <WifiCredentials.h>

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


TFT_eSPI tft = TFT_eSPI();
Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);

char buff[512];
int vref = 1100;
int btnCick = false;

const char* ssid = WIFI_SSID;
const char* password =  WIFI_PASSWD;

////////////////////////////////////////////////////////  Function Declerations  //////////////////////////////////////////////////////// 
void ConnectToNetwork(); 
double GetVoltage();
void espDelay(int ms);

void DrawTextCentered(String text);

////////////////////////////////////////////////////////  Setup & Loop  //////////////////////////////////////////////////////// 
void setup() {
  tft.init();
  tft.setRotation(1);

  Serial.begin(115200);

  DrawTextCentered("Starting");

  ConnectToNetwork();
  
}
 
void loop() {
  GetVoltage();
  espDelay(1000);
}

////////////////////////////////////////////////////////  Functions Implementations //////////////////////////////////////////////////////// 
void ConnectToNetwork() {
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    String connectingText = "Connecting to " + String(ssid);
    Serial.println(connectingText);
    DrawTextCentered (connectingText);
    delay(500);
  }
  String connectedText = "Connected to " + String(ssid);
  Serial.println(connectedText);
  DrawTextCentered (connectedText);
  espDelay(2000);
}

double GetVoltage () {
  static uint64_t timeStamp = 0;
  if (millis() - timeStamp > 1000) {
      timeStamp = millis();
      uint16_t v = analogRead(ADC_PIN);
      float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
      String voltage = "Voltage :" + String(battery_voltage) + "V";
      Serial.println(voltage);
      tft.fillScreen(TFT_BLACK);
      // tft.setTextDatum(BL_DATUM);
      tft.setTextColor(TFT_RED);
      tft.drawString( voltage ,  0 , SCREEN_HEIGHT );
      // tft.print(voltage);
  }
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