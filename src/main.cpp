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

HTTPClient client;
struct Stoplight stoplight1 = {0, 32, 25, 26, LOW, LOW, LOW};
// struct Stoplight stoplight2 = {1, 27, 14, 12, LOW, LOW, HIGH};
WebSocketsClient webSocket;

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);

void setupStoplight(Stoplight &stoplight){
  digitalWrite(stoplight.red_led, stoplight.red_led_status);
  digitalWrite(stoplight.yellow_led, stoplight.yellow_led_status);
  digitalWrite(stoplight.green_led, stoplight.green_led_status);
}

void overwriteStoplight(Stoplight &stoplight, int red_status, int yellow_status, int green_status) {
  stoplight.red_led_status = red_status;
  stoplight.yellow_led_status = yellow_status;
  stoplight.green_led_status = green_status;
}

void startingStoplightSetup(Stoplight &stoplight) {
  overwriteStoplight(stoplight1, HIGH, LOW, LOW);
  setupStoplight(stoplight1);
  delay(500);
  overwriteStoplight(stoplight1, LOW, HIGH, LOW);
  setupStoplight(stoplight1);
  delay(500);
  overwriteStoplight(stoplight1, LOW, LOW, HIGH);
  setupStoplight(stoplight1);
  delay(500);
  overwriteStoplight(stoplight1, LOW, LOW, LOW);
  setupStoplight(stoplight1);
}

void setup() {
  Serial.begin(921600);
  pinMode(LED_BUILTIN, OUTPUT);
  // pinMode(stoplight1.red_led, OUTPUT);
  // pinMode(stoplight1.yellow_led, OUTPUT);
  // pinMode(stoplight1.green_led, OUTPUT);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // startingStoplightSetup(stoplight1);
  Serial.println("Connecting to ws");
  webSocket.begin("tabipo.xyz", 80, "/ws/simulation/"); 
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);

}

void loop() {
  webSocket.loop();
  // overwriteStoplight(stoplight1, LOW, HIGH, HIGH);
  // setupStoplight(stoplight1);
  // Serial.println("red");
  // delay(5000);
  // overwriteStoplight(stoplight1, HIGH, LOW, HIGH);
  // setupStoplight(stoplight1);
  // Serial.println("yellow");
  // delay(5000);
  // overwriteStoplight(stoplight1, HIGH, HIGH, LOW);
  // setupStoplight(stoplight1);
  // Serial.println("green");
  // delay(5000);
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length){
  switch (type) {
    case WStype_CONNECTED:
        Serial.println("WebSocket client connected");
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
