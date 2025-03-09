
// Variabeln
var Startbutton = false;
var Stoppbutton = false;
var ZeitInterval;   
var countDownDate;
var stopp=false;
  const pichelButton = document.getElementById('PichelButton');
  const teamButton = document.getElementById('TeamButton');
  const content = document.getElementById('idcontent');
  const intro = document.getElementById('idIntro');
  const Pichel= document.getElementById('idPichel');
  const Ueberschrift= document.getElementById('idUeberschrift');
  const gaugeHum1 = document.getElementById('gauge-humidity');
 const UeberschriftMWert1= document.getElementById('UeberschriftMWert');


 //Windows hört zu

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

  //Websocket Block
  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  window.addEventListener('load', onLoad);

  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage;// <-- add this line
  }

  function onOpen(event) {
    console.log('Connection opened');
    websocket.send('hi');
    //Anfang();
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
    //initButton();
  }
// ESP sendet Daten an den Browser
  function onMessage(event) {
    let data = JSON.parse(event.data);
      if (data.ledState == "1000"){  
        document.getElementById('gauge-humidity').innerHTML=data.Messwert;
      }
      else if (data.ledState =="1001"){
      document.getElementById("idInfo").innerHTML = "Messung ist im ESP32 gestartet";
      }
      else if (data.ledState =="1002"){
      document.getElementById("idInfo").innerHTML = "Messung ist im ESP32 gestoppt";
       }
    }
//Ende Block

// Anzeige bei Button Click Ereignis

  pichelButton.addEventListener('click', function() {
    content.style.display = 'block';
    intro.style.display = 'none';
    gaugeHum1.style.display = 'block';
    
    Ueberschrift.style.display='block';
    Pichel.style.display='flex';
    UeberschriftMWert1.style.display='block';
  });

  teamButton.addEventListener('click', function() {
    intro.style.display = 'block';
    Ueberschrift.style.display ='none';
    Pichel.style.display ='none';
    gaugeHum1.style.display = 'none';
    content.style.display = 'none';
    UeberschriftMWert1.style.display='none';
    
  });

// Funktion beim Laden der Seite
function Anfang() {
  intro.style.display = 'none';
}

// Startbutton Funktion-tring Start wird an den ESP32 gesendet
function opStart(){
websocket.send("Start");
const a1 = "./logo/Uschi_start_m.jpg";
const image = document.getElementById("U_start");
image.src = a1;
Startbutton=true;
startTimer();
}
//Stoppbutton Funktion- String Stopp wird an den ESP32 gesendet
function opStopp(){
websocket.send("Stopp");
const a1 = "./logo/Uschi_stopp_m.jpg";
const image = document.getElementById("U_stopp"); 
image.src = a1;
Stoppbutton=true;
startTimer();
}

//Timer wird gestartet, wenn der Start, oder Stopp Button gedrückz wird

function startTimer(){ 
  ZeitInterval = setInterval(myTimer,500);
  countDownDate = new Date().getTime();
  }
function myTimer() {
  var now = new Date().getTime();
  var distance =  now - countDownDate;
  // Time calculations for days, hours, minutes and seconds
  var days = Math.floor(distance / (1000 * 60 * 60 * 24));
  var hours = Math.floor((distance % (1000 * 60 * 60 * 24)) / (1000 * 60 * 60));
  var minutes = Math.floor((distance % (1000 * 60 * 60)) / (1000 * 60));
  var seconds = Math.floor((distance % (1000 * 60)) / 1000);
  if (seconds==2){
    if(Startbutton){  
      const a1 = "./logo/Uschi_start_o.jpg";
      const image = document.getElementById("U_start");
      image.src = a1;
      Startbutton=false;
      stoppTimer();     
    }  
    else if (Stoppbutton){
      const a1 = "./logo/Uschi_stopp_o.jpg";
      const image = document.getElementById("U_stopp");
      image.src = a1;
      Stoppbutton=false;
      stoppTimer();
    }
  } 
}

function stoppTimer(){
   clearInterval(ZeitInterval);
   stopp=false;
}
