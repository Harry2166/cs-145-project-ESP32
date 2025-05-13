#include <Arduino.h>
#include <WiFi.h>
#include <Private.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <map>

/*
stoplight with id x will have the cycles as: red -> green -> yellow
  + red (32) 
  + yellow (25)
  + green (26)
stoplight with id x+1 will have the cycles as: green -> yellow -> red
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

enum TransitionState {
  NONE, // whether it is comfortably inactvive / active
  TO_PREACTIVE, // preactive state is the state as it is going to the active state (ie it is ACTIVE but still trying to become red/green)
  TO_ACTIVE, // statement that keeps track of whether it is comfortably in the active state
  TO_INACTIVE // to be used during the transition from active -> inactive
};

// Global Variables
HTTPClient client;
WebSocketsClient webSocket;
struct Stoplight stoplight1 = {1, 32, 25, 26, LOW, HIGH, HIGH, 0, 0}; 
struct Stoplight stoplight2 = {2, 27, 14, 12, HIGH, HIGH, LOW, 2, 2};
enum State currentState = INACTIVE;
enum TransitionState currentTransitionState = NONE;
int activeStoplightID; // global variable to keep track of which stoplight is active

// make it so that onRedLightInterval > onYellowLightInterval + onGreenLightInterval
const unsigned long onRedLightInterval = 3500; // length of time (in ms) that the red light is on 
const unsigned long onYellowLightInterval = 1000; // length of time (in ms) that the yellow light is on
const unsigned long onGreenLightInterval = 2000; // length of time (in ms) that the green light is on
const unsigned long transitionLightInterval = 500; // length of time (in ms) that the transition is occuring
unsigned long toInactiveTransitionTime = 0; // this is the time wherein it has just became inactive when it was originally active
unsigned long toActiveTransitionTime = 0; // tracks the time when it became yellow due to being in preactive state
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
    lightUpStoplight(*pair.second);
  }
}
/*
 *
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
 * The function that transitions to the normal inactive state; Assumes that currentTransitionState == TO_INACTIVE
 * @params currentTime: The time at which the stoplight changed
 */
void toInactiveStoplights(unsigned long currentTime){
  resetAllStoplightLastUpdateTime(currentTime);
  allStoplightsColorsToDefault(); 
  putAllStoplightCountersToStart();
  currentTransitionState = NONE;
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
 * Calls the inactiveStoplight function to all the stoplights; This function assumes that currentTransitionState == NONE and currentState == INACTIVE
 * @param currentTime: The time at which the stoplight changed
 */
void allInactiveStoplights(unsigned long currentTime) {
  for (auto& pair : stoplightsMap) {
    inactiveStoplight(*pair.second, currentTime);
  }
}

/** 
 * The function in charge of handling the stoplights if the emergency vehicle just became nearby (if it has just switched to active, change stoplight to yellow); This function assumes that currentTransitionState == TO_PREACTIVE and currentState == ACTIVE 
 * @param stoplight: The stoplight to be cycled
 * @param currentTime: The time at which the stoplight changed
 */
void toPreactiveStoplight(Stoplight &stoplight) {
  Serial.println("Preactive State: " + String(stoplight.id));
  overwriteStoplight(stoplight, HIGH, LOW, HIGH);
  lightUpStoplight(stoplight);
}

/** 
 * The function in charge of handling the stoplights if the emergency vehicle is nearby (if it is already active and the yellow time is up, change the stoplight to their respective colors); This function assumes that currentTransitionState == TO_ACTIVE and currentState == ACTIVE 
 * @param stoplight: The stoplight to be cycled
 * @param currentTime: The time at which the stoplight changed
 */
void toActiveStoplight(Stoplight &stoplight) {
  Serial.println("Active State: " + String(stoplight.id));
  if (activeStoplightID == stoplight.id) {
    overwriteStoplight(stoplight, HIGH, HIGH, LOW); // turn green
  } else {
    overwriteStoplight(stoplight, LOW, HIGH, HIGH); // turn red
  }
  lightUpStoplight(stoplight);
}

/**
 * Calls the `toPreactiveStoplight` or `toActiveStoplight` function to all the stoplights; This function assumes that currentTransitionState == TO_ACTIVE / TO_PREACTIVE and currentState == ACTIVE
 * @param currentTime: The time at which the stoplight changed
 */
void allActiveStoplights(unsigned long currentTime) {

  int isComfortablyActive = currentTime - toActiveTransitionTime >= transitionLightInterval && currentTransitionState == TO_ACTIVE;

  for (auto& pair : stoplightsMap) {
    if (currentTransitionState == TO_PREACTIVE) toPreactiveStoplight(*pair.second);
    else if (isComfortablyActive) toActiveStoplight(*pair.second);
  }

  if (currentTransitionState == TO_PREACTIVE) {
    currentTransitionState = TO_ACTIVE;
    toActiveTransitionTime = currentTime;
  }
  else if (isComfortablyActive) currentTransitionState = NONE;
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
  unsigned long currentTime = millis(); // calling this here rn and not within each function so that each time would be synced

  // if it is inactive and it has already transitioned to the inactive state and it is time to change
  if (currentState == INACTIVE && currentTime - toInactiveTransitionTime >= transitionLightInterval && currentTransitionState == TO_INACTIVE) {
    toInactiveStoplights(currentTime);
  } else if (currentState == INACTIVE && currentTransitionState == NONE) {
    allInactiveStoplights(currentTime);
  } else if (currentState == ACTIVE && (currentTransitionState == TO_PREACTIVE || currentTransitionState == TO_ACTIVE)) { 
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
      digitalWrite(LED_BUILTIN, LOW); 
      break;
    }
    case WStype_TEXT: {
      Serial.print("Received: ");
      Serial.println((char*)payload);

      // parsing JSON here
      turnIntoJsonDocument((char*)payload, doc);
      int status = doc["activate"];
      int groupID = doc["groupID"];
      activeStoplightID = doc["stoplightID"];

      // if the emergency vehicle is nearby (make active) 
      if (status == 1 && groupID == STOPLIGHT_GROUP_ID) {
        currentState = ACTIVE;
        currentTransitionState = TO_PREACTIVE;

      // if the emergency vehicle is not nearby (make inactive)
      } else { 

        // this is the instance that the emergency vehicle is exiting the radius, switch transition state to TO_INACTIVE; make all stoplights yellow
        if (groupID == STOPLIGHT_GROUP_ID) {
          currentTransitionState = TO_INACTIVE;
          toInactiveTransitionTime = millis();
          allStoplightsSameColor(HIGH, LOW, HIGH); // make all stoplights yellow
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
