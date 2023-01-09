/****************************************************************************************************************************
 * Stuff used for the network device/shield integrated onto the board
 * https://www.waveshare.com/LAN8720-ETH-Board.htm
 * 
 * Useful info from the original library, but not actually use (since the defaults are sufficient):
#define ETH_PHY_TYPE   ETH_PHY_LAN8720    // Type of typedef enum { ETH_PHY_LAN8720, ETH_PHY_TLK110, ETH_PHY_RTL8201, ETH_PHY_DP83848, ETH_PHY_DM9051, ETH_PHY_KSZ8081, ETH_PHY_MAX } eth_phy_type_t;
#define ETH_PHY_MDC    23    // Pin# of the I²C clock signal for the Ethernet PHY
#define ETH_PHY_MDIO   18    // Pin# of the I²C IO signal for the Ethernet PHY
#define ETH_CLK_MODE   ETH_CLOCK_GPIO0_IN  //  ETH_CLOCK_GPIO17_OUT
#define SHIELD_TYPE   "ETH_PHY_LAN8720" 
#define ETH_PHY_ADDR   1    // I²C-address of Ethernet PHY (0 or 1 for LAN8720)
#define ETH_PHY_POWER 16    // Pin# of the enable signal for the external crystal oscillator (-1 to disable for internal APLL source)
 *
 * Time zone config: https://github.com/esp8266/Arduino/blob/master/cores/esp8266/TZ.h
 * example: https://forum.arduino.cc/t/time-library-functions-with-esp32-core/515397/17
 * SimpleTime: https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/Time/SimpleTime/SimpleTime.ino
 * API:  https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system_time.html
 * Structure: https://ftp.gnu.org/old-gnu/Manuals/glibc-2.2.3/html_chapter/libc_21.html
 *
 *****************************************************************************************************************************/

#ifndef lan8720_h
#define lan8720_h


//###################################################################

////#include <ETH.h>   //CANNOT declare here; declare before any functions are defined
#include <WiFi.h>
#include <ESPmDNS.h>
//#include <Time.h>
//#include "time.h"
#include "sntp.h"

const long  gmtOffset_sec =  -8 * 60 * 60 ; // pacific = -8 hour
//const int   daylightOffset_sec = 3600;
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const char* time_zone = "PST8PDT,M3.2.0,M11.1.0";  // TimeZone rule for TZ_America_Los_Angeles including daylight adjustment rules
//unsigned long ntpLastUpdate = 0;  // when did ntp last sync the time
// declared globally  time_t ntpTimeLastUpdate;  // when did ntp last sync the time

//-------------------------------------------------------------------
void ETH_event_handler(WiFiEvent_t event){
  if (isVerbose) {Serial.print("ETH_event_handler: Executing on core: "); Serial.print(xPortGetCoreID()); Serial.print(";  priority: "); Serial.println(uxTaskPriorityGet(NULL));}

  switch (event)
  {
    case ARDUINO_EVENT_ETH_START:
      if (isVerbose) {Serial.println(F("ETH starting"));}
      ETH.setHostname(dnsAliasName);
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      if (isVerbose) {Serial.println(F("ETH connected"));}
      wt32EthConnected = true;
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      if (isVerbose) {Serial.println(F("ETH got IP"));}
      Serial.print(F("URL: http://")); Serial.print(ETH.getHostname()); Serial.println(F(".local"));
      Serial.print(F("ETH MAC: "));    Serial.print(ETH.macAddress());
      Serial.print(F(", IPv4: "));     Serial.print(ETH.localIP());
      if (ETH.fullDuplex())
      {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      connectToMqtt();
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println(F("ETH lost connection"));
      wt32EthConnected = false;
      xTimerStop(mqttReconnectTimer, 0);   // ensure we don't reconnect to MQTT when no ETH
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println(F("ETH stops"));
      wt32EthConnected = false;
      xTimerStop(mqttReconnectTimer, 0);   // ensure we don't reconnect to MQTT when no ETH
      break;

    default:
      break;
  }
}

//-------------------------------------------------------------------
// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval *t)
{
  if (isVerbose){Serial.print(F("Got time adjustment from NTP!  ")); Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");}
//  ntpLastUpdate = millis();
  ntpTimeLastUpdate = time (NULL);

  if (isRtcExists) {
    //update the attached clock
    rtcTime.adjust(t->tv_sec);  //setting RTC to UTC time
    if(isVerbose){Serial.print(F("Compare new to to set time.  NTP: ")); Serial.print(t->tv_sec); Serial.print(F(".  RTC: ")); Serial.println(rtcTime.now().unixtime());}
  }
}

//###################################################################
void lan8720_setup(){

    // set notification call-back function
  sntp_set_time_sync_notification_cb( timeavailable );
  
  // NTP server address aquired via DHCP,
  // NOTE: This call should be made BEFORE esp32 aquires IP address via DHCP,
  //   otherwise SNTP option 42 would be rejected by default.
  sntp_servermode_dhcp(1);    // (optional)

  // handle TimeZones with daylightOffset. specify a environmnet variable with TimeZone definition including daylight adjustmnet rules.
  // A list of rules for your zone could be obtained from https://github.com/esp8266/Arduino/blob/master/cores/esp8266/TZ.h
  configTzTime(time_zone, ntpServer1, ntpServer2);
  
  // WT32_ETH01_onEvent();  // cannot uses OEM function; need to add MQTT to case statement  https://github.com/khoih-prog/AsyncWebServer_WT32_ETH01/blob/main/src/AsyncWebServer_WT32_ETH01.cpp
  // To be called before ETH.begin()
  WiFi.onEvent(ETH_event_handler);

  //bool begin(uint8_t phy_addr=ETH_PHY_ADDR, int power=ETH_PHY_POWER, int mdc=ETH_PHY_MDC, int mdio=ETH_PHY_MDIO, 
  //           eth_phy_type_t type=ETH_PHY_TYPE, eth_clock_mode_t clk_mode=ETH_CLK_MODE);
  //ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER, ETH_PHY_MDC, ETH_PHY_MDIO, ETH_PHY_TYPE, ETH_CLK_MODE);
  ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER);

  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp32.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (MDNS.begin(dnsAliasName)) {
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up mDNS responder!");
  }

  //init and get the time
  // replace by configTzTime, above. configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

//-------------------------------------------------------------------
//https://ftp.gnu.org/old-gnu/Manuals/glibc-2.2.3/html_chapter/libc_21.html
void lan8720_loop(){
//  if (ntpTimeLastUpdate) {
//    struct tm timeFirst;
//    struct tm timeDup;
//    struct tm timeLater;
//    time_t time1;
//    time_t timeA;
//    time_t time2;
//    time1 = time (NULL);
//    vTaskDelay(15000 / portTICK_PERIOD_MS);
//    time2 = time (NULL);
//    timeA = time1;
//    time_t epochUnix;
//    
//  if(getLocalTime(&timeFirst)){
//    Serial.print("Got the time: ");
//    timeFirst.tm_year = 2008-1900;  // Year - 1900
//    epochUnix = mktime(&timeFirst);
//    Serial.print(epochUnix);
//    Serial.print(",  ");
//    Serial.println(&timeFirst, "%A, %B %d %Y %H:%M:%S");
//    timeFirst.tm_mday += 1;   //60*60*24;   //https://forum.arduino.cc/t/time-library-functions-with-esp32-core/515397/18
//    Serial.println(&timeFirst, "%A, %B %d %Y %H:%M:%S");
//  }
//        if (time1==timeA) {
//          Serial.println("time1==timeA");
//        } else {
//          Serial.println("time1=!!=timeA");
//        }
//        if (time2 > time1) {
//          Serial.println("time2 > time1");
//        } else {
//          Serial.println("!!!time2 > time1");
//        }
//    vTaskDelay(15000 / portTICK_PERIOD_MS);
//  }
}

#endif      //lan8720_h
