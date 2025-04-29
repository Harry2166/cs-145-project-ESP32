// #include <Arduino.h>
// #include <WiFi.h>
// #include <Private.h>
// #include <HTTPClient.h>
// #include <ArduinoJson.h>
// #include <WebSocketsClient.h>

// struct Stoplight {
//     int id;
//     const byte red_led;
//     const byte yellow_led;
//     const byte green_led;
//     int red_led_status;
//     int yellow_led_status;
//     int green_led_status;
// };

// bool isConnected = false;
// HTTPClient client;
// JsonDocument doc;
// struct Stoplight stoplight1 = {0, 32, 25, 26, HIGH, LOW, LOW};
// // struct Stoplight stoplight2 = {1, 27, 14, 12, LOW, LOW, HIGH};
// WebSocketsClient webSocket;

// void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);

// void setupStoplight(Stoplight &stoplight){
//     digitalWrite(stoplight.red_led, stoplight.red_led_status);
//     digitalWrite(stoplight.yellow_led, stoplight.yellow_led_status);
//     digitalWrite(stoplight.green_led, stoplight.green_led_status);
// }

// void overwriteStoplight(Stoplight &stoplight, int red_status, int yellow_status, int green_status){
//     stoplight.red_led_status = red_status;
//     stoplight.yellow_led_status = yellow_status;
//     stoplight.green_led_status = green_status;
// }

// void startingStoplightSetup(Stoplight &stoplight){
//     overwriteStoplight(stoplight, HIGH, LOW, LOW);
//     setupStoplight(stoplight);
//     delay(500);
//     overwriteStoplight(stoplight, LOW, HIGH, LOW);
//     setupStoplight(stoplight);
//     delay(500);
//     overwriteStoplight(stoplight, LOW, LOW, HIGH);
//     setupStoplight(stoplight);
//     delay(500);
//     overwriteStoplight(stoplight, LOW, LOW, LOW);
//     setupStoplight(stoplight);
// }

// void setStoplightOutputPin(Stoplight &stoplight){
//     pinMode(stoplight.red_led, OUTPUT);
//     pinMode(stoplight.yellow_led, OUTPUT);
//     pinMode(stoplight.green_led, OUTPUT);
// }

// void setup(){
//     Serial.begin(921600);
//     pinMode(LED_BUILTIN, OUTPUT);
//     setStoplightOutputPin(stoplight1);
//     WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//     if (WiFi.status() != WL_CONNECTED) {
//         Serial.println(".");
//         digitalWrite(LED_BUILTIN, HIGH);
//         // isConnected = true;
//     }

//     Serial.println("IP Address:");
//     Serial.println(WiFi.localIP());

//     startingStoplightSetup(stoplight1);

//         // Serial.println("Connecting to ws");
//         // webSocket.beginSSL(WS_HOST, WS_PORT, WS_URL, "", "wss"); 
//         // Serial.println("Connected to ws");
//     webSocket.onEvent(webSocketEvent);
//     webSocket.setReconnectInterval(5000);

// }

// void webSocketEvent(WStype_t type, uint8_t *payload, size_t length){
//     if(type == WStype_TEXT) {
//         DeserializationError err = deserializeJson(doc, payload);
//         if (err) {
//         Serial.println("Deserialization failed D:");
//         Serial.println(err.c_str());
//         return;
//         }
//         // Serial.println(doc);
//         int signal = doc["stoplight"];
//         if (signal == 0) {
//         overwriteStoplight(stoplight1, HIGH, LOW, LOW);
//         setupStoplight(stoplight1);
//         } else if (signal == 1) {
//         overwriteStoplight(stoplight1, LOW, HIGH, LOW);
//         setupStoplight(stoplight1);
//         } else if (signal == 2) {
//         overwriteStoplight(stoplight1, LOW, LOW, HIGH);
//         setupStoplight(stoplight1);
//         } else {
//         overwriteStoplight(stoplight1, LOW, LOW, LOW);
//         setupStoplight(stoplight1);
//         }
//     }

//     // Serial.println(doc);
//     webSocket.sendTXT("{\"stats\":\"OK\"}");

// }

// void loop(){
//     webSocket.loop();
// }
