#include <Arduino.h>
#include <map>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Button2.h>
#include <WifiCredentials.h>
#include <ArduinoJson.h>  //https://arduinojson.org/v6/assistant/
#include <MillisTimer.h>
#include <string>
#include <Media\pogchamp.h>
#include <Media\triangle.h>
#include <Media\AWinnerIsYou.h>
#include <Media\MarioSleep.h>
#include <Fonts\Free_Fonts.h>
#include <Fonts\GothamLight_7.h>
#include <Fonts\GothamLight_9.h>
#include <Fonts\GothamLight_12.h>
#include <Fonts\GothamBook_7.h>
#include <Fonts\GothamBook_9.h>
#include <Fonts\GothamBook_12.h>
#include <ESP_WiFiManager.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <WiFiClient.h>

#define ESP_getChipId()   ((uint32_t)ESP.getEfuseMac())

#define LED_ON HIGH
#define LED_OFF LOW

#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN 0x10
#endif

#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_CS 5
#define TFT_DC 16
#define TFT_RST 23

#define TFT_BL 4 // Display backlight control pin
#define ADC_EN 14
#define ADC_PIN 34
#define BUTTON_1 35
#define BUTTON_2 0
#define BUTTON_RED 12
#define BUTTON_BLUE 13

#define RED_TEAM "Red"
#define BLUE_TEAM "Blue"


// Supposed dimensions
#define SCREEN_HEIGHT 135
#define SCREEN_WIDTH 240

int pointsToWin = 20;

TFT_eSPI tft = TFT_eSPI(SCREEN_HEIGHT, SCREEN_WIDTH);
Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);
Button2 btnRed(BUTTON_RED);
Button2 btnBlue(BUTTON_BLUE);

auto MarioSleepGif = { MarioSleep_00,MarioSleep_01,MarioSleep_02,MarioSleep_03,MarioSleep_04,MarioSleep_05,MarioSleep_06,MarioSleep_07,MarioSleep_08,MarioSleep_09,MarioSleep_10,MarioSleep_11 };

char buff[512];
int vref = 1100;
int btnCick = false;

HTTPClient http;

// const char* ssidLocal = WIFI_SSID;
// const char* passwordLocal =  WIFI_PASSWD;

// SSID and PW for Config Portal
String apStationSSID = "ESP_" + String(ESP_getChipId());
const char* apStationPassword = "1234567890";

// SSID and PW for your Router
String Router_SSID;
String Router_Pass;

String baseUrl = "http://nerf-data-api-dfw.herokuapp.com/koth";
// String baseUrl = "http://192.168.10.10:3000/koth";
String urlStatus = "/status";
String urlStopTimers = "/stopTimers";
String urlStartTeamTimer = "/startTimer";
const String urlStartTeamTimerFull = baseUrl + urlStartTeamTimer;
const String urlStopTimersFull = baseUrl + urlStopTimers;

struct JsonTeamStruct {
  const char* teamName = ""; // "Red"
  bool isActive = false; // false
  int elapsedTimeInSeconds = 0; // 0
}jsonTeamStruct;

String elapsedGameTime = "";
const char* ElapsedGameTimeFormatted = "";

String TeamToProcess = "";

#define sizeOfTeamsAllowed 2
JsonTeamStruct teams[sizeOfTeamsAllowed];
JsonTeamStruct teamsTemp[sizeOfTeamsAllowed];

MillisTimer statusTimer = MillisTimer(500);
MillisTimer postGameTimer = MillisTimer(30000);

TaskHandle_t ButtonLoopTaskHandle;

////////////////////////////////////////////////////////  Function Declerations  //////////////////////////////////////////////////////// 
void ConnectToNetwork(); 
double GetVoltage();
void espDelay(int ms);
void CreateAsyncSerialTask();
void SetButtonDefaults();
void ButtonLoopTask(void * parameter);
void ButtonLoop();

void DrawTextCentered(String text);
void UpdateStatus(MillisTimer &mt);
void SetDisplayToSleepExpired(MillisTimer &mt);
void UpdateDisplay();
void SetTeamValuesFromJson (String jsonVal);
uint16_t GetTeamColor (String teamName, bool isForProgressBar);
void StartWifiConfigManager() ;
void DisplayWinnerScreen (String teamWinner, const char *ElapsedGameTimeFormatted);
void SetDisplayToSleep();
void SendTeamTimerButtonPress(String team, int retries);
void StartConfigPortal();
void StopTimers();

////////////////////////////////////////////////////////  Setup & Loop  //////////////////////////////////////////////////////// 

void setup() {
  Serial.begin(115200);
  Serial.println("STARTED");
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setSwapBytes(true);
  tft.pushImage(0, 0,  SCREEN_HEIGHT, SCREEN_WIDTH, pogchamp);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Domination", 140, 70);
  tft.drawString("Tracker", 140, 100);
  SetButtonDefaults();
  for (int i = 0 ; i < 1500 ; i ++) {
    // Serial.println("checking button");
    ButtonLoop();
  }

  DrawTextCentered("Starting");

  // ConnectToNetwork();
  StartWifiConfigManager();
  statusTimer.expiredHandler(UpdateStatus);
  statusTimer.start();
  postGameTimer.expiredHandler(SetDisplayToSleepExpired);
  CreateAsyncSerialTask();
  tft.fillScreen(TFT_BLACK);
}
 
void loop() {
  // ButtonLoop();
  if (TeamToProcess != "") {
    SendTeamTimerButtonPress(TeamToProcess, 5);
    TeamToProcess = "";
  }
  statusTimer.run();
}

////////////////////////////////////////////////////////  Functions Implementations //////////////////////////////////////////////////////// 

void StartWifiConfigManager() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TC_DATUM);
  int padding = tft.textWidth("The Quick Brown Fox", GFXFF); // get the width of the text in pixels
  tft.setTextPadding(padding);
  tft.setFreeFont(&GothamLight9pt7b);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Connect to AP:", SCREEN_WIDTH * .50 , SCREEN_HEIGHT * .00);
  tft.drawString(apStationSSID, SCREEN_WIDTH * .50 , SCREEN_HEIGHT * .20);

  ESP_WiFiManager ESP_wifiManager;

  if (btnCick) {
    tft.fillScreen(TFT_BLACK);
    tft.drawString("Connect to AP:", SCREEN_WIDTH * .50 , SCREEN_HEIGHT * .00);
    tft.drawString(apStationSSID, SCREEN_WIDTH * .50 , SCREEN_HEIGHT * .20);
    tft.drawString("Manual WiFi Config", SCREEN_WIDTH * .50 , SCREEN_HEIGHT * .50);
    tft.drawString("is Activated", SCREEN_WIDTH * .50 , SCREEN_HEIGHT * .75);
    StartConfigPortal();
    return;
  }
  
  Router_SSID = ESP_wifiManager.WiFi_SSID();
  Router_Pass = ESP_wifiManager.WiFi_Pass();

  if (Router_SSID.length() > 0) {
    tft.drawString("Saved SSID:", SCREEN_WIDTH * .50 , SCREEN_HEIGHT * .50);
    tft.drawString("SSIDstr: " + Router_SSID, SCREEN_WIDTH * .50 , SCREEN_HEIGHT * .75);
  }

  //Remove this line if you do not want to see WiFi password printed
  Serial.println("Stored: SSID = " + Router_SSID + ", Pass = " + Router_Pass);
 
  // For some unknown reason webserver can only be started once per boot up 
  // so webserver can not be used again in the sketch.
  #define WIFI_CONNECT_TIMEOUT        15000L
  #define WHILE_LOOP_DELAY            200L
  #define WHILE_LOOP_STEPS            (WIFI_CONNECT_TIMEOUT / ( 3 * WHILE_LOOP_DELAY ))
  
  unsigned long startedAt = millis();
  
  while ( (WiFi.status() != WL_CONNECTED) && (millis() - startedAt < WIFI_CONNECT_TIMEOUT ) )
  {   
    WiFi.mode(WIFI_STA);
    WiFi.persistent (true);
    // We start by connecting to a WiFi network
  
    Serial.print("Connecting to ");
    Serial.println(Router_SSID);
  
    WiFi.begin(Router_SSID.c_str(), Router_Pass.c_str());

    int i = 0;
    while((!WiFi.status() || WiFi.status() >= WL_DISCONNECTED) && i++ < WHILE_LOOP_STEPS)
    {
      delay(WHILE_LOOP_DELAY);
    }    
  }

  if (WiFi.status() == WL_CONNECTED && !btnCick) {
    return;
  } else {
    if (Router_SSID != "")
    {
      ESP_wifiManager.setConfigPortalTimeout(60); //If no access point name has been previously entered disable timeout.
      Serial.println("Timeout 60s");
    }
    else
      Serial.println("No timeout");

    // SSID to uppercase 
    apStationSSID.toUpperCase();  

    //it starts an access point 
    //and goes into a blocking loop awaiting configuration
    if (!ESP_wifiManager.startConfigPortal()) 
      Serial.println("Not connected to WiFi but continuing anyway.");
    else 
      Serial.println("WiFi connected...yeey :)");
  }
}

void StartConfigPortal() {
  ESP_WiFiManager ESP_wifiManager;
  if (!ESP_wifiManager.startConfigPortal()) 
    Serial.println("Not connected to WiFi but continuing anyway.");
  else 
    Serial.println("WiFi connected...yeey :)");
}

void SetButtonDefaults()
{
    btn1.setLongClickHandler([](Button2 & b) {
        btnCick = false;
        unsigned int time = b.wasPressedFor();
        if (time > 3000) {
          SetDisplayToSleep();
        }
    });
    btn1.setPressedHandler([](Button2 & b) {
        Serial.println("Detect Voltage..");
        btnCick = true;
    });

    btn2.setPressedHandler([](Button2 & b) {
        btnCick = true;
        Serial.println("btn press wifi scan");
        // wifi_scan();
    });
    
    btnRed.setPressedHandler([](Button2 & b) {
      btnCick = true;
      // SendTeamTimerButtonPress(RED_TEAM);
      TeamToProcess = RED_TEAM;
    });

    btnBlue.setPressedHandler([](Button2 & b) {
      btnCick = true;
      // SendTeamTimerButtonPress(BLUE_TEAM);
      TeamToProcess = BLUE_TEAM;
    });
}

void ButtonLoopTask(void * parameter) {
  while(true) {
    ButtonLoop();
  }
}

void ButtonLoop()
{
    btnRed.loop();
    btnBlue.loop();
    btn1.loop();
    btn2.loop();
}

void CreateAsyncSerialTask()
{
    // Set data fetching on Core 0 which leaves UI updates on Core 1
    xTaskCreatePinnedToCore(
      ButtonLoopTask, /* Function to implement the task */
      "ButtonLoopTaskOnCore0", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      0,  /* Priority of the task */
      &ButtonLoopTaskHandle,  /* Task handle. */
      0); /* Core where the task should run */
}

// Returns Voltage as a Double
double GetVoltage () {
    uint16_t v = analogRead(ADC_PIN);
    float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
    // String voltage = "Voltage :" + String(battery_voltage) + "V";
    return battery_voltage;
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
  
    String fullUrl = baseUrl + urlStatus;
    // Serial.println(fullUrl);
    http.begin(fullUrl);
    int httpCode = http.GET();                                        //Make the request
 
    if (httpCode > 0) { //Check for the returning code
 
        String payload = http.getString();
        // Serial.println(httpCode);
        // Serial.println(payload);
        SetTeamValuesFromJson(payload);
        UpdateDisplay();
      }
 
    else {
      Serial.println("Error on HTTP request");
    }
 
    http.end(); //Free the resources
  } else {
    Serial.println("No HTTP for some reason");
    // ConnectToNetwork();
  }
}

void SetDisplayToSleepExpired(MillisTimer &mt) {
  SetDisplayToSleep();
}

void SetTeamValuesFromJson (String jsonVal) {
  String winningTeam = "";
  const size_t capacity = JSON_ARRAY_SIZE(2) + 3*JSON_OBJECT_SIZE(4) + 210;
  DynamicJsonDocument doc(capacity);
  
  DeserializationError deserializationError = deserializeJson(doc, jsonVal);
  
  // Test if deserialization failed.
  if (deserializationError) {
    Serial.println("deserialization failed");
    return;
  }

  pointsToWin = doc["PointsToWin"];
  ElapsedGameTimeFormatted = doc["ElapsedGameTimeFormatted"];
  JsonArray Teams = doc["Teams"];
  
  for (int i = 0 ; i < sizeOfTeamsAllowed; i++) {
    // teams[i] = {};  // Clear out object just in case-ies'
    JsonObject Team = Teams[i];
    teams[i].teamName = Team["teamName"];
    teams[i].isActive = Team["isActive"];
    teams[i].elapsedTimeInSeconds = Team["elapsedTimeInSeconds"];
    if (teams[i].elapsedTimeInSeconds >= pointsToWin) {
      winningTeam = teams[i].teamName;
    }
  }

  if (winningTeam.length() > 0) {
    // Stop Status Timer and kill the Button Task on Core 0 as they are no longer needed.
    statusTimer.stop();
    vTaskSuspend(ButtonLoopTaskHandle);
    vTaskDelete(ButtonLoopTaskHandle);
    StopTimers();
    DisplayWinnerScreen(winningTeam, ElapsedGameTimeFormatted);
  }
}

void UpdateDisplay() {

  auto _teams = teams;

  // Voltage
  double voltage = GetVoltage();
  if (voltage > 3.2) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
  }
  String voltageText = String(voltage) + "v";
  tft.setTextDatum(TL_DATUM);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextSize(1);
  int padding = tft.textWidth("99.9v", GFXFF); // get the width of the text in pixels
  tft.setTextPadding(padding);
  tft.drawString( voltageText , SCREEN_WIDTH * .05 , SCREEN_HEIGHT * .05 );

  // Elapsed Game Time
  tft.setTextDatum(TR_DATUM);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextSize(1);
  padding = tft.textWidth("00:00", GFXFF); // get the width of the text in pixels
  tft.setTextPadding(padding);
  tft.drawString( String(ElapsedGameTimeFormatted) , SCREEN_WIDTH , SCREEN_HEIGHT * .05 );

  // Signal Strength
  tft.setTextDatum(TC_DATUM);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextSize(1);
  padding = tft.textWidth("-99dBm", GFXFF); // get the width of the text in pixels
  tft.setTextPadding(padding);
  String signalStrength = String(WiFi.RSSI());
  signalStrength = signalStrength.substring(0,3)  + "dBm"; // Substring 0,3 to ensure we don't get potential garbage data with signalStrength
  tft.drawString( signalStrength , SCREEN_WIDTH / 2 , SCREEN_HEIGHT * .05 );

  // Team Info
  for (int i =0 ; i < 2 ; i++){

    String teamName = _teams[i].teamName;

    // TFT_YELLOW is the default return of GetTeamColor if no team is found.   
    // In this scenario, don't udpate the team color bars if TFT_YELLOW.
    auto teamColorBackground = GetTeamColor(_teams[i].teamName, false);
    auto teamColorForeground = GetTeamColor(_teams[i].teamName, true);
    if (teamColorBackground != TFT_YELLOW && teamColorForeground != TFT_YELLOW) {
      
      // Progress Bar Background
      tft.fillRect( SCREEN_WIDTH *.05 , ( SCREEN_HEIGHT * .25 ) + (i * 32 ), SCREEN_WIDTH * .9, 30, teamColorBackground);

      // Progress Bar Begin/Mids/End Pieces
      tft.fillRect( SCREEN_WIDTH * .05 , ( SCREEN_HEIGHT * .25 ) + (i * 32 ), SCREEN_WIDTH * .025, 30, teamColorForeground);
      tft.fillRect( SCREEN_WIDTH * .50 , ( SCREEN_HEIGHT * .25 ) + (i * 32 ), SCREEN_WIDTH * .025, 30, teamColorForeground);
      tft.fillRect( SCREEN_WIDTH * .80 , ( SCREEN_HEIGHT * .25 ) + (i * 32 ), SCREEN_WIDTH * .025, 30, teamColorForeground);
      tft.fillRect( SCREEN_WIDTH * .95 , ( SCREEN_HEIGHT * .25 ) + (i * 32 ), SCREEN_WIDTH * .025, 30, teamColorForeground);

      // Draw Control Point Markers
      if (_teams[i].isActive) {
        tft.pushImage( ( SCREEN_WIDTH * .50 ) + 30, ( SCREEN_HEIGHT * .25 ) + (i * 32 ) + 2 , 25, 25, triangle, TFT_BLACK );
      }
      
      // Progress Bar
      double teamPoints = _teams[i].elapsedTimeInSeconds; 

      double ProgressBarEndLocation = SCREEN_WIDTH *.05 + SCREEN_WIDTH * .025;          // 18.00
      double ProgressBarStartLocation = SCREEN_WIDTH *.50;                              // 120.00
      double ProgressBarTotalSize = ProgressBarStartLocation - ProgressBarEndLocation;  // 102.00
      double ProgressBarStartLocationUpdate = ProgressBarStartLocation - (ProgressBarTotalSize * (teamPoints / pointsToWin ));
      double ProgressBarStartLocationUpdateWidth = (ProgressBarStartLocationUpdate - ProgressBarStartLocation) * -1;
      tft.fillRect( ProgressBarStartLocationUpdate , ( SCREEN_HEIGHT * .25 ) + ( i * 32 ), ProgressBarStartLocationUpdateWidth , 30 , teamColorForeground );

      // Number on top of Progress Bar
      tft.setTextDatum(BC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b);
      tft.setTextColor(TFT_WHITE);
      padding = tft.textWidth("999", GFXFF); // get the width of the text in pixels
      tft.setTextPadding(padding);
      tft.drawString( String( (int) teamPoints ) , ( SCREEN_WIDTH * .50 ) * .90 , ( SCREEN_HEIGHT * .45 ) + (i * 32 ) );

      // Team Letter
      tft.setFreeFont(&FreeSansBoldOblique12pt7b);
      tft.setTextColor(TFT_WHITE);
      padding = tft.textWidth("R", GFXFF); // get the width of the text in pixels
      tft.setTextPadding(padding);
      tft.drawString( String(_teams[i].teamName[0]) , SCREEN_WIDTH  * .88 , ( SCREEN_HEIGHT * .45 ) + (i * 32 ) );
    } else {
      // Serial.println("What garbage did I just read?");
      // Serial.println(_teams[i].teamName);
    }
  }

  // Game Mode Info Bottom Bar
  tft.setTextDatum(BC_DATUM);
  tft.setFreeFont(&GothamLight9pt7b);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  String bottomRowTextp1 = String(pointsToWin) + " Points to Win";
  tft.setTextPadding(tft.textWidth(bottomRowTextp1) + 30);
  tft.drawString( bottomRowTextp1 , SCREEN_WIDTH *.5 , SCREEN_HEIGHT * .86 );
  String bottomRowTextp2 = "Domination";
  tft.drawString( bottomRowTextp2 , SCREEN_WIDTH *.5 , SCREEN_HEIGHT );
}

void SendTeamTimerButtonPress(String team, int retries) {
  if (WiFi.status() == WL_CONNECTED) { //Check the current connection status

    String fullUrl = urlStartTeamTimerFull + "/" + team;
    Serial.println(fullUrl);
    http.begin(fullUrl);
    int httpCode = http.GET();                                        //Make the request

    if (httpCode > 0) { //Check for the returning code
      Serial.println("Sent Team Timer: " + String(team));
    } else {
      retries = retries - 1;
      if (retries <= 0) {
        Serial.println("WARNING: Something wrong with sending Button Press!");
        return;
      }
      Serial.println("Retrying to send button press");
      SendTeamTimerButtonPress(team, retries);
    }
  }
}

void StopTimers() {
  if (WiFi.status() == WL_CONNECTED) { //Check the current connection status

    String fullUrl = urlStopTimersFull;
    Serial.println(fullUrl);
    http.begin(fullUrl);
    int httpCode = http.GET();                                        //Make the request

    if (httpCode > 0) { //Check for the returning code
      Serial.printf("Stopped Timers!");
    } else {
      Serial.println("Failed to stop timers.");
    }
  }
}

uint16_t GetTeamColor (String teamName, bool isForProgressBar) {
  if (teamName == RED_TEAM) {
    if (isForProgressBar) {
      return TFT_RED;
    } else {
      return TFT_MAROON;
    }
  } else if (teamName == BLUE_TEAM) {
    if (isForProgressBar) {
      return TFT_BLUE;
    } else {
      return TFT_NAVY;
    }
  } 
  String teamNamestr = String(teamName);
  int nameLength = teamNamestr.length();
  Serial.println ("Drawing yellow for a team: " + String(teamName));
  Serial.println ("Length: " + String(nameLength));
  return TFT_YELLOW;
}

void DisplayWinnerScreen (String teamWinner, const char *ElapsedGameTimeFormatted) {
  String winningTeam, losingTeam;
    statusTimer.stop();
  for (int i = 0 ; i < sizeOfTeamsAllowed; i++) {
    if (teamWinner == teams[i].teamName) {
      winningTeam = String(teams[i].teamName) + ": " + teams[i].elapsedTimeInSeconds;
    } else {
      losingTeam = String(teams[i].teamName) + ": " + teams[i].elapsedTimeInSeconds;
    }
  }
  
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(ML_DATUM);
  tft.setSwapBytes(true);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.pushImage(SCREEN_WIDTH * .25 , 0,  41, 110, AWinnerIsYou);
  tft.drawString("A WINNER IS " + teamWinner + "!", SCREEN_WIDTH * .025 , SCREEN_HEIGHT * .9);
  tft.drawString("Time - " + String(ElapsedGameTimeFormatted), SCREEN_WIDTH * .50 , SCREEN_HEIGHT * .35 );
  tft.drawString(winningTeam, SCREEN_WIDTH * .60 , SCREEN_HEIGHT * .50 );
  tft.drawString(losingTeam, SCREEN_WIDTH * .60 , SCREEN_HEIGHT * .65 );
  postGameTimer.start();
  while (true) {
    postGameTimer.run();
  }
}

void SetDisplayToSleep() {
  Serial.println("Putting device to sleep");
  statusTimer.stop();
  delay(2000);
  int r = digitalRead(TFT_BL);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.fillScreen(TFT_BLACK);
  Serial.println("before iterator");
  for (int i = 0 ; i < 3 ; i++) {
    for (auto it = begin(MarioSleepGif); it != end(MarioSleepGif); ++it)
    {
        tft.pushImage( (tft.width() - 177 ) / 2, 0, 177, 135, *it);
        delay(30);
    }
  }        
  Serial.println("after iterator");
  tft.drawString("Going into Deep Sleep",  tft.width() / 2, 5 );
  espDelay(6000);
  digitalWrite(TFT_BL, !r);

  tft.writecommand(TFT_DISPOFF);
  tft.writecommand(TFT_SLPIN);
  //After using light sleep, you need to disable timer wake, because here use external IO port to wake up
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
  // esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 0);
  delay(200);
  esp_deep_sleep_start();
}