const OnLoad = (event) => { InitWebSocket(); }

let gateway = `ws://${window.location.hostname}/ws`;
let websocket;
addEventListener('load', OnLoad);

function InitWebSocket(){
  console.log('INFO: Attempting to open a WebSocket connection...');
  websocket = new WebSocket(gateway);
  websocket.onopen = OnOpen;
  websocket.onclose = OnClose;
  websocket.onmessage = OnMessage;
}

const GetValues = () => { websocket.send("GetValues"); }

function OnOpen(event){
  console.log('INFO: Connection opened.');
  GetValues();
}

function OnClose(event){
  console.log('INFO: Connection closed.');
  setTimeout(InitWebSocket, 2000);
}

function OnMessage(event){
  try {
    // Parse the JSON
    const data = JSON.parse(event.data);

    // Check for temperature & humidity keys
    if (data.temperature !== undefined && data.humidity !== undefined) {

      // Update the HTML
      updateSensorDisplay(
          Number(data.temperature).toFixed(1),
          Number(data.humidity).toFixed(1)
      );
    }
  } catch (e) {
    // If the ESP32 sends normal text, JSON parsing fails & this executes
    console.log("Received text message from ESP32:", event.data);
  }
}