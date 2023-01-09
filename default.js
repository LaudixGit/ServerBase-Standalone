
var websock;
var websocketConnected = false;      // true when connection is successful
var websocketReconnectCheck = setInterval(startWebsocket, 60000);  //Once a minute try to reconnect
var websocketHealthCheckTimer;  //holds timer info for health  check

window.onload = function () {
  console.log("Script runs");
  startWebsocket();
}

  function startWebsocket() {
    if (websocketConnected) {
//console.log('Websocket already connected');
    } else {
        websock = new WebSocket("ws://" + document.location.host   + "/ws");
        websock.onopen = function(evt) { 
          websocketConnected = true;
          console.log('websock open');
//          websock.send("syn");
          };
        websock.onclose = function(evt) { 
          websocketConnected = false;
          console.log('websock close');
          };
        websock.onerror = function(evt) { console.log(evt); };
        websock.onmessage = processMessage;
    }
  }

  function processMessage(evt) {
//console.log("Incoming Event: ",evt);
      try {
        jsonCurrent = JSON.parse(evt.data);
        jsonValid = true;
      } catch (e) {
        //invalid JSON
        jsonValid = false;
      }
  
    if (jsonValid) {
      //process all known elements
      for (var key in jsonCurrent){
        var value = jsonCurrent[key];
//console.log(key + ": " + value);
  
        if (key == "SS") { processSystemStatus(jsonCurrent);}
        else if (key == "SC") { processSystemConfig(jsonCurrent);}
        else if (key == "Files") { processFiles(jsonCurrent);}
        else if (key == "SL") { processSchedule(jsonCurrent);}
        else if (key == "MQTT") { processMQTT(jsonCurrent);}
        
        else { console.log( "Unknown JSON object: ~", key, "~\n", jsonCurrent );}
      }
    } else {
      //event wasn't json maybe it's something else interesting
      var evtData = evt.data;  // variable for convenience
      if (evtData == "ack") {
        //server has acknowledged connection
      } else {
        // not an event we are looking for
              console.log('unknown event\n', evt, "\n", evt.data);
      }
    }
  }

  function processSystemStatus (jsonDoc) {
    document.getElementById("serverTime").innerHTML      = jsonDoc["SS"]["Now"];
    document.getElementById("rx_led").checked            = jsonDoc["SS"]["RX_LED"];
    document.getElementById("tx_led").checked            = jsonDoc["SS"]["TX_LED"];
  }
  
  function processSystemConfig (jsonDoc) {
  }
  
  function processSchedule (jsonDoc) {
  }
  
  function processFiles (jsonDoc) {
    // the Files json is not used on this page.
    // this function exists to handle the json that is sent - to avoid the "Unknown JSON object"
  }

  function processMQTT (jsonDoc) {
  }

    function systemStatusSend(){
    var jsonOut = {"SS": {}};
    jsonOut["SS"]["RX_LED"]=document.getElementById("rx_led").checked;
    websock.send(JSON.stringify(jsonOut));
  }
