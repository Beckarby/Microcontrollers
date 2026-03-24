#pragma once

void InitWebServer();
void InitWebSocket();

template <typename F>
bool InitComponent(const char* componentName, F InitFunction){
    Serial.print("-> Starting "); Serial.print(componentName); Serial.println("...");

    int timeoutCount = TIMEOUT;
    while(!InitFunction()){
        delay(1000);
        Serial.print('.');
        timeoutCount--;
        if(timeoutCount == 0){
            Serial.println("\n[ ERR ] COULD NOT START " + String(componentName) + ".");
            return false;
        }
    }

    Serial.println("[ OK ] Started " + String(componentName) + ".");

    return true;
}