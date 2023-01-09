/******************************************************************
 * for Preferences: https://www.tutorialspoint.com/esp32_for_iot/esp32_for_iot_preferences.htm
 * max size of Preferences is 500k  https://github.com/espressif/arduino-esp32/blob/master/libraries/Preferences/src/Preferences.h
 * 
******************************************************************/

#ifndef system_h
#define system_h

//###################################################################

#if !( defined(ESP32) )
  #error This code is designed for WT32_ETH01 to run on ESP32 platform! Please check your Tools->Board setting.
#endif

#define INCLUDE_vTaskDelay  1   // make sure the compiler includes the vTaskDelay 

#define I2C_Freq 100000
#define SDA_0 15   //I2C data pin
#define SCL_0 14   //I2C clock pin

#define RX_LED   5    //LED3: Serial port indicator, RXD2(IO5) When there is data flow, the indicator light is on;
#define TX_LED  17    //LED4: Serial port indicator, TXD2(IO17) When there is data flow, the indicator light is on;
#define RELAY_PIN  32 //Used in LED_Controller to enable the relay

#define systemParameterNamespace "LaudixESP32"   // Each app module needs to use a namespace name to prevent key name collisions. limited to 15 chars.
#define HOSTNAME_Parameter "hostname"   // used to find/set DNS_ALIAS_NAME
#define HOSTNAME_Size 16    // Array length of name
bool wt32EthConnected = false;   // is the cable connected to the RJ45 jack
char dnsAliasName[HOSTNAME_Size];   //point browser to https://<DNS_ALIAS_NAME>.local

static char versionDate[] = __DATE__;
static char versionTime[] = __TIME__;
static char versionVersion[] = __VERSION__;
static char versionFile[] = __FILE__;

unsigned int restartCount = 0;   //how many times has this device rebooted

#define i2cMaxDevices 16            // maximum number of devices expected to be connected simultaneously
byte i2cAddresses[i2cMaxDevices];   // array of addresses of detected device addresses

volatile unsigned long systemCore0IdleCount = 0;   // how many times/ticks has core been idle
volatile unsigned long systemCore1IdleCount = 0;   // how many times/ticks has core been idle
unsigned long systemCore0IdleCountLastReset = 1;    // when was the count last reset
unsigned long systemCore1IdleCountLastReset = 1;    // when was the count last reset
float core0PercentIdle = 0;   // store the calculated utilization
float core1PercentIdle = 0;   // store the calculated utilization
unsigned long systemCheckLastUpdate = 0;  // when were the system variables last updated

#include <Preferences.h>
#include <Wire.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
//#include <time.h>   //appears to be included by default


//#include "AsyncTCP.h"
#include <ETH.h>  // declare this BEFORE any functions or else error: variable or field  declared void
#include <AsyncWebServer_WT32_ETH01.h>


Preferences systemParameter;
TwoWire I2C_0 = TwoWire(0);   // used to detect I2C addresses
struct tm timeinfo;   //holds 'current' time. only updated when the websocket refreshes

//declare variables used accross multiple files
time_t ntpTimeLastUpdate;  // when did ntp last sync the time

//declare functions used across multiple files
String scheduleJson(void);
void setScheduleStatus(bool);

//-------------------------------------------------------------------
//return all metrics as a JSON string
String systemStatusJson() {
  if (isVerbose) {Serial.print("systemStatusJson: Executing on core: "); Serial.print(xPortGetCoreID()); Serial.print(";  priority: "); Serial.println(uxTaskPriorityGet(NULL));}

  char timeStringBuff[50]; //50 chars should be enough
//  struct tm timeinfo;   //declared globally
  bool gotTime = getLocalTime(&timeinfo);
  if(gotTime){
    strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
    //Serial.println(timeStringBuff);
  } else {
    Serial.println(F("Failed to obtain time"));
  }

  StaticJsonDocument<450> thisJsonDoc;
  thisJsonDoc["SS"]["Now"] = timeStringBuff;
  thisJsonDoc["SS"]["Runtime"] = millis();
  thisJsonDoc["SS"]["FreeHeap"] = ESP.getFreeHeap();
  thisJsonDoc["SS"]["TotalHeap"] = ESP.getHeapSize();
  thisJsonDoc["SS"]["LargestBlock"] = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);   // https://github.com/espressif/arduino-esp32/issues/5346
  thisJsonDoc["SS"]["core0PercentIdle"] = core0PercentIdle;
  thisJsonDoc["SS"]["core1PercentIdle"] = core1PercentIdle;
  thisJsonDoc["SS"]["RX_LED"] = !digitalRead(RX_LED);
  thisJsonDoc["SS"]["TX_LED"] = !digitalRead(TX_LED);
  String outJson;
  serializeJson(thisJsonDoc, outJson);
  if (isVerbose) {Serial.print(F("SYSTEM STATUS JSON: ")); Serial.println(outJson);}
//Serial.print(F("SYSTEM STATUS JSON: ")); Serial.println(outJson);
  return outJson;
}

//-------------------------------------------------------------------
//return all settings as a JSON string
String systemConfigJson() {
  if (isVerbose) {Serial.print("systemConfigJson: Executing on core: "); Serial.print(xPortGetCoreID()); Serial.print(";  priority: "); Serial.println(uxTaskPriorityGet(NULL));}

  StaticJsonDocument<450> thisJsonDoc;
  thisJsonDoc["SC"]["isVerbose"] = isVerbose;
  String outJson;
  serializeJson(thisJsonDoc, outJson);
  if (isVerbose) {Serial.print(F("SYSTEM CONFIG JSON: ")); Serial.println(outJson);}
//Serial.print(F("SYSTEM CONFIG JSON: ")); Serial.println(outJson);
  return outJson;
}

//-------------------------------------------------------------------
//return list of files as a JSON string
// https://arduinojson.org/v6/api/jsonarray/
String filesJson() {

  StaticJsonDocument<450> thisJsonDoc;
  int indx = 0;

  File d = SPIFFS.open("/");
  if (d.isDirectory()) {
    char tmpName[33];    // SPIFFS permits 32-char name length
    File f = d.openNextFile();
    while (f) {
      strcpy(tmpName,f.name());  //something wrong with the SPIFFs name output - cause the json to become corrupt. running it through another char array fixes it
//      thisJsonDoc["Files"][indx] = f.name();  // results in corrupt json
      thisJsonDoc["Files"][indx] = tmpName;
      indx++;
      f = d.openNextFile();
    }
    f.close();
  }
  d.close();

  String outJson;
  serializeJson(thisJsonDoc, outJson);
  if (isVerbose) {Serial.print(F("FILES JSON: ")); Serial.println(outJson); }
  
  return outJson;
}

//-------------------------------------------------------------------
// check if a sing I2c address is responding
// assume that checking the address of a ACTIVE device will disrupt its communications (the device might recover)
// returns //https://forum.arduino.cc/t/wire-endtransmission-returns-error-code-5-which-is-undocumented/896221/3
//        I2C_ERROR_OK=0,    // 0  //success
//        I2C_ERROR_DEV,     // 1
//        I2C_ERROR_ACK,     // 2  //typical for an address with no device
//        I2C_ERROR_TIMEOUT, // 3
//        I2C_ERROR_BUS,     // 4
//        I2C_ERROR_BUSY,    // 5
//        I2C_ERROR_MEMORY,  // 6
//        I2C_ERROR_CONTINUE,// 7
//        I2C_ERROR_NO_BEGIN // 8
byte i2cAddressError(byte checkAddr){ //I2C address range 0 to 127
  // The i2c_scanner uses the return value of
  // the Write.endTransmisstion to see if
  // a device did acknowledge to the address.
  I2C_0.beginTransmission(checkAddr);
  return I2C_0.endTransmission();
}

//-------------------------------------------------------------------
void getI2CAddresses(void){
  I2C_0.begin(SDA_0 , SCL_0 , I2C_Freq);
  memset(i2cAddresses, 0, sizeof(i2cAddresses));   //sets all of the bytes associated with i2cAddresses to 0
  // loop through all possible addresses
  byte error, address;
  int nDevices;
  if (isVerbose) {Serial.println("Scanning...");}
  nDevices = 0;
  for(address = 1; address < 127; address++ )
  {
    error = i2cAddressError(address);
//Serial.print("I2C address: "); Serial.print(address, HEX); Serial.print("  result: "); Serial.println(error);
    if (error == 0) {
      if (isVerbose) {
        Serial.print("I2C device found at address 0x");
        if (address<16)
          Serial.print("0");
        Serial.print(address,HEX);
        Serial.println("  !");
      }
      i2cAddresses[nDevices] = address;
      nDevices++;
    } else if (error==4) {
      Serial.print("Unknown I2C error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    if (isVerbose) {Serial.println("No I2C devices found\n");}
  else
    if (isVerbose) {Serial.println("done\n");}
  I2C_0.end();
}

//-------------------------------------------------------------------------------------------
void core0Idle(void *params) {
  while(1){
    systemCore0IdleCount++;
    vTaskDelay( 1 );  // delay for only 1 tick (but DO delay to release the focus)
  }
}

//-------------------------------------------------------------------------------------------
void core1Idle(void *params) {
  while(1){
    systemCore1IdleCount++;
    vTaskDelay( 1 );  // delay for only 1 tick (but DO delay to release the focus)
  }
}

//-------------------------------------------------------------------
// calculate the rates for both cores and store each
void coreUtilizationCalculation(){
  if (isVerbose) {Serial.print("coreUtilizationCalculation: Executing on core: "); Serial.print(xPortGetCoreID()); Serial.print(";  priority: "); Serial.println(uxTaskPriorityGet(NULL));}
  unsigned long checkstatusLastUpdate = systemCheckLastUpdate;
  core0PercentIdle = (systemCore0IdleCount * 100.0) / ((checkstatusLastUpdate - systemCore0IdleCountLastReset) * 1.0);
  core1PercentIdle = (systemCore1IdleCount * 100.0) / ((checkstatusLastUpdate - systemCore1IdleCountLastReset) * 1.0);
  systemCore0IdleCountLastReset = checkstatusLastUpdate;
  systemCore0IdleCount = 0;
  systemCore1IdleCountLastReset = checkstatusLastUpdate;
  systemCore1IdleCount = 0;
  systemCheckLastUpdate = millis();
}

//-------------------------------------------------------------------
void espRestart() {
Serial.println("#####################################################");
Serial.println("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
Serial.println("                  OTA REBOOT ");
Serial.println("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
Serial.println("#####################################################");

    //delay(1000);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    yield();
    //delay(1000);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    yield();
    ESP.restart();
}

//-------------------------------------------------------------------
// If refresh, store default, update result, return size
// if refresh and sizeof default is 0, remove parameter
// else, look up value stored in parameterName, if not exist, add with default content
// update result with found value or default
// return size of result
//    systemParameter.x.clear();   //delete all keys. the ".x" is added to prevent accidentally running this command. only remove if needed
int parameterLookUp(char* parameterName, char* result, int resultMax, const char* defaultValue, bool refresh = false){
  unsigned int pSize = 0;   //size of read value
  if (refresh) {
    // force use of default value
    if (strlen(defaultValue)<=0){
      // remove the stored value
      systemParameter.remove(parameterName); 
      strncpy(result, "", resultMax);
      pSize = 0;
      Serial.print(F("Parameter, ")); Serial.print(parameterName); Serial.println(F(", has been REMOVED"));
    } else {
      // store the default, and return it
      systemParameter.putBytes(parameterName, defaultValue, strlen(defaultValue));
      int keySize = systemParameter.getBytesLength(parameterName);
      pSize = systemParameter.getBytes(parameterName, result, keySize);
      result[pSize] = '\0';   //getBytes fails to add the terminating character
      if (isVerbose){Serial.print(F("Parameter, ")); Serial.print(parameterName); Serial.print(F(", has been REPLACED with ")); Serial.print(result); Serial.print("  "); Serial.println(pSize);}
    }
  } else {
    int keySize = 0;
    if (systemParameter.isKey(parameterName)) {
      //key already exists; is anything in it?
      keySize = systemParameter.getBytesLength(parameterName);
    }
    
    if (keySize > 1){
      // key exists, use it
      pSize = systemParameter.getBytes(parameterName, result, keySize);
      result[pSize] = '\0';   //getBytes fails to add the terminating character
      if (isVerbose){Serial.print(F("Parameter, ")); Serial.print(parameterName); Serial.print(F(", has been READ as: ")); Serial.print(result); Serial.print("  "); Serial.println(keySize);}
    } else {
      // name has not yet been set. do it now
      systemParameter.putBytes(parameterName, defaultValue, strlen(defaultValue));
      int keySize = systemParameter.getBytesLength(parameterName);
      pSize = systemParameter.getBytes(parameterName, result, keySize);
      result[pSize] = '\0';   //getBytes fails to add the terminating character
      Serial.print(F("New parameter, ")); Serial.print(parameterName); Serial.print(F(", has been set as: ")); Serial.print(result); Serial.print("  "); Serial.println(keySize);
    }
  }
  return pSize;
}

//###################################################################
void system_setup() {
  Serial.begin(115200);
  pinMode(RX_LED,OUTPUT);
  pinMode(TX_LED,OUTPUT);
  pinMode(RELAY_PIN,OUTPUT);
  digitalWrite(RX_LED,HIGH);
  digitalWrite(TX_LED,HIGH);
  digitalWrite(RELAY_PIN,LOW);
    
  long serialInterval = 10000;   // interval to wait
  unsigned long previous = millis();   // hold current 'time'
  while (!Serial) {
    // wait for serial port to connect. Needed for built-in USB port only
    if (millis() - previous >= serialInterval) {
      // exceeded the allowed wait-time; give up and don't use the serial communications
      break;
    } else {
      delay(50);
    }
  }
  delay(250);  //it seems that occassionally serial is ready a few moments before it can be written to
  Serial.println();

  Serial.print( F("Compiled: "));  //https://forum.arduino.cc/t/displaying-date-time-and-ide-compiler-version-in-serial-monitor/154290/3
  Serial.print( F(__DATE__));
  Serial.print( F(", "));
  Serial.print( F(__TIME__));
  Serial.print( F(", "));
  Serial.println( F(__VERSION__));
  Serial.print( F("From: "));
  Serial.println( F(__FILE__));

  Serial.println(F("Initializing ... "));

  systemParameter.begin(systemParameterNamespace, false);   // open storage in RW-mode (second parameter = false).

  // increase the count of reboots
  restartCount  = systemParameter.getUInt("bootcount", 0);
  restartCount++;
  systemParameter.putUInt("bootcount", restartCount);

  int rSize = parameterLookUp(HOSTNAME_Parameter, dnsAliasName, sizeof(dnsAliasName), "ESP32TBD");   //add true as last parameter to update stored value

  getI2CAddresses();
  Serial.print("i2cAddresses: ");
  char tmpData[320]; 
  strcpy(tmpData, "no I2C addresses found");
  for (int i=0; i<sizeof(i2cAddresses); i++){
    if (i2cAddresses[i] == 0) { break; }
    char tmpAddr[8];
    if (i == 0) {
      // for the 1st entry overwrite the warning message
      sprintf(tmpAddr, "x%02X", i2cAddresses[i]);
      strcpy(tmpData, tmpAddr);
    } else {
      // append next address
      strcat(tmpData, ", ");
      sprintf(tmpAddr, "x%02X", i2cAddresses[i]);
      strcat(tmpData, tmpAddr);
    }
  }
  Serial.println(tmpData);

  // Setup filesystem
  if (!SPIFFS.begin(true)) Serial.println("Mounting SPIFFS failed");

  //super tiny tasks to determine how often priority 0 is available (more = more CPU idle time)
  xTaskCreatePinnedToCore(core0Idle, "core0Idle", 640, NULL, 0, NULL, 0);  //https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html
  xTaskCreatePinnedToCore(core1Idle, "core1Idle", 640, NULL, 0, NULL, 1);
//  xTaskCreatePinnedToCore(coreUtilizationTask, "coreUtilization", 864, NULL, 2, NULL, 0);  //priorit:2, core 0
  
}

//-------------------------------------------------------------------
void system_loop() {

}

#endif      //system_h
