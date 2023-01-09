/******************************************************************
 * 
******************************************************************/

#ifndef websocketDef_h
  #define websocketDef_h

//###################################################################

//#include <AsyncWebSocket.h>   //this is already in AsyncWebserver

AsyncWebSocket websocket("/ws");

//-------------------------------------------------------------------
void processSystemStatus(JsonObject incomingJsonObjNested){
  //received JSON was for System
  if (!(incomingJsonObjNested["RX_LED"].isNull())){digitalWrite(RX_LED,!incomingJsonObjNested["RX_LED"].as<bool>());}
  if (!(incomingJsonObjNested["TX_LED"].isNull())){digitalWrite(TX_LED,!incomingJsonObjNested["TX_LED"].as<bool>());}
  websocket.printfAll("%s", systemStatusJson().c_str());
}

//-------------------------------------------------------------------
void processSystemConfig(JsonObject incomingJsonObjNested){
  if (!(incomingJsonObjNested["isVerbose"].isNull())){isVerbose = incomingJsonObjNested["isVerbose"].as<bool>();}
  websocket.printfAll("%s", systemConfigJson().c_str());
}

//-------------------------------------------------------------------
void processSchedule(JsonObject incomingJsonObjNested){
  if (!(incomingJsonObjNested["isOn"].isNull())){setScheduleStatus(incomingJsonObjNested["isOn"].as<bool>());}
  if (!(incomingJsonObjNested["isEnabled"].isNull())){scheduleEnableUpdate(incomingJsonObjNested["isEnabled"].as<bool>());}
  if (!(incomingJsonObjNested["OnSecond"].isNull()) && !(incomingJsonObjNested["OnDuration"].isNull())){
    scheduleUpdate(incomingJsonObjNested["OnSecond"].as<int>(), incomingJsonObjNested["OnDuration"].as<int>());}
  websocket.printfAll("%s", scheduleJson().c_str());
}

//-------------------------------------------------------------------
void processMQTT(JsonObject incomingJsonObjNested){
  int rSize = 0;
  if (!(incomingJsonObjNested["Broker"].isNull())){
    rSize = parameterLookUp(MQTT_BROKER_Parameter, mqttBroker, sizeof(mqttBroker), incomingJsonObjNested["Broker"].as<char*>(), true);}
  if (!(incomingJsonObjNested["Port"].isNull())){
    rSize = systemParameter.putUInt(MQTT_PORT_Parameter, incomingJsonObjNested["Port"].as<int>());
    mqttPort = systemParameter.getUInt(MQTT_PORT_Parameter, mqttPort);}
  if (!(incomingJsonObjNested["QoS"].isNull())){
    rSize = systemParameter.putUInt(MQTT_QOS_Parameter, incomingJsonObjNested["QoS"].as<int>());
    mqttQosLevel = systemParameter.getUInt(MQTT_QOS_Parameter, mqttQosLevel);}
  if (!(incomingJsonObjNested["Location"].isNull())){
    rSize = parameterLookUp(MQTT_LOCATION_Parameter, mqttLocation, sizeof(mqttLocation), incomingJsonObjNested["Location"].as<char*>(), true);}
  websocket.printfAll("%s", mqttJson().c_str());
}

//-------------------------------------------------------------------
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) 
{
//  if (isVerbose)   {Serial.print("onWsEvent: Executing on core: "); Serial.print(xPortGetCoreID()); Serial.print(";  priority: "); Serial.println(uxTaskPriorityGet(NULL));}

  if (type == WS_EVT_CONNECT) 
  {
    Serial.printf("Websocket client #%u connected\n", client->id());
    client->printf("%s", systemStatusJson().c_str());
    client->printf("%s", systemConfigJson().c_str());
    client->printf("%s", filesJson().c_str());
    client->printf("%s", scheduleJson().c_str());
    client->printf("%s", mqttJson().c_str());
  } 
  else if (type == WS_EVT_DISCONNECT) 
  {
    Serial.printf("Websocket client #%u disconnect\n", client->id());
  } 
  else if (type == WS_EVT_ERROR) 
  {
    Serial.printf("Websocket client #%u error(%u): %s\n", client->id(), *((uint16_t*)arg), (char*)data);
  } 
  else if (type == WS_EVT_PONG) 
  {
    Serial.printf("Websocket client #%u pong[%u]: %s\n", client->id(), len, (len) ? (char*)data : "");
  } 
  else if (type == WS_EVT_DATA) 
  {
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    
    if (info->final && info->index == 0 && info->len == len) 
    {
      //the whole message is in a single frame and we got all of it's data
      if (isVerbose) {Serial.printf("Websocket[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);}

      if (info->opcode == WS_TEXT) 
      {
        for (size_t i = 0; i < info->len; i++) 
        {
          msg += (char) data[i];
        }
      } 
      else 
      {
        char buff[6];
        
        for (size_t i = 0; i < info->len; i++) 
        {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      
      if (isVerbose) {Serial.printf("RECEIVED WEBSOCKET TEXT: %s\n", msg.c_str());}

//      if (info->opcode == WS_TEXT)
//        client->text("I got your text message");
//      else
//        client->binary("I got your binary message");

      StaticJsonDocument<152> incomingJsonDoc;
      DeserializationError err = deserializeJson(incomingJsonDoc, msg);
      switch (err.code()) {
          case DeserializationError::Ok:
              if (isVerbose) {
                Serial.println(F("Deserialization succeeded"));
                Serial.print(F("set \"StaticJsonDocument<???> settingsJson;\" to :"));
                Serial.println(incomingJsonDoc.memoryUsage());  //Use this function at design time to measure the required capacity for the JsonDocument.
              }
              break;
          case DeserializationError::InvalidInput:
              Serial.print(F("Invalid json input!  -> ")); Serial.println(msg.c_str());
              break;
          case DeserializationError::NoMemory:
              Serial.println(F("Not enough memory"));
              break;
          default:
              Serial.println(F("Deserialization failed"));
              break;
      }
    
      if (incomingJsonDoc.isNull()) {
        // incoming data is NOT json
        char  outMsg[12]="";
        if (strcmp(msg.c_str(), "syn") == 0) {
          //reply with acknowledgement (not required since the websocket protocol uses builtin ping/pong for keep alive
        } else {
          strcpy(outMsg, "Unexpected message: ");
          strcat(outMsg,msg.c_str());
        }
      } else {
        // incoming data IS json
        if (isVerbose) {serializeJsonPretty(incomingJsonDoc, Serial);}
        for (JsonPair kvPair : incomingJsonDoc.as<JsonObject>()) {
        if (isVerbose) {Serial.println(kvPair.key().c_str());}
          if (strcmp(kvPair.key().c_str(), "SS") == 0) { processSystemStatus(incomingJsonDoc["SS"]); }
          if (strcmp(kvPair.key().c_str(), "SC") == 0) { processSystemConfig(incomingJsonDoc["SC"]); }
          if (strcmp(kvPair.key().c_str(), "SL") == 0) { processSchedule(incomingJsonDoc["SL"]); }
          if (strcmp(kvPair.key().c_str(), "MQTT") == 0) { processMQTT(incomingJsonDoc["MQTT"]); }
          
        }
      }
    }
    else 
    {
      //message is comprised of multiple frames or the frame is split into multiple packets
      
      if (info->index == 0) 
      {
        if (info->num == 0)
          Serial.printf("Websocket[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
          
        Serial.printf("Websocket[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("Websocket[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);

      if (info->opcode == WS_TEXT) 
      {
        for (size_t i = 0; i < len; i++) 
        {
          msg += (char) data[i];
        }
      } 
      else 
      {
        char buff[6];
        
        for (size_t i = 0; i < len; i++) 
        {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      
//      Serial.printf("%s\n", msg.c_str());
      Serial.printf("RECEIVED LARGE WEBSOCKET TEXT:\n %s\n", msg.c_str());

      if ((info->index + len) == info->len) 
      {
        Serial.printf("Websocket[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        
        if (info->final) 
        {
          Serial.printf("Websocket[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
          
          if (info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
}

//###################################################################
void websocket_setup(){

}

//-------------------------------------------------------------------
void websocket_loop() {
  
}

#endif      //websocketDef_h
