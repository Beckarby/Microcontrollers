#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <globals.h>
#include <init.h>
#include <server.h>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

unsigned long lastSensorUpdate = 0;
const unsigned long updateInterval = 2000;

void setup() {
    Serial.begin(9600);

    while (!Serial) {
        ;   // Wait for the Serial port to be available
    }

    WiFi.softAP(AP_NAME);

    Serial.print("\nWebServer listening on: ");
    Serial.println(WiFi.softAPIP());

    // Start LittleFS
    if (!InitComponent("LittleFS", []() -> bool { return LittleFS.begin(true); })) return;

    // Start mDNS
    if (!InitComponent("mDNS", []() -> bool { return MDNS.begin(mDNS_NAME); })) return;

    // Start the WebSocket server
    InitWebSocket();

    // Start the WebServer
    InitWebServer();
}

void loop() {
    ws.cleanupClients();

    if (millis() - lastSensorUpdate > updateInterval) {
        // Placeholder data
        float temp = 24.0 + (random(-10, 10) / 10.0); 
        float hum = 50.0 + (random(-20, 20) / 10.0);

        JsonDocument doc;
        doc["temperature"] = temp;
        doc["humidity"] = hum;

        String jsonString;
        serializeJson(doc, jsonString);

        ws.textAll(jsonString);

        lastSensorUpdate = millis();
    }
}