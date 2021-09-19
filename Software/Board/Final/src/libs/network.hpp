#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "driver/adc.h"

class NetworkHandler {
public:
    NetworkHandler();
    ~NetworkHandler();

private:
    void connect();
    void disconn();

    String ssid = "ssid";
    String pass = "pass";
};