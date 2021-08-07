# Nerf Domination Tracker

![IMG](ReadmeMedia/IMG_20210523_214103_1.jpg?raw=true "IMG")

## Hardware:
* TTGO TDisplay (Arduino-based Microcontroller)
  * [Example Link](https://www.aliexpress.com/item/4000059428373.html?spm=a2g0s.9042311.0.0.493c4c4dIXBpsw)
* 3D Printed Case for the TTGO TDisplay
  * Print out the files in [3dCaseSTLs](https://github.com/ccoane/NerfDominationTracker/tree/master/3dCaseSTLs)
    * Print out 1 of each file but 2 of the Buttons
  * 1 x M3x4 Screw
  * 1 x M3x30 Screw
  * 1 x M3 Nut

## Setup:
* Install Visual Studio Code (VS Code)
* Install the PlatformIO Extension in VS Code
* Download and install the [CP210x USB to UART Driver](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers)
* Open the project in VS Code and Build the project.
* Navigate to the TFT_eSPI Library folder in your Project directory
  * Example: \CPU_Monitor_Display\\.pio\libdeps\esp32dev\TFT_eSPI
* Make the following changes to the User_Setup_Select.h file:
  * Comment out "#include <User_Setup.h>"
  * Uncomment "#include <User_Setups/Setup25_TTGO_T_Display.h>"
  * Save Settings
