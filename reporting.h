/******************************************************************
 * 
******************************************************************/

#ifndef actions_h
#define actions_h

//###################################################################
//-------------------------------------------------------------------
void reportingTask(void *params){
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

    vTaskDelay(15000 / portTICK_PERIOD_MS);
  }
}

//###################################################################
void actions_setup(){
    xTaskCreatePinnedToCore(reportingTask, "reportingTask", 3072, NULL, 2, NULL, 0);  //priority:2, core 0
}

//-------------------------------------------------------------------
void actions_loop() {

}

#endif      //actions_h
