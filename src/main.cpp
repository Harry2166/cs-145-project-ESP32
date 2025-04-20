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

bool isConnected = false;
HTTPClient client;
const byte red_led = 32;
const byte yellow_led = 25;
const byte green_led = 26;
struct Stoplight stoplight1 = {0, 32, 25, 26, HIGH, LOW, LOW};
struct Stoplight stoplight2 = {1, 27, 14, 12, LOW, LOW, HIGH};

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

void setup() {
  Serial.begin(921600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(stoplight1.red_led, OUTPUT);
  pinMode(stoplight1.yellow_led, OUTPUT);
  pinMode(stoplight1.green_led, OUTPUT);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  digitalWrite(red_led, HIGH);
  delay(500);
  digitalWrite(red_led, LOW);
  digitalWrite(yellow_led, HIGH);
  delay(500);
  digitalWrite(yellow_led, LOW);
  digitalWrite(green_led, HIGH);
  delay(500);
  digitalWrite(green_led, LOW);

}

void turnIntoJsonDocument(String payload, StaticJsonDocument<1024> &doc){
  char json[1024];
  payload.replace("\n", "");
  payload.trim();
  payload.toCharArray(json, 1024);
  deserializeJson(doc, json);
}

void accessJokeAPI(StaticJsonDocument<1024> &doc){
  int id = doc["id"];
  const char* setup = doc["setup"];
  const char* delivery = doc["delivery"];
  Serial.println("[" + String(id) + "] " + String(setup) + " = " + String(delivery));
  Serial.println(id);
  if (id % 3 == 0) {
    digitalWrite(red_led, HIGH); 
    digitalWrite(yellow_led, LOW); 
    digitalWrite(green_led, LOW); 
  } else if (id % 3 == 1) {
    digitalWrite(red_led, LOW); 
    digitalWrite(yellow_led, HIGH); 
    digitalWrite(green_led, LOW); 
  } else if (id % 3 == 2) {
    digitalWrite(red_led, LOW); 
    digitalWrite(yellow_led, LOW); 
    digitalWrite(green_led, HIGH); 
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED && !isConnected) {
    // Serial.println("Connected");
    digitalWrite(LED_BUILTIN, HIGH);
    isConnected = true;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("...");
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(1000);
    isConnected = false;
  }

  if (isConnected){
    client.begin(URL);
    int httpCode = client.GET();
    if (httpCode > 0) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(1000);
      String payload = client.getString();
      delay(1000);
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

      StaticJsonDocument<1024> doc;
      turnIntoJsonDocument(payload, doc);
      accessJokeAPI(doc);

    } else {
      Serial.println("HTTP request err");
    }
    delay(10000); // delay of ten seconds after every access 
  }
}
