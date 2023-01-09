/******************************************************************
 * shows list of files and enables uploads (including OTA)
 * 
 * https://developer.mozilla.org/en-US/docs/Web/API/FormData/Using_FormData_Objects
******************************************************************/

#ifndef directory_html_h
  #define directory_html_h

static const char PROGMEM directory_html[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Directory</title>
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
        
        else { console.log( "Unknown JSON object: ", key, "\n", jsonCurrent );}
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
  }

  function processSystemConfig (jsonDoc) {
  }
  
  function processSchedule (jsonDoc) {
  }
  
  function processFiles (jsonDoc) {
    var parentUL = document.getElementById('fileList');
    parentUL.innerHTML = "";
    for (var i=0; i<jsonDoc["Files"].length; i++) {
      newLi = document.createElement('li');

      newLink = document.createElement('a');
      newLink.setAttribute('href',"/" + jsonDoc["Files"][i]);
      newLink.innerText = jsonDoc["Files"][i];
      newLi.appendChild(newLink);      

      newLi.innerHTML += " ";

      newLink = document.createElement('a');
      newLink.setAttribute('href',"/remove?filename=" + jsonDoc["Files"][i]);
      newLink.innerText = String.fromCodePoint(0x1F5D1);
      newLi.appendChild(newLink);      
            
      
      parentUL.appendChild(newLi);
//console.log(jsonDoc["Files"][i]);
    }
  }

  function processMQTT (jsonDoc) {
  }
  
    function _(el) {
      return document.getElementById(el);
    }

    function fileExtCheck(field){
        const name = field.files[0].name;
        const lastDot = name.lastIndexOf('.');
        const ext = name.substring(lastDot + 1).toLowerCase();
        if (ext == 'bin') {
            document.getElementById('isOTALabel').style.visibility = 'visible';
            document.getElementById('isOTA').style.visibility = 'visible';
            document.getElementById('isOTA').checked = true;
        } else {
            document.getElementById('isOTALabel').style.visibility = 'hidden';
            document.getElementById('isOTA').style.visibility = 'hidden';
            document.getElementById('isOTA').checked = false;
        }
    }

    function uploadFile() {
      var file = _('fileSelected').files[0];
      // alert(file.name+" | "+file.size+" | "+file.type);
      var formdata = new FormData();
      if (document.getElementById('isOTA').style.visibility != 'hidden'){
        formdata.append('isOTA', document.getElementById('isOTA').checked);
      }
      formdata.append('MD5', '5eb63bbbe01eeed093cb22bb8f5acdc3');
      formdata.append("file", file);
      var ajax = new XMLHttpRequest();
      ajax.upload.addEventListener("progress", progressHandler, false);
      ajax.addEventListener("load", completeHandler, false); // doesnt appear to ever get called even upon success
      ajax.addEventListener("error", errorHandler, false);
      ajax.addEventListener("abort", abortHandler, false);
      ajax.open("POST", "/update");
      ajax.send(formdata);
    }
    function progressHandler(event) {
      //_("loaded_n_total").innerHTML = "Uploaded " + event.loaded + " bytes of " + event.total; // event.total doesnt show accurate total file size
      _("loaded_n_total").innerHTML = "Uploaded " + event.loaded + " bytes";
      var percent = (event.loaded / event.total) * 100;
      _("progressBar").value = Math.round(percent);
      _("status").innerHTML = Math.round(percent) + "%% uploaded... please wait";
      if (percent >= 100) {
        _("status").innerHTML = "Please wait, writing file to filesystem";
      }
    }
    function completeHandler(event) {
      _("status").innerHTML = "Upload Complete";
      _("progressBar").value = 0;
//      xmlhttp=new XMLHttpRequest();
//      xmlhttp.open("GET", "/", false);
//      xmlhttp.send();
      if (document.getElementById('isOTA').checked) {
        document.getElementById("status").innerHTML = "File Uploaded.  Waiting for device restart...";
      } else {
        document.getElementById("status").innerHTML = "File Uploaded";
      }
      window.location = "/directory";
    }
    function errorHandler(event) {
console.log("error event: ");
console.log(event.type);
console.log(event.message);
      _("status").innerHTML = "Upload Failed";
    }
    function abortHandler(event) {
      _("status").innerHTML = "inUpload Aborted";
    }
    </script>
</head>
<body>
    <h1>File Listing</h1>
    Server Time: <span id="serverTime"></span></br>
    <a href='/'>Home</a>
    <hr>
    <ul id='fileList'>
    </ul>
    <div>
        <form method='POST' action='/update' enctype='multipart/form-data' id='upload_form'>
            <input type='hidden' name='MD5' value='5eb63bbbe01eeed093cb22bb8f5acdc3' >
            <input type='file' id='fileSelected' name='file' onchange='fileExtCheck(this);'>
        </form>
        <label for='isOTA' id='isOTALabel'  style='visibility: hidden;'>OTA</label>
        <input type='checkbox' id='isOTA' name='isOTA' style='visibility: hidden;' >
        <input type='submit' value='Upload' onclick='uploadFile();'>
        <h3 id='status'></h3>
        <progress id='progressBar' value='0' max='100' style='width:300px;'></progress>
        <p id='loaded_n_total'></p>
    </div>
</body>
<script>
</script>
</html>
)rawliteral";

#endif      //directory_html_h
