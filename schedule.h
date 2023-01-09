/******************************************************************
 * 
******************************************************************/

#ifndef schedule_h
#define schedule_h

//###################################################################

#define SCHEDULE_ONSECOND_Parameter "slOnSec"
#define SCHEDULE_ONDURATION_Parameter "slOnDrtn"
#define SCHEDULE_ISON_Parameter "slIsOn"
#define SCHEDULE_ISENABLED_Parameter "slIsEn"

bool isScheduleOn = false;  // is the accessory supposed to be running
bool isScheduleEnabled = true; // is the schedule supposed to be controlling the accessories
unsigned long scheduleOnSecond = 22*60*60; // Which second of the day to turn on
unsigned long scheduleOnDuration = 60*60;   //6*60*60;  // =3600=1hour. elapsed number of seconds for the accessory to remain on
time_t scheduleOnTime = scheduleOnSecond;  // when to turn on the accessory
time_t scheduleOffTime = scheduleOnSecond+scheduleOnDuration;  // when to turn off the accessory.  This is calculated: OnTime + Duration = OffTime
struct tm timeBuffer;   // reserve space to avoid dynamically a tm structure (which is not thread safe)
//struct tm timeNowBuffer;  // another buffer - this to hold the current time

//-------------------------------------------------------------------
//return schedule as a JSON string
String scheduleJson() {
  if (isVerbose) {Serial.print("scheduleJson: Executing on core: "); Serial.print(xPortGetCoreID()); Serial.print(";  priority: "); Serial.println(uxTaskPriorityGet(NULL));}

  StaticJsonDocument<450> thisJsonDoc;
  thisJsonDoc["SL"]["OnSecond"] = scheduleOnSecond;
  thisJsonDoc["SL"]["OnDuration"] = scheduleOnDuration;
  thisJsonDoc["SL"]["OnTime"] = scheduleOnTime;
  thisJsonDoc["SL"]["OffTime"] = scheduleOffTime;
  thisJsonDoc["SL"]["isOn"] = isScheduleOn;
  thisJsonDoc["SL"]["isEnabled"] = isScheduleEnabled;
  String outJson;
  serializeJson(thisJsonDoc, outJson);
  if (isVerbose) {Serial.print(F("SCHEDULE JSON: ")); Serial.println(outJson);}
//Serial.print(F("SCHEDULE JSON: ")); Serial.println(outJson);
  return outJson;
}

//-------------------------------------------------------------------
// check if it is time to turn off (or on) 
// assumes a daily schedule - each day turn on at OnTime and turn off Duration later
void scheduleCheck(void) {
  if(!isScheduleEnabled){
    // schedule has been manually paused
    return;   // nothing to do, so exit
  }
  if(getLocalTime(&timeBuffer)){
    time_t timeNow = 0; 
    timeNow = mktime(&timeBuffer);
    timeBuffer.tm_hour  = 0;
    timeBuffer.tm_min  = 0;
    timeBuffer.tm_sec  = 0;
    time_t todayMidnight = 0; 
    todayMidnight = mktime(&timeBuffer);
//    if ((todayMidnight+scheduleOnSecond) > scheduleOnTime && (todayMidnight+scheduleOnSecond) > scheduleOffTime) {
//      // all schedules are in the past - reset both (usually only happens immediately after bootup
//      scheduleOnTime = todayMidnight+scheduleOnSecond;
//      scheduleOffTime = scheduleOnTime + scheduleOnDuration;
//    }
    if (scheduleOnTime > timeNow) {
      // on is future
      if (scheduleOffTime > timeNow) {
        // on is future, off is future
        // accessory should be off
        //isScheduleOn = false;
        setScheduleStatus(false);
      } else {
        // on is future, off is past
        // error this shouldn't happen. Erase current times
        scheduleOnTime = 0+scheduleOnSecond;
        scheduleOffTime = scheduleOnTime + scheduleOnDuration;
      }
    } else {
      // on is past
      if (scheduleOffTime > timeNow) {
        // on is past, off is future
        // accessory should be on
        //isScheduleOn = true;
        setScheduleStatus(true);
      } else {
        // on is past, off is past
        // accessory should be off
        //isScheduleOn = false;
        setScheduleStatus(false);
        // reset schedule times
        scheduleOnTime = todayMidnight+scheduleOnSecond;
        scheduleOffTime = scheduleOnTime + scheduleOnDuration;
      }
    }
    // save changes in persistant storage
    
//localtime_r(&scheduleOnTime, &timeBuffer);   // https://en.cppreference.com/w/c/chrono/localtime
//Serial.print("scheduleOnTime: "); Serial.println(&timeBuffer, "%A, %B %d %Y %H:%M:%S");
//localtime_r(&scheduleOffTime, &timeBuffer);   // https://en.cppreference.com/w/c/chrono/localtime
//Serial.print("scheduleOffTime: "); Serial.println(&timeBuffer, "%A, %B %d %Y %H:%M:%S");
//Serial.print("isScheduleOn: "); Serial.println(isScheduleOn);

  } else {
    // time is not yet available
    Serial.println(F("Unable to obtain time. Schedule NOT updated!"));
  }
}

//-------------------------------------------------------------------
// sets the variables when the user updates the webpage
void scheduleUpdate (unsigned long  startSecond,unsigned long durationSecond){
  unsigned long result;
  result = systemParameter.putULong(SCHEDULE_ONSECOND_Parameter, startSecond);
  result = systemParameter.putULong(SCHEDULE_ONDURATION_Parameter, durationSecond);
  scheduleOnSecond =   systemParameter.getULong(SCHEDULE_ONSECOND_Parameter,   scheduleOnSecond);
  scheduleOnDuration = systemParameter.getULong(SCHEDULE_ONDURATION_Parameter, scheduleOnDuration);
  if(getLocalTime(&timeBuffer)){
    time_t timeNow = 0; 
    timeNow = mktime(&timeBuffer);
    timeBuffer.tm_hour  = 0;
    timeBuffer.tm_min  = 0;
    timeBuffer.tm_sec  = 0;
    time_t todayMidnight = 0; 
    todayMidnight = mktime(&timeBuffer);
    scheduleOnTime = todayMidnight+scheduleOnSecond;
    scheduleOffTime = scheduleOnTime + scheduleOnDuration;
    scheduleCheck();
  } else {
    // time is not yet available
    Serial.println(F("Unable to obtain time. Schedule NOT updated!"));
  }
}

//-------------------------------------------------------------------
// a simple fuction to set the override parameter
void scheduleEnableUpdate(bool newSetting){
  bool result;
  result = systemParameter.putBool(SCHEDULE_ISENABLED_Parameter, newSetting);
  isScheduleEnabled =  systemParameter.getBool (SCHEDULE_ISENABLED_Parameter,  isScheduleEnabled);
  if (!isScheduleEnabled) {
    // in override mode need to also set/retrieve the on-state
    result = systemParameter.putBool(SCHEDULE_ISON_Parameter, isScheduleOn);
    isScheduleOn = systemParameter.getBool (SCHEDULE_ISON_Parameter, isScheduleOn);
  }
}

//###################################################################
void schedule_setup(){
  //retrieve storge parameters
  scheduleOnSecond =   systemParameter.getULong(SCHEDULE_ONSECOND_Parameter,   scheduleOnSecond);
  scheduleOnDuration = systemParameter.getULong(SCHEDULE_ONDURATION_Parameter, scheduleOnDuration);
  isScheduleEnabled =  systemParameter.getBool (SCHEDULE_ISENABLED_Parameter,  isScheduleEnabled);
  if (!isScheduleEnabled) {
    // in override mode need to also retrieve the on-state
    bool result = systemParameter.getBool (SCHEDULE_ISON_Parameter, isScheduleOn);
    setScheduleStatus(!result);   //trick the function into seeing a new value
    setScheduleStatus(result);   //ensure the accessories match the manual set state
  }
}

//-------------------------------------------------------------------
void schedule_loop() {

}

#endif      //schedule_h
