#include <Arduino.h>
#include <WiFi.h>
#include <Private.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

struct Stoplight {
  int id;
  const byte red_led;
  const byte yellow_led;
  const byte green_led;
  int red_led_status;
  int yellow_led_status;
  int green_led_status;
};

// bool isConnected = false;
HTTPClient client;
// const byte red_led = 32;
// const byte yellow_led = 25;
// const byte green_led = 26;
struct Stoplight stoplight1 = {0, 32, 25, 26, LOW, LOW, LOW};
// struct Stoplight stoplight2 = {1, 27, 14, 12, LOW, LOW, HIGH};

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
  pinMode(stoplight1.red_led, OUTPUT);
  pinMode(stoplight1.yellow_led, OUTPUT);
  pinMode(stoplight1.green_led, OUTPUT);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  startingStoplightSetup(stoplight1);

}

void turnIntoJsonDocument(String payload, JsonDocument &doc){
  char json[1024];
  Serial.println(payload);
  payload.replace("\n", "");
  payload.trim();
  payload.toCharArray(json, 1024);
  deserializeJson(doc, json);
}

void accessJokeAPI(JsonDocument &doc){
  int id = doc["id"];
  const char* setup = doc["setup"];
  const char* delivery = doc["delivery"];
  Serial.println("[" + String(id) + "] " + String(setup) + " = " + String(delivery));
  Serial.println(id);
  if (id % 3 == 0) {
    overwriteStoplight(stoplight1, HIGH, LOW, LOW);
    setupStoplight(stoplight1);
  } else if (id % 3 == 1) {
    overwriteStoplight(stoplight1, LOW, HIGH, LOW);
    setupStoplight(stoplight1);
  } else if (id % 3 == 2) {
    overwriteStoplight(stoplight1, LOW, LOW, HIGH);
    setupStoplight(stoplight1);
  }
}

void loop() {
  overwriteStoplight(stoplight1, LOW, HIGH, HIGH);
  setupStoplight(stoplight1);
  Serial.println("red");
  delay(5000);
  overwriteStoplight(stoplight1, HIGH, LOW, HIGH);
  setupStoplight(stoplight1);
  Serial.println("yellow");
  delay(5000);
  overwriteStoplight(stoplight1, HIGH, HIGH, LOW);
  setupStoplight(stoplight1);
  Serial.println("green");
  delay(5000);
  overwriteStoplight(stoplight1, HIGH, HIGH, HIGH);
  setupStoplight(stoplight1);
  Serial.println("off");
  delay(5000);
  // if (WiFi.status() == WL_CONNECTED && !isConnected) {
  //   Serial.println("Connected");
  //   digitalWrite(LED_BUILTIN, HIGH);
  //   isConnected = true;
  // }

  // if (WiFi.status() != WL_CONNECTED) {
  //   Serial.println("...");
  //   digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  //   delay(1000);
  //   isConnected = false;
  // }

  // if (isConnected){
  //   client.begin(URL);
  //   int httpCode = client.GET();
  //   if (httpCode > 0) {
  //     digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  //     delay(1000);
  //     String payload = client.getString();
  //     delay(1000);
  //     digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

  //     JsonDocument doc;
  //     turnIntoJsonDocument(payload, doc);
  //     accessJokeAPI(doc);

  //   } else {
  //     Serial.println("HTTP request err");
  //     isConnected = false;
  //   }
  //   delay(10000); // delay of ten seconds after every access 
  //   Serial.println("10 second delay done");
  // }
  // Serial.println("loop will now repeat");
}
