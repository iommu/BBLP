#include "driver/adc.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "time.h"
#include <esp_wifi.h>

class NetworkHandler {
public:
  NetworkHandler();
  String getQuestions();
  void uploadAnswers(String answers);
  void updateTime();

private:
  void connect();
  void disconn();

  String server_uri = "http://";
  String ssid = "ssid";
  String pass = "pass";
};