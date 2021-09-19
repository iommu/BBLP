#include "network.hpp"

NetworkHandler::NetworkHandler() {
  // Shutdown wifi to save power
  disconn();
}

void NetworkHandler::connect() {
  adc_power_on();
  WiFi.disconnect(false); // Reconnect the network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.print("Connecting to WIFI : ");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("WiFi connected with IP addr :");
  Serial.println(WiFi.localIP());
}

void NetworkHandler::disconn() {
  // Disconnect and power down wifi
  adc_power_off();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}