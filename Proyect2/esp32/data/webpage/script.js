function sendMessage() {
  let msgInput = document.getElementById('message-input');
  let msg = msgInput.value.trim();

  if (!msg) {
    return;
  }

  if (typeof websocket !== 'undefined' && websocket.readyState === WebSocket.OPEN) {
    websocket.send(JSON.stringify({
      'message': msg
    }));
    console.log(`Sent message: ${msg}`);
  }
}

function updateSensorDisplay(temp, hum) {
  document.getElementById('temp-display').innerText = temp;
  document.getElementById('hum-display').innerText = hum;
}

// Placeholder ESP32 Data
function simulateESP32Data() {
  console.log("Started Sensor Simulation...");

  // Run this every 2 seconds
  setInterval(() => {
    // Generate random variations around 24.0°C and 50.0%
    let fakeTemp = (24 + (Math.random() * 2 - 1)).toFixed(1);
    let fakeHum = (50 + (Math.random() * 5 - 2.5)).toFixed(1);

    updateSensorDisplay(fakeTemp, fakeHum);
  }, 2000);
}

window.onload = function() {
  simulateESP32Data();
};