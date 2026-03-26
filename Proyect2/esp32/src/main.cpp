#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <globals.h>
#include <server.h>
#include <init.h>
#include <web_handlers.h>

// -----------------------------------------------------------------------------
// Web server + websocket
// -----------------------------------------------------------------------------
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// -----------------------------------------------------------------------------
// Pines
// -----------------------------------------------------------------------------
#define DHT_PIN     4
#define DHT_TYPE    DHT11      // Cambia a DHT11 si tu sensor real es DHT11
#define SERVO_PIN   18
#define POT_PIN     34
#define BUTTON_PIN  25         // HW-483
#define BUZZER_PIN  27         // HW-512

// -----------------------------------------------------------------------------
// Objetos
// -----------------------------------------------------------------------------
DHT dht(DHT_PIN, DHT_TYPE);
Servo myServo;

// -----------------------------------------------------------------------------
// Variables principales
// -----------------------------------------------------------------------------
float temperatura = NAN;
float humedad = NAN;

int servoAngle = 0;
int sweepDirection = 1;

bool autoMode = false;
bool buttonSweepActive = false;

// Posición objetivo desde el potenciómetro.
// Cuando termina el barrido manual, el servo vuelve a esta posición.
int potTargetAngle = 0;

// Timers
unsigned long lastDhtRead = 0;
const unsigned long DHT_INTERVAL = 2000;

unsigned long lastServoStep = 0;
const unsigned long SERVO_STEP_INTERVAL = 20;

unsigned long lastBroadcast = 0;
const unsigned long BROADCAST_INTERVAL = 500;

// Debounce del botón
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_MS = 50;
int lastButtonReading = HIGH;
int buttonState = HIGH;

// -----------------------------------------------------------------------------
// Buzzer
// -----------------------------------------------------------------------------
bool buzzerActive = false;
unsigned long buzzerStartTime = 0;
const unsigned long BUZZER_DURATION = 300;

// -----------------------------------------------------------------------------
// Utilidades
// -----------------------------------------------------------------------------
int mapPotToAngle(int potValue) {
  return map(potValue, 0, 4095, 0, 180);
}

void setServoAngle(int angle) {
  angle = constrain(angle, 0, 180);
  myServo.write(angle);
  servoAngle = angle;
}

// -----------------------------------------------------------------------------
// Buzzer
// -----------------------------------------------------------------------------
void startBuzzer() {
  digitalWrite(BUZZER_PIN, HIGH);
  buzzerActive = true;
  buzzerStartTime = millis();
}

void updateBuzzer() {
  if (buzzerActive && (millis() - buzzerStartTime >= BUZZER_DURATION)) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
  }
}

void sendState() {
  JsonDocument doc;
  doc["temperature"] = isnan(temperatura) ? NAN : temperatura;
  doc["humidity"] = isnan(humedad) ? NAN : humedad;
  doc["servo_angle"] = servoAngle;
  doc["pot_target_angle"] = potTargetAngle;
  doc["auto_mode"] = autoMode;
  doc["button_sweep_active"] = buttonSweepActive;

  String jsonString;
  serializeJson(doc, jsonString);
  ws.textAll(jsonString);

  Serial.print("WS -> ");
  Serial.println(jsonString);
}

void readDhtSensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Error leyendo el DHT");
    return;
  }

  humedad = h;
  temperatura = t;
  autoMode = (temperatura > 25.0f);

  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.print(" C | Humedad: ");
  Serial.print(humedad);
  Serial.println(" %");
}

void updateAutoSweep() {
  if (millis() - lastServoStep < SERVO_STEP_INTERVAL) return;
  lastServoStep = millis();

  int nextAngle = servoAngle + (2 * sweepDirection);

  if (nextAngle >= 180) {
    nextAngle = 180;
    sweepDirection = -1;
  } else if (nextAngle <= 0) {
    nextAngle = 0;
    sweepDirection = 1;
  }

  setServoAngle(nextAngle);
}

void updateButtonSweep() {
  if (millis() - lastServoStep < SERVO_STEP_INTERVAL) return;
  lastServoStep = millis();

  int nextAngle = servoAngle + (2 * sweepDirection);

  if (nextAngle >= 180) {
    nextAngle = 180;
    sweepDirection = -1;
    setServoAngle(nextAngle);
    return;
  }

  if (nextAngle <= 0 && sweepDirection == -1) {
    nextAngle = 0;
    setServoAngle(nextAngle);
    buttonSweepActive = false;

    // Al terminar el barrido, vuelve a la posición actual del potenciómetro
    setServoAngle(potTargetAngle);
    return;
  }

  setServoAngle(nextAngle);
}

void updateManualPotControl() {
  int potValue = analogRead(POT_PIN);
  potTargetAngle = mapPotToAngle(potValue);
  setServoAngle(potTargetAngle);
}

void checkButton() {
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_MS) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW && !autoMode && !buttonSweepActive) {
        // Guardar la posición actual del potenciómetro
        int potValue = analogRead(POT_PIN);
        potTargetAngle = mapPotToAngle(potValue);

        buttonSweepActive = true;
        sweepDirection = 1;
        startBuzzer();
        setServoAngle(0);
        Serial.println("Barrido manual iniciado");
      }
    }
  }

  lastButtonReading = reading;
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  WiFi.softAP(AP_NAME);
  Serial.print("\nWebServer listening on: ");
  Serial.println(WiFi.softAPIP());

  if (!InitComponent("LittleFS", []() -> bool { return LittleFS.begin(true); })) return;
  if (!InitComponent("mDNS", []() -> bool { return MDNS.begin(mDNS_NAME); })) return;

  InitWebSocket();
  InitWebServer();

  dht.begin();

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);

  myServo.setPeriodHertz(50);
  myServo.attach(SERVO_PIN, 500, 2400);
  setServoAngle(0);

  Serial.println("Sistema iniciado");
  Serial.println("Lectura DHT cada 2 segundos");
  Serial.println("Temperatura > 25 C -> barrido automatico continuo");
  Serial.println("Temperatura <= 25 C -> control manual con potenciometro");
  Serial.println("Boton -> barrido completo y regreso a la posicion del potenciometro");
  sendState();
}

// -----------------------------------------------------------------------------
// Loop
// -----------------------------------------------------------------------------
void loop() {
  ws.cleanupClients();

  unsigned long now = millis();

  if (now - lastDhtRead >= DHT_INTERVAL) {
    lastDhtRead = now;
    readDhtSensor();
  }

  checkButton();
  updateBuzzer();

  if (autoMode) {
    updateAutoSweep();
  } else if (buttonSweepActive) {
    updateButtonSweep();
  } else {
    updateManualPotControl();
  }

  if (now - lastBroadcast >= BROADCAST_INTERVAL) {
    lastBroadcast = now;
    sendState();
  }
}