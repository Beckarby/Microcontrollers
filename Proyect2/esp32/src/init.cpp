#include <LittleFS.h>
#include <globals.h>
#include <server.h>

void InitWebSocket() {
    ws.onEvent(OnEvent);
    server.addHandler(&ws);
}

void InitWebServer() {
    // Assign the files to serve when requesting an address on the server.
    // Only "/" is available. Any other address will result in 404: Not found
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request)
             { request->send(LittleFS, "/webpage/index.html", "text/html"); });

    server.serveStatic("/webpage", LittleFS, "/webpage");
    server.serveStatic("/websockets", LittleFS, "/websockets");
    server.onNotFound(NotFound);

    // Start the server
    server.begin();
}