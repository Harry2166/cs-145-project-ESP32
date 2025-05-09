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
  int id; // stoplightID
  const byte red_led; // the GPIO Pin corresponding to the red_led
  const byte yellow_led; // the GPIO Pin corresponding to the yellow_led 
  const byte green_led; // the GPIO Pin corresponding to the green_led
  int red_led_status; // the state of the red led (HIGH / LOW)
  int yellow_led_status; // the state of the yellow led (HIGH / LOW)
  int green_led_status; // the state of the green led (HIGH / LOW)
  int default_red_led_status; // the default state of the red led (HIGH / LOW)
  int default_yellow_led_status; // the default state of the red led (HIGH / LOW)
  int default_green_led_status; // the default state of the red led (HIGH / LOW)
  int counter; // this counter is for the back and forth between red, yellow, and green
  int startingCounter; // this counter is to go back to the default setup again
  unsigned long lastUpdateTime; // the last time that it was updated / changed

  // constructor for building a stoplight at default -> meaning you set values for id, red, yellow, etc. 
  Stoplight(int _id, byte _red, byte _yellow, byte _green, int _red_led_status, int _yellow_led_status, int _green_led_status, int _counter = 0, int _startingCounter = 0, unsigned long _lastUpdateTime = 0)
    : id(_id),
      red_led(_red),
      yellow_led(_yellow),
      green_led(_green),
      red_led_status(_red_led_status),
      yellow_led_status(_yellow_led_status),
      green_led_status(_green_led_status),
      default_red_led_status(_red_led_status),
      default_yellow_led_status(_yellow_led_status),
      default_green_led_status(_green_led_status),
      counter(_counter),
      startingCounter(_startingCounter),
      lastUpdateTime(_lastUpdateTime)
  {}

};

/**
 * Represents if the ESP32 / Microcontroller is currently "active" (i.e. depending on the emergency vehicle) or "inactive" (just being a regular stoplight)
 */
enum State {
  ACTIVE,
  INACTIVE
};

// Global Variables
HTTPClient client;
WebSocketsClient webSocket;
struct Stoplight stoplight1 = {0, 32, 25, 26, LOW, HIGH, HIGH, 0, 0}; 
struct Stoplight stoplight2 = {1, 27, 14, 12, HIGH, HIGH, LOW, 2, 2};
enum State currentState = INACTIVE;
int activeStoplightID; // global variable to keep track of which stoplight is active
const unsigned long onRedLightInterval = 3500; // make it so that onRedLightInterval > onYellowLightInterval + onGreenLightInterval 
const unsigned long onYellowLightInterval = 1000; 
const unsigned long onGreenLightInterval = 2000; 
int hasTransitionedToPreActive = 0; // preactive state is the state as it is going to the active state (ie it is "technically" ACTIVE but still trying to become red/green)
int hasTransitionedToActive = 0; // statement that keeps track of whether it is comfortably in the active state
int hasTransitionedToInactive = 0; // to be used during the transition from active -> inactive
unsigned long toInactiveTransitionTime = 0; // this is the time wherein it has just became inactive when it was originally active
JsonDocument doc; // json document to store the websocket payload
std::map<int, Stoplight*> stoplightsMap = {
  {stoplight1.id, &stoplight1}, 
  {stoplight2.id, &stoplight2}
};

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);

/**
 * Lights up the stoplight given the current status passed on the {color}_led_status entries; LED_MODE is a constant found in Private.h wherein it takes the opposite of the stored input 
 * @param stoplight: the stoplight that will be lit up
 */
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

/**
 * Overwrites the stoplight status colors with the given inputs
 * @param red_status, yellow_status, green_status: HIGH / LOW input if you want the light to be on or off
 */
void overwriteStoplight(Stoplight &stoplight, int red_status, int yellow_status, int green_status) {
  stoplight.red_led_status = red_status;
  stoplight.yellow_led_status = yellow_status;
  stoplight.green_led_status = green_status;
}

/**
 * Readies the stoplight's stated GPIO pins for output
 * @param stoplight: stoplight that will be readied
 */
void setupStoplight(Stoplight &stoplight) {
  pinMode(stoplight.red_led, OUTPUT);
  pinMode(stoplight.yellow_led, OUTPUT);
  pinMode(stoplight.green_led, OUTPUT);
}

/**
 * Puts all the local stoplight counters back to their initial state
 */
void putAllStoplightCountersToStart() {
  for (auto& pair : stoplightsMap) {
    pair.second->counter = pair.second->startingCounter;
  }
}

/**
 * Turns the payload into a JSON file to be parsed
 * @param[in] payload: the payload of the webSocket
 * @param[out] doc: the output Json document
 */
void turnIntoJsonDocument(String payload, JsonDocument &doc){
  char json[1024];
  Serial.println(payload);
  payload.replace("\n", "");
  payload.trim();
  payload.toCharArray(json, 1024);
  deserializeJson(doc, json);
}

/** 
 * The function in charge of cycling the stoplights in their normal order without the emergency vehicle. The order of the colors go red -> green -> yellow (in terms of indices: 0->2->1) 
 * @param stoplight: The stoplight to be cycled
 * @param currentTime: The time at which the stoplight changed
 */
 void inactiveStoplight(Stoplight &stoplight, unsigned long currentTime) {
  // switching to red from yellow
  if (stoplight.counter == 1 && currentTime - stoplight.lastUpdateTime >= onYellowLightInterval) { 
    overwriteStoplight(stoplight, LOW, HIGH, HIGH);
    stoplight.counter = 0;
    lightUpStoplight(stoplight);
    stoplight.lastUpdateTime = currentTime;

  // switching to yellow from green
  } else if (stoplight.counter == 2 && currentTime - stoplight.lastUpdateTime >= onGreenLightInterval) { 
    overwriteStoplight(stoplight, HIGH, LOW, HIGH);
    stoplight.counter = 1;
    lightUpStoplight(stoplight);
    stoplight.lastUpdateTime = currentTime;

  // switching to green from red
  } else if (stoplight.counter == 0 && currentTime - stoplight.lastUpdateTime >= onRedLightInterval) { 
    overwriteStoplight(stoplight, HIGH, HIGH, LOW);
    stoplight.counter = 2;
    lightUpStoplight(stoplight);
    stoplight.lastUpdateTime = currentTime;
  }
}

/**
 * Calls the `inactiveStoplight` function to all the stoplights
 * @param currentTime: The time at which the stoplight changed
 */
void allInactiveStoplights(unsigned long currentTime) {
  for (auto& pair : stoplightsMap) {
    inactiveStoplight(*pair.second, currentTime);
  }
}

/*
 * Sets the stoplights to the same colors
 * @param[in] red_status, yellow_status, green_status: HIGH / LOW input if you want the light to be on or off
 */
void allStoplightsSameColor(int red_status, int yellow_status, int green_status) {
  for (auto& pair : stoplightsMap) {
    overwriteStoplight(*pair.second, red_status, yellow_status, green_status);
    lightUpStoplight(*pair.second);
  }
}

/** 
 * The function in charge of handling the stoplights if the emergency vehicle is nearby. 
 * @param stoplight: The stoplight to be cycled
 * @param currentTime: The time at which the stoplight changed
 */
void activeStoplight(Stoplight &stoplight, unsigned long currentTime) {

  // if it has just switched to active, change all stoplights to yellow
  if (!hasTransitionedToPreActive) { 
    Serial.println("Preactive State");
    stoplight.lastUpdateTime = currentTime;
    hasTransitionedToPreActive = 1;
    allStoplightsSameColor(HIGH, LOW, HIGH);

    // if it is already active and the yellow time is up, change the stoplight to their respective colors
  } else if (currentTime - stoplight.lastUpdateTime >= onYellowLightInterval && hasTransitionedToPreActive && !hasTransitionedToActive) {
    Serial.println("Active State");
    if (activeStoplightID == stoplight.id) {
      overwriteStoplight(stoplight, HIGH, HIGH, LOW);
    } else {
      overwriteStoplight(stoplight, LOW, HIGH, HIGH);
    }
    lightUpStoplight(stoplight);
    hasTransitionedToActive = 1;
  }
}

/**
 * Calls the `activeStoplight` function to all the stoplights
 * @param currentTime: The time at which the stoplight changed
 */
void allActiveStoplights(unsigned long currentTime) {
  for (auto& pair : stoplightsMap) {
    activeStoplight(*pair.second, currentTime);
  }
}

/**
 * Resets all the stoplights' last update time to the current time; This is used when the active state is done
 * @param currentTime: The time at which the stoplight changed
 */
void resetAllStoplightLastUpdateTime(unsigned long currentTime) {
  for (auto& pair : stoplightsMap) {
    pair.second->lastUpdateTime = currentTime;
  }
}

/**
 * Puts the colors of the stoplights to their default color scheme
 */
void allStoplightsColorsToDefault() {
  for (auto& pair : stoplightsMap) {
    pair.second->red_led_status = pair.second->default_red_led_status; 
    pair.second->yellow_led_status = pair.second->default_yellow_led_status; 
    pair.second->green_led_status = pair.second->default_green_led_status; 
    lightUpStoplight(stoplight);
  }
}

/**
* Connects the given wifi and websocket; Also setups and lights up the stoplights
*/
void setup() {
  Serial.begin(921600);
  pinMode(LED_BUILTIN, OUTPUT);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  setupStoplight(stoplight1);
  setupStoplight(stoplight2);

  // connecting to the websocket
  Serial.println("connecting to ws://" + String(WS_HOST) + String(WS_URL));
  webSocket.begin(WS_HOST, WS_PORT, WS_URL); 
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);

  lightUpStoplight(stoplight1);
  lightUpStoplight(stoplight2);
}

/**
* will continously connect to the websocket and light up the stoplights
*/
void loop() {
  webSocket.loop();
  unsigned long currentTime = millis();
  // if it is inactive and it has already transitioned to the inactive state and it is time to change
  if (currentState == INACTIVE && currentTime - toInactiveTransitionTime >= onYellowLightInterval && hasTransitionedToInactive) {
    resetAllStoplightLastUpdateTime(currentTime);
    allStoplightsColorsToDefault(); 
    putAllStoplightCountersToStart();
    hasTransitionedToInactive = 0;
  } else if (currentState == INACTIVE) {
    allInactiveStoplights(currentTime);
  } else {
    allActiveStoplights(currentTime);
  }
}

/*
* The function in charge of handling the websocket events (i.e. when it receives data from /connects/disconnects to the websocket)
*/
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
      activeStoplightID = doc["stoplightID"];

      // if the emergency vehicle is nearby (make active) 
      if (status == 1 && groupID == STOPLIGHT_GROUP_ID) {
        currentState = ACTIVE;
        hasTransitionedToInactive = 0;

      // if the emergency vehicle is not nearby (make inactive)
      } else { 

        // this is when it returns status: 0, switch all the transition states to active back to 0 and the inactive transition state to 1; make all stoplights yellow
        if (groupID == STOPLIGHT_GROUP_ID) {
          hasTransitionedToPreActive = 0;
          hasTransitionedToActive = 0;
          hasTransitionedToInactive = 1;
          toInactiveTransitionTime = millis();
          allStoplightsSameColor(HIGH, LOW, HIGH); 
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
