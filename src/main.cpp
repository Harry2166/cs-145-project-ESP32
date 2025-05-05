#include <Arduino.h>
#include <WiFi.h>
#include <Private.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

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

HTTPClient client;
struct Stoplight stoplight1 = {0, 32, 25, 26, LOW, LOW, LOW};
// struct Stoplight stoplight2 = {1, 27, 14, 12, LOW, LOW, LOW};
WebSocketsClient webSocket;
enum State currentState = INACTIVE;
int counter = 0;

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);

void lightUpStoplight(Stoplight &stoplight){
  digitalWrite(stoplight.red_led, stoplight.red_led_status);
  digitalWrite(stoplight.yellow_led, stoplight.yellow_led_status);
  digitalWrite(stoplight.green_led, stoplight.green_led_status);
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

void loop() {
  webSocket.loop();
  if (currentState == INACTIVE) {
    switch(counter % 3) {
    case 0:
      overwriteStoplight(stoplight1, LOW, HIGH, HIGH);
      break;
    case 1:
      overwriteStoplight(stoplight1, HIGH, LOW, HIGH);
      break;
    case 2:
      overwriteStoplight(stoplight1, HIGH, HIGH, LOW);
      break;
    }
    lightUpStoplight(stoplight1);
    counter += 1;
  }
  delay(1000);
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length){
  switch (type) {
    case WStype_CONNECTED:
        Serial.println("connected to" + String(WS_HOST) + String(WS_URL));
        webSocket.sendTXT("{\"message\": \"ACK\"}");
        break;
    case WStype_DISCONNECTED:
        Serial.println("WebSocket client disconnected");
        break;
    case WStype_TEXT:
        Serial.print("Received: ");
        Serial.println((char*)payload);
        // insert parsing shit here
        break;
    default:
        break;
  }
}
