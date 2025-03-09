/*const idInfo = document.getElementById('idInfo');
const idPichel = document.getElementById('idPichel');
const idTeam = document.getElementById('idTeam');
const idcontent = document.getElementById('idcontent'); // Corrected assignment
const PichelButton = document.getElementById('PichelButton');
const TeamButton = document.getElementById('TeamButton');*/
let isDragging = false;
var Startbool=false;
var Stoppbool=false;
// Remove redundant src assignments
// src="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js";
// src="https://ajax.googleapis.com/ajax/libs/jquery/3.7.1/jquery.min.js";
// src="https://maxcdn.bootstrapcdn.com/bootstrap/3.4.0/js/bootstrap.min.js";
window.addEventListener('load', onLoad);
if (!!window.EventSource) {
  var source = new EventSource('/events');
  
  source.addEventListener('open', function(e) {
    console.log("Events Connected");
  }, false);

  source.addEventListener('error', function(e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);
  
  source.addEventListener('message', function(e) {
    console.log("message", e.data);
  }, false);
}

window.addEventListener('load', onLoad);

function initButton() {
 
}



document.getElementById('TeamButton').addEventListener('click', () => {
  document.getElementById('TeamButton').classList.add('active');
  document.getElementById('PichelButton').classList.remove('active');
  document.getElementById('idTeam').style.display = 'flex';
  document.getElementById('idPichel').style.display = 'none';
});

document.getElementById('PichelButton').addEventListener('click', () => {
  document.getElementById('TeamButton').classList.remove('active');
  document.getElementById('PichelButton').classList.add('active');
  document.getElementById('idTeam').style.display = 'none';
  document.getElementById('idPichel').style.display = 'flex';
});

var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

function initWebSocket() {
  console.log('Trying to open a WebSocket connection...');
  websocket = new WebSocket(gateway);
  websocket.onopen    = onOpen;
  websocket.onclose   = onClose;
  websocket.onmessage = onMessage;
}




function Anfang() {
 
}

function opStart() { 
  websocket.send("Start")
}

function opStopp() { 
  websocket.send("Stopp")
}

function onOpen(event) {
  console.log('Connection opened');
  websocket.send('hi');
  setInterval(function() {
    if (websocket.readyState === WebSocket.OPEN) {
      websocket.send('ping');
    }
  }, 30000); // Alle 30 Sekunden
}

function onClose(event) {
  console.log('Connection closed');
  setTimeout(initWebSocket, 2000);
}

function onLoad(event) {
  initWebSocket();
  initButton();
  
}

function onMessage(event) {
  let data = JSON.parse(event.data);
  if (data.ledState == "1000") {document.getElementById("idinfo").innerHTML = data.Messwert;
  } else if (data.ledState == "1001") {
  } else if (data.ledState == "1002") {
  } else if (data.ledState == "1003") {
  }
}









