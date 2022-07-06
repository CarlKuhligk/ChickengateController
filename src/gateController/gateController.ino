#include <WiFi.h>
#include <MQTT.h>
#include <Arduino.h>
#include <Arduino_JSON.h>
#include "esp_system.h"
#include "LinearActuator.h"
#include "ota.h"
#include "config.h"

LinearActuator *Axis1;
Preferences MyPreference;

WiFiClient net;
MQTTClient client;

enum GateState {
  opening,
  opened,
  closing,
  closed
};

enum Action {
  openGate,
  closeGate
};

GateState gateState = opened;
Action action;

// online timer
unsigned long startTime;
unsigned long currentTime;
const unsigned long period = 1000;

void sendState(int position = (int)Axis1->getPosition());

void setup()
{
  // Deug only
  Serial.begin(115200);
  // debugled
  pinMode(LED_D0_PIN, OUTPUT);

  MyPreference.begin("VariableStorage", false);

  Axis1 = new LinearActuator(EN_PIN, PWM_PIN, CS_PIN, CH_A_PIN, CH_B_PIN, ENC_A_PIN, ENC_B_PIN, &MyPreference);

  // Connect to wifi and mqtt
  connect();
  client.onMessage(messageReceived);
  startTime = millis();

  // OTA webserver
  startOTAWebserver();

  // MOVEMENT
  Axis1->enable();

  // StartPosition
  Axis1->moveHome(80);
}

void loop()
{
  // wifi & mqtt
  connect();

  // OTA webserver
  otaWebserver();

  if (WiFi.status() == WL_CONNECTED)
  {
    // wifi is connected
    digitalWrite(LED_D0_PIN, HIGH);
  }

  client.loop();
  switch (action) {
    case openGate:
      if (gateState == closed) {
        gateState = opening;
        sendState();
        Axis1->moveAbsolute(0, 80., &sendState);
        gateState = opened;
        sendState();
      }
      break;
    case closeGate:
      if (gateState == opened) {
        gateState = closing;
        sendState();
        Axis1->moveAbsolute(250, 80., &sendState);
        gateState = closed;
        sendState();
      }
      break;
  }
  sendState();
}

void connect() {
  // WIFI
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    for (int i = 0; i < 15 && WiFi.status() != WL_CONNECTED; i++) {
      Serial.println("Reconnecting");
      digitalWrite(LED_D0_PIN, !digitalRead(LED_D0_PIN));
      delay(500);
    }
  }
  if (WiFi.status() == WL_CONNECTED) WiFi.localIP();

  // MQTT
  if (WiFi.status() == WL_CONNECTED && !client.connected()) {
    client.begin(mqttURL, mqttPort, net);
    client.onMessage(messageReceived);

    Serial.print("\n Connect to MQTT-Server:");
    while (!client.connect("ESP32GateController", mqttUser, mqttPassword)) {
      Serial.print(".");
      delay(500);
    }
    Serial.println("\nconnected!");

    client.subscribe("GateControl");
    client.publish("GateConnectionState", "connected");
    sendState();
    // client.unsubscribe("/hello");
  }
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  JSONVar myObject = JSON.parse(payload);

  if (topic == "GateControl") {
    if ((int)myObject["action"] == 1)
    {
      // open gate
      action = openGate;
    }
    else if ((int)myObject["action"] == 3)
    {
      // close gate
      action = closeGate;
    }
  }
}

void sendState(int position) {
  currentTime = millis();
  if (currentTime - startTime >= period)//test whether the period has elapsed
  {
    startTime = currentTime;
    JSONVar responseObject;
    responseObject["position"] = position;
    responseObject["temperature"] = (temprature_sens_read() - 32) / 1.8;
    switch (gateState) {
      case opening:
        responseObject["state"] = 2;
        break;
      case opened:
        responseObject["state"] = 1;
        break;
      case closing:
        responseObject["state"] = 4;
        break;
      case closed:
        responseObject["state"] = 3;
        break;
    }
    const String responseString = JSON.stringify(responseObject);
    client.publish("GateResponse", responseString , true, 2);
  }
}
