# cs-145-project-ESP32
This is the ESP32 code dedicated to the taking in the stoplight information from the web and translating it into the LEDs.

## Prerequisites
1. [PlatformIO extension for VSCode](https://platformio.org/install/ide?install=vscode)

## Steps
1. Within the `/lib/` folder, copy the `_Private.h` file and rename the new copy into `Private.h`
2. Fill up `Private.h` with the appropriate credentials of the WiFi, Websocket URL, and other necessary information.
3. Connect ESP32 to the device that you are using
4. Press the "upload" button for PlatformIO
5. You may need to press the boot button on the ESP32 as the terminal says "Connecting..." after pressing the upload button
6. Connect the correct pins to their slots on the breadboard (if using LEDs) or to the channel relay module (if using light bulbs)

## Team
This project was developed in fulfillment of the requirements for CS145.

The following are the developers of the project:
1. [Cedric John De Vera](https://github.com/shedwyck)
2. [Garlu Victor Eladio](https://github.com/SendCodesXD)
3. [Nathaniel Feliciano](https://github.com/natecomsci)
4. [Prince Harry Quijano](https://github.com/Harry2166)
5. [Gabriel Ramos](https://github.com/lemonjerome)
6. [Gerard Andrew Salao](https://github.com/gsalao)
