/******************************************************************
 * 
******************************************************************/

#ifndef actions_h
#define actions_h

//###################################################################

//-------------------------------------------------------------------
void actionsTask(void *params){
  int tmpSize;
  int maxTopicSize = 45;
  int maxValueSize = 7;
  char tmpTopic[maxTopicSize];
  char tmpValue[maxValueSize];
  while(true) {
    coreUtilizationCalculation();    // calaculate time cores have been idle
    websocket.printfAll("%s", systemStatusJson().c_str());   // update all connected clients.
    tmpSize=snprintf  (tmpTopic, maxTopicSize, "%s%s", mqttTopicRoot, "core0PercentIdle");
    tmpSize=snprintf  (tmpValue, maxValueSize, "%.2f", core0PercentIdle);
    if (isVerbose) {Serial.print(tmpTopic); Serial.print(": "); Serial.print(tmpValue); }
    mqttClient.publish(tmpTopic, mqttQosLevel, mqttRetain, tmpValue);
    tmpSize=snprintf  (tmpTopic, maxTopicSize, "%s%s", mqttTopicRoot, "core1PercentIdle");
    tmpSize=snprintf  (tmpValue, maxValueSize, "%.2f", core1PercentIdle);
    if (isVerbose) {Serial.print("  "); Serial.print(tmpTopic); Serial.print(": "); Serial.println(tmpValue); }
    mqttClient.publish(tmpTopic, mqttQosLevel, mqttRetain, tmpValue);

    scheduleCheck();
    
    vTaskDelay(15000 / portTICK_PERIOD_MS);
  }
}

//-------------------------------------------------------------------
// do activities needed when schedule is active
void setScheduleStatus (bool newStatus){
  if (isVerbose) {Serial.print("setScheduleStatus: Executing on core: "); Serial.print(xPortGetCoreID()); Serial.print(";  priority: "); Serial.println(uxTaskPriorityGet(NULL));}
  if (isScheduleOn != newStatus) {
    isScheduleOn = newStatus;
    if (isScheduleOn) {
      // Turn on relay
      digitalWrite(RELAY_PIN,HIGH);
      digitalWrite(TX_LED,LOW);  //use TX to show the schedule is working (in case there is no relay attached)
    } else {
      // Turn off relay
      digitalWrite(RELAY_PIN,LOW);
      digitalWrite(TX_LED,HIGH);
    }
    websocket.printfAll("%s", systemStatusJson().c_str());
    websocket.printfAll("%s", scheduleJson().c_str());
  }
}

//###################################################################
void actions_setup(){
    xTaskCreatePinnedToCore(actionsTask, "actionsTask", 3072, NULL, 2, NULL, 0);  //priority:2, core 0
}

//-------------------------------------------------------------------
void actions_loop() {

}

#endif      //actions_h
