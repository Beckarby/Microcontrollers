#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <globals.h>
#include <init.h>
#include <web_handlers.h>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

unsigned long lastSensorUpdate = 0;
const unsigned long updateInterval = 2000;

const uint8_t DHT_PIN = 4;
const uint8_t DHT_TYPE = DHT11;
DHT dht(DHT_PIN, DHT_TYPE);

const uint8_t SERVO_PIN = 5;
const uint8_t BUTTON_PIN = 23;
const unsigned long BUTTON_DEBOUNCE_MS = 30;

Servo myServo;
int lastButtonReading = HIGH;
int buttonState = HIGH;
unsigned long lastDebounceTime = 0;

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

    dht.begin();

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    myServo.setPeriodHertz(50);
    myServo.attach(SERVO_PIN, 500, 2400);
    myServo.write(0);
}

void loop() {
    ws.cleanupClients();

    int reading = digitalRead(BUTTON_PIN);
    if (reading != lastButtonReading) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > BUTTON_DEBOUNCE_MS) {
        if (reading != buttonState) {
            buttonState = reading;
            if (buttonState == LOW) {
                myServo.write(180);
                Serial.println("Button pressed: servo moved to 180");
            }
        }
    }
    lastButtonReading = reading;

    if (millis() - lastSensorUpdate > updateInterval) {
        float hum = dht.readHumidity();
        float temp = dht.readTemperature();

        if (isnan(temp) || isnan(hum)) {
            Serial.println("Failed to read from DHT11 sensor");
            lastSensorUpdate = millis();
            return;
        }

        JsonDocument doc;
        doc["temperature"] = temp;
        doc["humidity"] = hum;

        String jsonString;
        serializeJson(doc, jsonString);

        Serial.printf("Sending JSON to %u clients: %s\n", ws.count(), jsonString.c_str());
        ws.textAll(jsonString);

        lastSensorUpdate = millis();
    }
}