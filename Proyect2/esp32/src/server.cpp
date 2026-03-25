#include <globals.h>
#include <web_handlers.h>

void NotFound(AsyncWebServerRequest* request){
    request->send(404, "text/plain", "ERROR 404: PAGE NOT FOUND");
}

void OnEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* payload, size_t len){
    switch(type){
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            HandleWebSocketMessage(arg, payload, len);
            break;
        default: 
            break;
    }
}

void HandleWebSocketMessage(void* arg, uint8_t* payload, size_t len){
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    // Make sure that the websocket frame received is actually the one with the message (payload)
    if(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT){
        payload[len] = 0;       // null-terminate the payload array so it can be converted to String type
        String msg = (char*)payload;

        if(strcmp((char*)payload, "GetValues") == 0){
            // TODO: Update the connected WebSocket clients
            Serial.println("Got GetValues");
            return;
        }

        JsonDocument rec_json;
        DeserializationError err = deserializeJson(rec_json, msg);

        if (err) {
            Serial.print("Failed to parse JSON: ");
            Serial.println(err.c_str());
            return;
        }

        Serial.println("Received from WebSocket: " + msg);

        if (rec_json["message"].is<String>()) {
            TestAction(rec_json);
        }
    }
}

void TestAction(JsonDocument& rec_json) {
    String msg = rec_json["message"];
    Serial.print(msg);
}