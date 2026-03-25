#pragma once

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

void NotFound(AsyncWebServerRequest *request);
void OnEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
void HandleWebSocketMessage(void* arg, uint8_t* payload, size_t len);
void TestAction(JsonDocument& rec_json);
