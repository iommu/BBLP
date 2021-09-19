#include <Arduino.h>
#include <WiFi.h>
#include <HttpClient.h>
#include <ArduinoJson.h>
#include <esp_wifi.h>
#include "driver/adc.h"

class NetworkHandler {
public:
    NetworkHandler();
    ~NetworkHandler();
    String getQuestions();
    void uploadAnswers(String answers);

private:
    void connect();
    void disconn();

    String ssid = "ssid";
    String pass = "pass";
};