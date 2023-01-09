/****************************************************************************************************************************
   The sketch is a foundation to build upon.
  It includes common management and communication functions.

  To use, connect a WT32-ETH01 to a network cable, power on, and upload this sketch (no additional circuits needed)

  Prerequisites. install the following into Arduino IDE:
  - AsyncTCP:                   https://github.com/me-no-dev/AsyncTCP
  - AsyncMqtt_Generic:          https://github.com/khoih-prog/AsyncMQTT_Generic
  - AsyncWebServer_WT32_ETH01:  https://github.com/khoih-prog/AsyncWebServer_WT32_ETH01
  - ArduinoJson:                https://arduinojson.org/

  Note: this is NOT a beginner level sketch.  Use the examples of the libraries while learning.
        Certainly if the tested and maintained examples do not work, THIS sketch certainly will not!
  
  This sketch is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

  To use:
  - Connect the WT32-ETH01 (https://www.amazon.com/gp/product/B08WC15NWZ) to USB and ethernet cable
  - Edit this sketch and update the variables for
    - MQTT_HOST
    - MQTT_PORT
    - SDA_0
    - SCL_0
  - Upload this sketch, and reboot
  - Observe output to the serial monitor. Look for the URL (default: http://ESP32TBD.local)
  - Observe output to the serial monitor. Notice every 30 seconds messages are received from the MQTT broker
  - With a browser open the URL
      content: Upload a file with the directory's upload function or try the default page

  - Click the link to the default page
    - observe the date and time of the source code (enables keeping track of what is running on the device)
    - Note: date and time from the device - updated every 30 seconds
    - observe the utilization
      - Heap: memory avialble
      - Idle: how many ticks are unused on each core
    - observe the addresses of any attached I2C devices
    - click RX or TX and observe the lights on the device.

  - With a second browser open the URL and the default page. click RX or TX and observe both browsers update.

  - Using Notepad, or equivellent, create a file "index.html".  (name required to be exactly that)
    Paste in the suggested content below.
  - In the browser open the URL, and click th elink for "directory"
  - Click th e"choose file" button
  - select the file just created
  - Click the "upload" button
  - return to the URL page (i.e., click the link "home")
  - observe the new content
  - [optional] create a "favicon.ico" file and upload to have a unique icon on the tab in the browser
  - [optional] export a binary of this sketch and upload that .bin file for OTA updating

  Suggested content for "index.html" file:
      <!DOCTYPE HTML><html>
      <head>
        <title>ESP Web Server</title>
      </head>
      <body>
        <h1>Test Page</h1>
        <hr>
        <a href="/config">config</a>
      </body>
      </html>
      
 * 20221213 function: paramterLookUp
 * 20221221 add systemConfig with isVerbose
 * 20230101 add scheduling
 * 20230103 ability to update MQTT settings
 * 20230105 add RTC
 * 
 * 
 * 
 * I2C addresses:
 *   x68: RTC DS3231
 *****************************************************************************************************************************/

//##################################################################

#define _ISVERBOSE_    //comment-out this line to suppress Serial messages

#ifdef _ISVERBOSE_
  #define _ASYNC_WEBSERVER_LOGLEVEL_       3
  bool isVerbose = true;   // isVerbose is redeclared as a variable so it can be changed programatically
#else
  #define _ASYNC_WEBSERVER_LOGLEVEL_       0
  bool isVerbose = false;
#endif

#include "system.h"
#include "mqttfile.h"   //before lan8720 due to dependancies
#include "rtcfile.h"
#include "lan8720.h"
#include "schedule.h"
#include "websocket.h"
#include "webserver.h"
#include "actions.h"

//##################################################################
//-------------------------------------------------------------------
void setup()
{

//##################################################################
  system_setup();
  lan8720_setup();
  rtc_setup();
  webserver_setup();
  websocket_setup();
  mqtt_setup();
  schedule_setup();
  actions_setup();
//##################################################################

}

//-------------------------------------------------------------------
void loop()
{

//##################################################################
  system_loop();
  lan8720_loop();
  rtc_loop();
  webserver_loop();
  websocket_loop();
  mqtt_loop();
  schedule_loop();
  actions_loop();
//##################################################################

//  Serial.println("Doing the loop");
  vTaskDelay(15000 / portTICK_PERIOD_MS);   // force the loop task to release resources periodically so the core monitoring thread can run
}
