# NerfDominationTracker - Very much still a WIP

[![Video](https://img.youtube.com/vi/8_mOT_Istnk/0.jpg)](https://www.youtube.com/watch?v=8_mOT_Istnk)

Steps:
- Install PlatformIO
- Download Library "TFT_eSPI"
- Navigate to the TFT_eSPI Library folder in your users directory
-- In Windows, this would be C:\Users\YOUR_USERNAME\.platformio\lib\TFT_eSPI_restOfFolderName
- Comment out the default settings #include <User_Setup.h> , select #include <User_Setups/Setup25_TTGO_T_Display.h> , Save Settings
- Don't track changes on the WiFiCredentials file:  git update-index --assume-unchanged WifiCredentials.h
