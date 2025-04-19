#include <Arduino.h>
#include <WiFi.h>
#include <Private.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

bool isConnected = false;
HTTPClient client;
const byte red_led = 32;
const byte yellow_led = 25;
const byte green_led = 26;

void setup() {
  Serial.begin(921600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(red_led, OUTPUT);
  pinMode(yellow_led, OUTPUT);
  pinMode(green_led, OUTPUT);
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
