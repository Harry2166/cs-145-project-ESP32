#include <Arduino.h>
#include <WiFi.h>
#include <Private.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <map>

struct Stoplight {
  int id;
  const byte red_led;
  const byte yellow_led;
  const byte green_led;
  int red_led_status;
  int yellow_led_status;
  int green_led_status;
};

enum State {
  ACTIVE,
  INACTIVE
};

// Global Variables
HTTPClient client;
struct Stoplight stoplight1 = {0, 32, 25, 26, LOW, LOW, LOW};
struct Stoplight stoplight2 = {1, 27, 14, 12, LOW, LOW, LOW};
enum State currentState = INACTIVE;
WebSocketsClient webSocket;
int counter = 0;
int stoplightID;
unsigned long lastUpdateTime = 0;
const unsigned long onLightInterval= 5000; 
const unsigned long onYellowLightInterval= 2500; 
JsonDocument doc;
std::map<int, Stoplight> stoplightsMap = {
  {stoplight1.id, stoplight1}, 
  {stoplight2.id, stoplight2}
};

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);

void lightUpStoplight(Stoplight &stoplight){
  if (!LED_MODE) {
    digitalWrite(stoplight.red_led, stoplight.red_led_status);
    digitalWrite(stoplight.yellow_led, stoplight.yellow_led_status);
    digitalWrite(stoplight.green_led, stoplight.green_led_status);
  } else {
    digitalWrite(stoplight.red_led, !stoplight.red_led_status);
    digitalWrite(stoplight.yellow_led, !stoplight.yellow_led_status);
    digitalWrite(stoplight.green_led, !stoplight.green_led_status);
  }
}

void overwriteStoplight(Stoplight &stoplight, int red_status, int yellow_status, int green_status) {
  stoplight.red_led_status = red_status;
  stoplight.yellow_led_status = yellow_status;
  stoplight.green_led_status = green_status;
}

void setupStoplight(Stoplight &stoplight) {
  pinMode(stoplight.red_led, OUTPUT);
  pinMode(stoplight.yellow_led, OUTPUT);
  pinMode(stoplight.green_led, OUTPUT);
}

void startingStoplightSetup(Stoplight &stoplight) {
  overwriteStoplight(stoplight, LOW, HIGH, HIGH);
  lightUpStoplight(stoplight);
  delay(500);
  overwriteStoplight(stoplight, HIGH, LOW, HIGH);
  lightUpStoplight(stoplight);
  delay(500);
  overwriteStoplight(stoplight, HIGH, HIGH, LOW);
  lightUpStoplight(stoplight);
  delay(500);
  overwriteStoplight(stoplight, HIGH, HIGH, HIGH);
  lightUpStoplight(stoplight);
}

void setup() {
  Serial.begin(921600);
  pinMode(LED_BUILTIN, OUTPUT);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  setupStoplight(stoplight1);
  startingStoplightSetup(stoplight1);
  Serial.println("Connecting to ws");
  webSocket.begin(WS_HOST, WS_PORT, WS_URL); 
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);

}

void turnIntoJsonDocument(String payload, JsonDocument &doc){
  char json[1024];
  Serial.println(payload);
  payload.replace("\n", "");
  payload.trim();
  payload.toCharArray(json, 1024);
  deserializeJson(doc, json);
}

void loop() {
  webSocket.loop();
  unsigned long currentTime = millis();
  if (
    (
      currentState == INACTIVE && 
      ((counter % 4) % 2 == 1) && 
      currentTime - lastUpdateTime >= onLightInterval
    ) ||
    (
      currentState == INACTIVE && 
      ((counter % 4) % 2 == 0) && 
      currentTime - lastUpdateTime >= onYellowLightInterval
    ) 
  ) {
    switch(counter % 4) {
      case 0:
        overwriteStoplight(stoplight1, LOW, HIGH, HIGH);
        // Serial.println("red");
        break;
      case 1: case 3:
        overwriteStoplight(stoplight1, HIGH, LOW, HIGH);
        // Serial.println("yellow");
        break;
      case 2:
        overwriteStoplight(stoplight1, HIGH, HIGH, LOW);
        // Serial.println("green");
        break;
      }
    lightUpStoplight(stoplight1);
    counter += 1;
    lastUpdateTime = currentTime;
  } 
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length){
  switch (type) {
    case WStype_CONNECTED: {
      Serial.println("connected to ws://" + String(WS_HOST) + String(WS_URL));
      digitalWrite(LED_BUILTIN, HIGH); 
      webSocket.sendTXT("{\"message\": \"ACK\"}");
      break;
    }
    case WStype_DISCONNECTED: {
      Serial.println("WebSocket client disconnected");
      break;
    }
    case WStype_TEXT: {
      Serial.print("Received: ");
      Serial.println((char*)payload);

      // parsing JSON here
      turnIntoJsonDocument((char*)payload, doc);
      int groupID = doc["groupID"];
      stoplightID = doc["stoplightID"];

      if (groupID != STOPLIGHT_GROUP_ID) {
        currentState = INACTIVE;
      } else {
        currentState = ACTIVE;
      } 
      break;
    }
    default: {
      break;
    }
  }
}
