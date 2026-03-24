#pragma once
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#define TIMEOUT 5       // Components startup timeout

const String AP_NAME = "ESP32_AP";
const String mDNS_NAME = "ESP32";

extern AsyncWebServer server;
extern AsyncWebSocket ws;