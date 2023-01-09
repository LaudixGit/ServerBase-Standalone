/******************************************************************
 * used for the config page (the page that would be index.html except for the custom page)
******************************************************************/

#ifndef index_html_h
  #define index_html_h

static const char PROGMEM index_html[] = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style type="text/css">
    /* https://www.w3schools.com/howto/howto_js_tabs.asp */
    
    :root {
      /* set css variables https://developer.mozilla.org/en-US/docs/Web/CSS/var */
      --toggleHeightSize: 16px;
    }
    
    body {font-family: Arial;}
    
    /* Style the tab */
    .tab {
      overflow: hidden;
      border: 1px solid #ccc;
      background-color: #f1f1f1;
    }
    
    /* Style the buttons inside the tab */
    .tab button {
      background-color: inherit;
      float: left;
      border: none;
      outline: none;
      cursor: pointer;
      padding: 14px 16px;
      transition: 0.3s;
      font-size: 17px;
    }
    
    /* Change background color of buttons on hover */
    .tab button:hover {
      background-color: #ddd;
    }
    
    /* Create an active/current tablink class */
    .tab button.active {
      background-color: #ccc;
    }
    
    /* Style the tab content */
    .tabcontent {
      display: none;
      padding: 6px 12px;
      border: 1px solid #ccc;
      border-top: none;
    }
    
    /* for a toggle switch http s://alvarotrigo.com/blog/toggle-switch-css */
    .toggle {
      cursor: pointer;
      display: inline-block;
    }
    
    .toggle-switch {
      display: inline-block;
      background: #ccc;
      border-radius: calc(var(--toggleHeightSize)/2);
      width: calc(var(--toggleHeightSize)*1.9);
      height: var(--toggleHeightSize);
      position: relative;
      vertical-align: middle;
      transition: background 0.25s;
    }
    .toggle-switch:before, .toggle-switch:after {
      content: "";
    }

    .toggle-switch:before {
      display: block;
      background: linear-gradient(to bottom, #fff 0%%, #eee 100%%);  /* use the double %% so the KeyValueLookup() function ignores them */
      border-radius: 50%%;
      box-shadow: 0 0 0 1px rgba(0, 0, 0, 0.25);
      width: calc(var(--toggleHeightSize) - var(--toggleHeightSize)/4);
      height: calc(var(--toggleHeightSize) - var(--toggleHeightSize)/4);
      position: absolute;
      top:  calc(var(--toggleHeightSize)/8);
      left: calc(var(--toggleHeightSize)/8);
      transition: left 0.25s;
    }
    .toggle:hover .toggle-switch:before {
      background: linear-gradient(to bottom, #fff 0%%, #fff 100%%);
      box-shadow: 0 0 0 1px rgba(0, 0, 0, 0.5);
    }
    .toggle-checkbox:checked + .toggle-switch {
      background: #56c080;
    }
    .toggle-checkbox:checked + .toggle-switch .toggle-checkMark {
      opacity: 1;
      transition: opacity $speed ease-in-out;
    }
    .toggle-checkbox:checked + .toggle-switch:before {
      left: calc(var(--toggleHeightSize));
    }
    
    
    .toggle-checkbox {
      position: absolute;
      visibility: hidden;
    }
    
    .toggle-checkMark {
      fill: #fff;
      font-size: calc(var(--toggleHeightSize)*.65);
      position: relative;
      left: calc(var(--toggleHeightSize) + var(--toggleHeightSize)/8);
      opacity: .1;
      transition: opacity $speed ease-in-out;
      display: flex;
      align-items: center;
      /* this would work if suported by Chrome top: calc(pow(2, (log(var(--toggleHeightSize),2)-3))); */
    }
    
    .toggle-label {
      margin-left: 5px;
      position: relative;
      top: 2px;
    }
    
    /* For Button-style checkbox */
    /* http s://stackoverflow.com/questions/38377062/how-to-make-html-button-look-pressed-in-using-css */
    label.button-checkbox {
      cursor: pointer;
    }
    
    label.button-checkbox input {
      position: absolute;
      top: 0;
      left: 0;
      visibility: hidden;
      pointer-events: none;
    }
    
    label.button-checkbox span {
      padding: 1px 5px;
      border: 1px solid #ccc;
      display: inline-block;
      color: #202020;
      border-radius: 25%%;
      margin: 1px;
      background: #f5f5f5;
      user-select: none;
      font-size: calc(.65em);
    }
    
    label.button-checkbox input:checked + span {
      color: #00BB00;
      box-shadow: inset 1px 2px 5px #777;
      transform: translateY(1px);
      background: #e5e5e5;
    }

  </style>
  <script>
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
        document.getElementById("freeHeapRatio").innerHTML   = (jsonDoc["SS"]["FreeHeap"]/jsonDoc["SS"]["TotalHeap"]*100).toFixed(1);
        document.getElementById("totalHeap").innerHTML       = jsonDoc["SS"]["TotalHeap"];
        document.getElementById("largestBlock").innerHTML       = jsonDoc["SS"]["LargestBlock"];
        document.getElementById("core0PercentIdle").innerHTML= Math.floor((jsonDoc["SS"]["core0PercentIdle"])*10)/10;
        document.getElementById("core1PercentIdle").innerHTML= Math.floor((jsonDoc["SS"]["core1PercentIdle"])*10)/10;
        document.getElementById("runtime").innerHTML         = jsonDoc["SS"]["Runtime"];
        document.getElementById("serverTime").innerHTML      = jsonDoc["SS"]["Now"];
        document.getElementById("rx_led").checked            = jsonDoc["SS"]["RX_LED"];
        document.getElementById("tx_led").checked            = jsonDoc["SS"]["TX_LED"];
      }
      
      function processSystemConfig (jsonDoc) {
        document.getElementById("isVerbose").checked         = jsonDoc["SC"]["isVerbose"];
      }
      
      function processSchedule (jsonDoc) {
        //let tmpDate = new Date(jsonDoc["SL"]["OnTime"] * 1000);
        // Date.parse(date)  https://stackoverflow.com/questions/13707333/javascript-convert-date-time-string-to-epoch
        document.getElementById("OnTime").innerHTML    = new Date(jsonDoc["SL"]["OnTime"] * 1000);
        document.getElementById("OffTime").innerHTML   = new Date(jsonDoc["SL"]["OffTime"] * 1000);
        document.getElementById("OnSecond").value      = (new Date(jsonDoc["SL"]["OnSecond"] * 1000)).toUTCString().substring(17,22);
        document.getElementById("scheduleOnDuration").value      = (jsonDoc["SL"]["OnDuration"])/60;  //convert from seconds into minutes
        document.getElementById("isScheduleOn").checked= jsonDoc["SL"]["isOn"];
        document.getElementById("isScheduleEnabled").checked= jsonDoc["SL"]["isEnabled"];
      }
      
      function processFiles (jsonDoc) {
        // the Files json is not used on this page.
        // this function exists to handle the json that is sent - to avoid the "Unknown JSON object"
      }

      function processMQTT (jsonDoc) {
        document.getElementById("mqttBroker").value    = jsonDoc["MQTT"]["Broker"];
        document.getElementById("mqttPort").value      = jsonDoc["MQTT"]["Port"];
        document.getElementById("mqttQosLevel").value  = jsonDoc["MQTT"]["QoS"];
        document.getElementById("mqttLocation").value  = jsonDoc["MQTT"]["Location"];
      }

        function systemStatusSend(){
        var jsonOut = {"SS": {}};
        jsonOut["SS"]["RX_LED"]=document.getElementById("rx_led").checked;
        jsonOut["SS"]["TX_LED"]=document.getElementById("tx_led").checked;
        websock.send(JSON.stringify(jsonOut));
      }

      function systemConfigSend(){
        var jsonOut = {"SC": {}};
        jsonOut["SC"]["isVerbose"]=document.getElementById("isVerbose").checked;
        websock.send(JSON.stringify(jsonOut));
      }

      function sheduleSend(){
        var jsonOut = {"SL": {}};
        jsonOut["SL"]["isOn"]=document.getElementById("isScheduleOn").checked;
        jsonOut["SL"]["isEnabled"]=document.getElementById("isScheduleEnabled").checked;
        jsonOut["SL"]["OnSecond"]=Date.parse(document.getElementById("OnSecond").valueAsDate)/1000;  //convert from ms to seconds
        jsonOut["SL"]["OnDuration"]=(document.getElementById("scheduleOnDuration").value)*60;  //convert from minutes to seconds
        websock.send(JSON.stringify(jsonOut));
      }
      
      function mqttSend(){
        var jsonOut = {"MQTT": {}};
        jsonOut["MQTT"]["Broker"]=document.getElementById("mqttBroker").value;
        jsonOut["MQTT"]["Port"]=document.getElementById("mqttPort").value;
        jsonOut["MQTT"]["QoS"]=document.getElementById("mqttQosLevel").value;
        jsonOut["MQTT"]["Location"]=document.getElementById("mqttLocation").value;
        websock.send(JSON.stringify(jsonOut));
      }

    function tabShow(evt, tabName) {
      var i, tabcontent, tablinks, isRepeat;
      isRepeat = evt.srcElement.classList.contains("active");
      tabcontent = document.getElementsByClassName("tabcontent");
      for (i = 0; i < tabcontent.length; i++) {
        tabcontent[i].style.display = "none";
      }
      tablinks = document.getElementsByClassName("tablinks");
      // for (i = 0; i < tablinks.length; i++) {
        // isRepeat = tablinks[i]==evt.srcElement;
    // console.log(evt);
        // if (isRepeat) { break;}
      // }
      for (i = 0; i < tablinks.length; i++) {
        tablinks[i].className = tablinks[i].className.replace(" active", "");
      }
      if (isRepeat) {
        // selected tab is the active tab, so exit without exposing any content; effectively hide the already open tab
        return;
      }
      document.getElementById(tabName).style.display = "block";
      evt.currentTarget.className += " active";
    }
  </script>
</head>
<body>
    <h1>config page</h1>
    <div>
        <small>Compiled:   %CompDate%  %CompTime% - %CompFile%</small></br>
        <hr>
        Server Time: <span id="serverTime"></span></br>
        Total Heap: <span id="totalHeap"></span>;  Free Heap: <span id="freeHeapRatio"></span>%%;  Largest Block: <span id="largestBlock"></span></br>
        Runtime: <span id="runtime"></span>ms.  </br>
        Idle, Core <b>0</b>: <span id="core0PercentIdle"></span>%%</br>
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Core <b>1</b>: <span id="core1PercentIdle"></span>%%</br>
        <hr>
        <label for="rx_led" title="Click to turn on/off the light on the device">RX</label><input id="rx_led" type="checkbox" onchange="systemStatusSend()"/>  <label for="tx_led">TX</label><input id="tx_led" type="checkbox" onchange="systemStatusSend()" disabled title="TX LED is disabled here because it is controlled by the Schedule"/>
    </div>
  <hr>
  <a href="/">Home</a> <a href="/directory">Directory</a>
  <hr>
    <h3>Settings</h3>
    <div class="tab">
      <button class="tablinks" onclick="tabShow(event, 'System')">System</button>
      <button class="tablinks" onclick="tabShow(event, 'Schedule')">Schedule</button>
      <button class="tablinks" onclick="tabShow(event, 'MQTT')">MQTT</button>
    <!--   <button class="tablinks" onclick="tabShow(event, 'Telemetry')">Telemetry</button> -->
    </div>

    <div id="System" class="tabcontent">
      <p>This device has rebooted %restartCount% times.</p>
      <p>I2C Addresses detected at device startup: %i2cAddresses%</p>
      <hr>
      <label for="isVerbose" Title="increase messages to the console log">Verbose: </label><input type="checkbox" id="isVerbose" onchange="systemConfigSend()"></br>
      <label for="systemReboot" title="Cause the device to restart">reboot: </label><a id=systemReboot href="/reboot">&#10226;</a>
    </div>

    <div id="Schedule" class="tabcontent">
      <p>When to start the accessory(ies), and how long to run.</p>
      <div>
        On  Time: <span id="OnTime"></span></br>
        Off Time: <span id="OffTime"></span></br>
      </div>
      <hr>
      <label for="OnSecond">Time of day to start: </label>
    <input id="OnSecond" type="time" name="OnSecond" value="00:00" onchange="sheduleSend()" /></br>  <!-- https://developer.mozilla.org/en-US/docs/Web/HTML/Element/input/time -->
      <label for="scheduleOnDuration" Title="How many minutes to run">Run Duration: </label> <input type="number" id="scheduleOnDuration" min="0" max="1440"  onchange="sheduleSend()"></br>
      <label class="toggle" Title="Turn On/Off"> <!-- https://alvarotrigo.com/blog/toggle-switch-css/ -->
          <input id="isScheduleOn" class="toggle-checkbox" type="checkbox" onchange="sheduleSend()">
          <div class="toggle-switch"><span class="toggle-checkMark">&#10003;</span></div>
          <span class="toggle-label">Accessories</span>
        </label>
        <label class="button-checkbox" title="Set to manual; ignore schedule until this is undone."> <!-- https://stackoverflow.com/questions/38377062/how-to-make-html-button-look-pressed-in-using-css -->
          <input id="isScheduleEnabled" type="checkbox" onchange="sheduleSend()" />
          <span>&#11208;</span>
        </label>
    </div>

    <div id="MQTT" class="tabcontent">
      <h3>MQTT</h3>
      <p>How to connect to the MQTT Broker.</p> 
      <p>MQTT settings are on used at startup, so <a href="/reboot">reboot</a> to use updated settings.</p> 
      <hr>
      <div>
      <span style="display: inline-block; width: 5em;"><label for="c" Title="DNS address of the MQTT server">Broker: </label></span> <input type="text" id="mqttBroker" onchange="mqttSend()"></br>
      <span style="display: inline-block; width: 5em;"><label for="mqttPort" Title="Port on the MQTT server">Port: </label></span> <input type="number" id="mqttPort" onchange="mqttSend()"></br>
      <span style="display: inline-block; width: 5em;"><label for="mqttQosLevel" Title="0: fire & forget. 1: at least once. 2: exactly once">QoS Level: </label></span> <input type="number" id="mqttQosLevel" min=0 max=2 step=1 onchange="mqttSend()"></br>
      <span style="display: inline-block; width: 5em;"><label for="mqttLocation" Title="Defines the facility the devvice is located at">Location: </label></span> <input type="text" id="mqttLocation" onchange="mqttSend()"></br>
      </div>
    </div>
</body>
</html>
)rawliteral";


#endif      //index_html_h
