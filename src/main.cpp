#include <Arduino.h>
#include <WiFi.h>
#include <Private.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <map>

/*
stoplight with id 0 will have the cycles as: red -> green -> yellow
  + red (32) 
  + yellow (25)
  + green (26)
stoplight with id 1 will have the cycles as: green -> yellow -> red
  + red (27) 
  + yellow (14)
  + green (12)
*/

struct Stoplight {
  int id;
  const byte red_led;
  const byte yellow_led;
  const byte green_led;
  int red_led_status;
  int yellow_led_status;
  int green_led_status;
  int counter; // this counter is for the back and forth between red, yellow, and green
  int startingCounter; // this counter is to setup the default setup again
  unsigned long lastUpdateTime;

  // constructor for building a stoplight at default -> meaning that you set values for id, red, yellow, etc. 
  Stoplight(int _id, byte _red, byte _yellow, byte _green, int _red_led_status, int _yellow_led_status, int _green_led_status, int _counter = 0, int _startingCounter = 0, unsigned long _lastUpdateTime = 0)
    : id(_id),
      red_led(_red),
      yellow_led(_yellow),
      green_led(_green),
      red_led_status(_red_led_status),
      yellow_led_status(_yellow_led_status),
      green_led_status(_green_led_status),
      counter(_counter),
      startingCounter(_startingCounter),
      lastUpdateTime(_lastUpdateTime)
  {}

};

enum State {
  ACTIVE,
  INACTIVE
};

// Global Variables
HTTPClient client;
struct Stoplight stoplight1 = {0, 32, 25, 26, LOW, HIGH, HIGH, 0, 0}; 
struct Stoplight stoplight2 = {1, 27, 14, 12, HIGH, HIGH, LOW, 2, 2};
enum State currentState = INACTIVE;
WebSocketsClient webSocket;
int stoplightID;
unsigned long lastUpdateTime = 0;
const unsigned long onRedLightInterval = 3500; // make it so that onRedLightInterval > onYellowLightInterval + onGreenLightInterval 
const unsigned long onYellowLightInterval = 1000; 
const unsigned long onGreenLightInterval = 2000; 
int hasTransitionedToPreActive = 0; // preactive state is the state as it is going to the active state
int hasTransitionedToActive = 0;
JsonDocument doc;
std::map<int, Stoplight*> stoplightsMap = {
  {stoplight1.id, &stoplight1}, 
  {stoplight2.id, &stoplight2}
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
  overwriteStoplight(stoplight, LOW, LOW, LOW);
  lightUpStoplight(stoplight);
  delay(500);
  overwriteStoplight(stoplight, HIGH, HIGH, HIGH);
  lightUpStoplight(stoplight);
  delay(500);
}

void putAllStoplightCountersToStart() {
  for (auto& pair : stoplightsMap) {
    pair.second->counter = pair.second->startingCounter;
  }
}

void turnIntoJsonDocument(String payload, JsonDocument &doc){
  char json[1024];
  Serial.println(payload);
  payload.replace("\n", "");
  payload.trim();
  payload.toCharArray(json, 1024);
  deserializeJson(doc, json);
}

// red -> green -> yellow
//  0  ->   2   ->   1
void inactiveStoplight(Stoplight &stoplight, unsigned long currentTime) {
  if (stoplight.counter == 1 && currentTime - stoplight.lastUpdateTime >= onYellowLightInterval) { // switching to red from yellow 
    overwriteStoplight(stoplight, LOW, HIGH, HIGH);
    stoplight.counter = 0;
    lightUpStoplight(stoplight);
    stoplight.lastUpdateTime = currentTime;
  } else if (stoplight.counter == 2 && currentTime - stoplight.lastUpdateTime >= onGreenLightInterval) { // switching to yellow from green 
    overwriteStoplight(stoplight, HIGH, LOW, HIGH);
    stoplight.counter = 1;
    lightUpStoplight(stoplight);
    stoplight.lastUpdateTime = currentTime;
  } else if (stoplight.counter == 0 && currentTime - stoplight.lastUpdateTime >= onRedLightInterval) { // switching to green from red 
    overwriteStoplight(stoplight, HIGH, HIGH, LOW);
    stoplight.counter = 2;
    lightUpStoplight(stoplight);
    stoplight.lastUpdateTime = currentTime;
  }
}

void allInactiveStoplights(unsigned long currentTime) {
  for (auto& pair : stoplightsMap) {
    inactiveStoplight(*pair.second, currentTime);
  }
}

void allStoplightsSameColor(int red_status, int yellow_status, int green_status) {
  for (auto& pair : stoplightsMap) {
    overwriteStoplight(*pair.second, red_status, yellow_status, green_status);
    lightUpStoplight(*pair.second);
  }
}

void activeStoplight(Stoplight &stoplight, unsigned long currentTime) {
  if (!hasTransitionedToPreActive) { // switching to a non-yellow color
    stoplight.lastUpdateTime = currentTime;
    hasTransitionedToPreActive = 1;
    allStoplightsSameColor(HIGH, LOW, HIGH);
  } else if (currentTime - stoplight.lastUpdateTime >= onYellowLightInterval && hasTransitionedToPreActive && !hasTransitionedToActive) {
    if (stoplightID == stoplight.id) {
      overwriteStoplight(stoplight, HIGH, HIGH, LOW);
    } else {
      overwriteStoplight(stoplight, LOW, HIGH, HIGH);
    }
    lightUpStoplight(stoplight);
    hasTransitionedToActive = 1;
  }
}

void allActiveStoplights(unsigned long currentTime) {
  for (auto& pair : stoplightsMap) {
    activeStoplight(*pair.second, currentTime);
  }
}

void resetAllStoplightLastUpdateTime() {
  for (auto& pair : stoplightsMap) {
    pair.second->lastUpdateTime = millis();
  }
}

void setup() {
  Serial.begin(921600);
  pinMode(LED_BUILTIN, OUTPUT);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  setupStoplight(stoplight1);
  /*startingStoplightSetup(stoplight1);*/
  setupStoplight(stoplight2);
  /*startingStoplightSetup(stoplight2);*/
  Serial.println("Connecting to ws");
  /*webSocket.begin(WS_HOST, WS_PORT, WS_URL); */
  /*webSocket.onEvent(webSocketEvent);*/
  /*webSocket.setReconnectInterval(5000);*/
  lightUpStoplight(stoplight1);
  lightUpStoplight(stoplight2);
}

void loop() {
  /*webSocket.loop();*/
  unsigned long currentTime = millis();
  if (currentState == INACTIVE) {
    allInactiveStoplights(currentTime);
  } else {
    allActiveStoplights(currentTime);
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
      int status = doc["status"];
      int groupID = doc["groupID"];
      stoplightID = doc["stoplightID"];

      if (status == 1 && groupID == STOPLIGHT_GROUP_ID) {
        currentState = ACTIVE;
        putAllStoplightCountersToStart();
      } else { // status == 0
        if (groupID == STOPLIGHT_GROUP_ID) {
          hasTransitionedToPreActive = 0;
          hasTransitionedToActive = 0;
          resetAllStoplightLastUpdateTime();
        }
        currentState = INACTIVE;
      } 
      // webSocket.sendTXT("{\"message\": \"ACK\", \"groupID\": " + String(STOPLIGHT_GROUP_ID) + "}");
      break;
    }
    default: {
      break;
    }
  }
}
